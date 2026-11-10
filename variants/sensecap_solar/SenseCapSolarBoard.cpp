#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include "nrf.h"

#include "SenseCapSolarBoard.h"
#include "target.h"

#ifdef NRF52_POWER_MANAGEMENT
const PowerMgtConfig power_config = {
  .lpcomp_ain_channel = PWRMGT_LPCOMP_AIN,
  .lpcomp_refsel = PWRMGT_LPCOMP_REFSEL,
  .voltage_bootlock = PWRMGT_VOLTAGE_BOOTLOCK
};
#endif

void SenseCapSolarBoard::calibrateSaadcOffset() {
  NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled;
  NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
  NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
  uint32_t start_us = micros();
  while (!NRF_SAADC->EVENTS_CALIBRATEDONE && (uint32_t)(micros() - start_us) < 2000U) {}
  NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
  NRF_SAADC->EVENTS_STOPPED = 0;
  NRF_SAADC->TASKS_STOP = 1;
  start_us = micros();
  while (!NRF_SAADC->EVENTS_STOPPED && (uint32_t)(micros() - start_us) < 2000U) {}
  NRF_SAADC->EVENTS_STOPPED = 0;
  NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Disabled;
}

uint16_t SenseCapSolarBoard::sampleRawBattMilliVolts() {
  digitalWrite(VBAT_ENABLE, LOW);
  analogReadResolution(12);
  analogReference(AR_INTERNAL_3_0);
  analogSampleTime(40);
  analogOversampling(8);
  int adcvalue = analogRead(BATTERY_PIN);
  return sensecap::adcToMilliVolts(adcvalue, _adc_multiplier, AREF_VOLTAGE);
}

uint16_t SenseCapSolarBoard::getBattMilliVolts() {
  // S1-F4 & S1-F14: Zero blocking delay while awake (VBAT_ENABLE stays LOW so filter cap stays charged),
  // 40 us acquisition time for 337.7 kOhm source impedance, 8x hardware oversampling, and 1/4 EMA filter.
  uint16_t sample_mv = sampleRawBattMilliVolts();
  _ema_vbat_mv = sensecap::updateBatteryEma(_ema_vbat_mv, sample_mv);
  return _ema_vbat_mv;
}

void SenseCapSolarBoard::shutdownPeripherals() {
  // S1-F2: Shut down LoRa radio, GPS, CC310, LEDs, SPI, I2C, and QSPI flash before entering sleep/SystemOff
  NRF52Board::shutdownPeripherals();
  SPI.end();

#ifdef LED_WHITE
  digitalWrite(LED_WHITE, LOW);
#endif
#ifdef LED_BLUE
  digitalWrite(LED_BLUE, LOW);
#endif
#ifdef SX126X_RXEN
  pinMode(SX126X_RXEN, OUTPUT);
  digitalWrite(SX126X_RXEN, LOW);
#endif

  // S4-F1: Eliminate 1-5 mA UART TX ESD back-powering and disconnect CMOS input buffers on powered-off L76K
  if (NRF_UARTE0->ENABLE != 0) {
    NRF_UARTE0->PSEL.RXD = 0xFFFFFFFF;
    Serial1.end();
  }
  pinMode(GPS_EN, OUTPUT);
  digitalWrite(GPS_EN, LOW);
  const uint32_t kDisconnectPinCnf =
      (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
      (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
      (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos);
  NRF_P0->PIN_CNF[2]  = kDisconnectPinCnf; // P0.02 GNSS_WAKEUP
  NRF_P1->PIN_CNF[3]  = kDisconnectPinCnf; // P1.03 GNSS_RESET
  NRF_P1->PIN_CNF[11] = kDisconnectPinCnf; // P1.11 GNSS_TX
  NRF_P1->PIN_CNF[12] = kDisconnectPinCnf; // P1.12 GNSS_RX

  Wire.end();
  NRF_P0->PIN_CNF[9]  = kDisconnectPinCnf; // P0.09 Grove SDA
  NRF_P0->PIN_CNF[10] = kDisconnectPinCnf; // P0.10 Grove SCL

  // S1-F5: Ensure P25Q16H 2MB QSPI flash is in Deep Power-Down (<1 uA)
  sensecapQspiFlashDeepPowerDown();
}

#ifdef NRF52_POWER_MANAGEMENT
void SenseCapSolarBoard::initiateShutdown(uint8_t reason) {
  shutdownPeripherals();

  const bool is_low_voltage =
      (reason == SHUTDOWN_REASON_LOW_VOLTAGE || reason == SHUTDOWN_REASON_BOOT_PROTECT);

  if (is_low_voltage) {
    // S1-F12: Two-Stage Sub-Zero Winter Hibernate
    // Stage 1 (3050..3249 mV): Stay in ~3 uA SYSTEMON tickless RTC sleep and poll SAADC (0.6V internal
    // reference) every 30s so the node recovers at 3250 mV (~12% SoC) instead of waiting for LPCOMP
    // REFSEL=2 (3/8 VDD = 3664 mV = 42% SoC), which strands the node for 6-11 days in Hochnebel fog.
    uint16_t vbat_mv = sampleRawBattMilliVolts();
    while (sensecap::selectHibernateStage(vbat_mv, isExternalPowered()) ==
           sensecap::HibernateStage::SYSTEMON_BANDGAP_POLL) {
      // Turn off voltage divider between 30s probes to save 2.45 uA
      digitalWrite(VBAT_ENABLE, HIGH);

      for (uint8_t sec = 0; sec < 30; sec++) {
        if (NRF_WDT->RUNSTATUS & WDT_RUNSTATUS_RUNSTATUS_Msk) {
          NRF_WDT->RR[0] = WDT_RR_RR_Reload;
        }
        if (isExternalPowered()) {
          NVIC_SystemReset();
        }
        delay(1000);
      }

      digitalWrite(VBAT_ENABLE, LOW);
      delay(5);  // allow external divider filter cap to charge after 30s high-Z sleep
      vbat_mv = sampleRawBattMilliVolts();
      if (sensecap::selectHibernateStage(vbat_mv, isExternalPowered()) ==
          sensecap::HibernateStage::WAKE_AND_REBOOT) {
        NVIC_SystemReset();
      }
    }
  }

  // Stage 2 (<3050 mV critical) or user powerOff(): Enter <1 uA SYSTEMOFF with LPCOMP + button wake
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, is_low_voltage ? LOW : HIGH);

#ifdef PIN_BUTTON1
  nrf_gpio_cfg_sense_input(
      digitalPinToInterrupt(g_ADigitalPinMap[PIN_BUTTON1]),
      NRF_GPIO_PIN_PULLUP,
      NRF_GPIO_PIN_SENSE_LOW);
#endif
#ifdef PIN_BUTTON2
  nrf_gpio_cfg_sense_input(
      digitalPinToInterrupt(g_ADigitalPinMap[PIN_BUTTON2]),
      NRF_GPIO_PIN_PULLUP,
      NRF_GPIO_PIN_SENSE_LOW);
#endif

  if (is_low_voltage) {
    configureVoltageWake(power_config.lpcomp_ain_channel, power_config.lpcomp_refsel);
  }

  enterSystemOff(reason);
}
#endif // NRF52_POWER_MANAGEMENT

void SenseCapSolarBoard::begin() {
  NRF52BoardDCDC::begin();

  pinMode(BATTERY_PIN, INPUT);
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  delay(5);  // initial filter capacitor charge

  calibrateSaadcOffset();
  float boot_temp = getMCUTemperature();
  if (!isnan(boot_temp)) {
    _last_cal_temp_c = boot_temp;
  }

#ifdef PIN_USER_BTN
  pinMode(PIN_USER_BTN, INPUT_PULLUP);
#elif defined(PIN_BUTTON1)
  pinMode(PIN_BUTTON1, INPUT_PULLUP);
#endif
#ifdef PIN_BUTTON2
  pinMode(PIN_BUTTON2, INPUT_PULLUP);
#endif

#ifdef LED_WHITE
  pinMode(LED_WHITE, OUTPUT);
  digitalWrite(LED_WHITE, LOW);
#endif
#ifdef LED_BLUE
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, LOW);
#endif
#ifdef P_LORA_TX_LED
  pinMode(P_LORA_TX_LED, OUTPUT);
  digitalWrite(P_LORA_TX_LED, LOW);
#endif

#ifdef NRF52_POWER_MANAGEMENT
  // Run boot voltage check BEFORE starting I2C or external peripherals
  checkBootVoltage(&power_config);
#endif

  // S4-F2: Arm hardware watchdog (60s timeout, paused during SLEEP and HALT)
  if (!(NRF_WDT->RUNSTATUS & WDT_RUNSTATUS_RUNSTATUS_Msk)) {
    NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos) |
                      (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos);
    NRF_WDT->CRV = 60UL * 32768UL;
    NRF_WDT->RREN = WDT_RREN_RR0_Enabled << WDT_RREN_RR0_Pos;
    NRF_WDT->TASKS_START = 1;
  }
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;
  _last_wdt_feed_ms = millis();
  _last_pwr_check_ms = millis();

#if defined(PIN_WIRE_SDA) && defined(PIN_WIRE_SCL)
  Wire.setPins(PIN_WIRE_SDA, PIN_WIRE_SCL);
#endif
  Wire.begin();
}

void SenseCapSolarBoard::loop() {
  const uint32_t now_ms = millis();

  // S4-F2: Feed hardware watchdog at 1 Hz
  if ((uint32_t)(now_ms - _last_wdt_feed_ms) >= 1000U) {
    _last_wdt_feed_ms = now_ms;
    if (NRF_WDT->RUNSTATUS & WDT_RUNSTATUS_RUNSTATUS_Msk) {
      NRF_WDT->RR[0] = WDT_RR_RR_Reload;
    }
  }

  // S1-F11: Sub-Zero winter battery & thermal monitor (runs every 30 seconds)
  if ((uint32_t)(now_ms - _last_pwr_check_ms) >= 30000U) {
    _last_pwr_check_ms = now_ms;

    if (isExternalPowered()) {
      _low_vbat_streak = 0;
      return;
    }

    float mcu_temp_c = getMCUTemperature();
    if (!isnan(mcu_temp_c) && fabsf(mcu_temp_c - _last_cal_temp_c) >= 10.0F) {
      calibrateSaadcOffset();
      _last_cal_temp_c = mcu_temp_c;
    }

    const uint16_t vbat_mv = getBattMilliVolts();
    const float effective_temp_c = isnan(mcu_temp_c) ? 20.0F : mcu_temp_c;
    const sensecap::ColdWeatherBatteryDecision decision = sensecap::evaluateColdWeatherBatteryState(
        vbat_mv, effective_temp_c, false, _low_vbat_streak);

#ifdef NRF52_POWER_MANAGEMENT
    if (decision.trigger_shutdown) {
      initiateShutdown(SHUTDOWN_REASON_LOW_VOLTAGE);
      return;
    }
#endif

    if (decision.shed_gps) {
#if ENV_INCLUDE_GPS
      if (sensors.getLocationProvider() != nullptr && sensors.getLocationProvider()->isEnabled()) {
        sensors.setSettingValue("gps", "0");
        _gps_shed_by_batt = true;
      }
#endif
    } else if (_gps_shed_by_batt && decision.restore_gps) {
#if ENV_INCLUDE_GPS
      sensors.setSettingValue("gps", "1");
#endif
      _gps_shed_by_batt = false;
    }
  }
}

bool SenseCapSolarBoard::handleCommand(const char* command, uint32_t sender_timestamp, char* reply) {
  (void)sender_timestamp;
  while (*command == ' ') command++;

  if (strcmp(command, "silicon") == 0) {
    uint32_t var_raw = NRF_FICR->INFO.VARIANT;
    char var_str[5] = {
      (char)((var_raw >> 24) & 0xFF),
      (char)((var_raw >> 16) & 0xFF),
      (char)((var_raw >> 8) & 0xFF),
      (char)(var_raw & 0xFF),
      '\0'
    };
    for (int i = 0; i < 4; i++) {
      if (var_str[i] < 0x20 || var_str[i] > 0x7E) var_str[i] = '?';
    }
    snprintf(
        reply,
        160,
        "{\"part\":\"%lX\",\"var\":\"%s\",\"id\":\"%08lX%08lX\",\"dcdc\":%lu,\"nfc\":\"0x%08lX\",\"hf\":\"0x%05lX\",\"lf\":\"0x%05lX\",\"wdt\":%lu,\"usb\":\"0x%lX\"}",
        (unsigned long)NRF_FICR->INFO.PART,
        var_str,
        (unsigned long)NRF_FICR->DEVICEID[1],
        (unsigned long)NRF_FICR->DEVICEID[0],
        (unsigned long)(NRF_POWER->DCDCEN & 1U),
        (unsigned long)NRF_UICR->NFCPINS,
        (unsigned long)NRF_CLOCK->HFCLKSTAT,
        (unsigned long)NRF_CLOCK->LFCLKSTAT,
        (unsigned long)(NRF_WDT->RUNSTATUS & 1U),
        (unsigned long)NRF_POWER->USBREGSTATUS);
    return true;
  }

  if (strcmp(command, "battdiag") == 0) {
    uint16_t b_min = 0xFFFF;
    uint16_t b_max = 0;
    uint32_t sum_mv = 0;
    uint32_t t0 = micros();
    for (int i = 0; i < 16; i++) {
      uint16_t v = sampleRawBattMilliVolts();
      if (v < b_min) b_min = v;
      if (v > b_max) b_max = v;
      sum_mv += v;
    }
    uint32_t burst_us = (uint32_t)(micros() - t0);
    uint16_t raw_mean = (uint16_t)((sum_mv + 8U) / 16U);
    uint16_t ema_mv = getBattMilliVolts();
    float temp_c = getMCUTemperature();
    float eff_temp = isnan(temp_c) ? 20.0F : temp_c;
    uint16_t crit_mv = (eff_temp < 0.0F) ? 3050U : 3150U;
    snprintf(
        reply,
        160,
        "{\"ema\":%u,\"raw\":%u,\"min16\":%u,\"max16\":%u,\"us16\":%lu,\"boot\":%u,\"temp\":%.2f,\"cal_t\":%.1f,\"cut\":%u,\"strk\":%u,\"ext\":%u}",
        (unsigned)ema_mv,
        (unsigned)raw_mean,
        (unsigned)b_min,
        (unsigned)b_max,
        (unsigned long)burst_us,
        (unsigned)getBootVoltage(),
        (double)eff_temp,
        (double)_last_cal_temp_c,
        (unsigned)crit_mv,
        (unsigned)_low_vbat_streak,
        (unsigned)(isExternalPowered() ? 1U : 0U));
    return true;
  }

  if (strcmp(command, "rfdiag") == 0) {
    uint8_t rxgain = 0, ocp = 0, clamp = 0, evt = 0, xta = 0;
    radio.readRegister(RADIOLIB_SX126X_REG_RX_GAIN, &rxgain, 1);
    radio.readRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
    radio.readRegister(RADIOLIB_SX126X_REG_TX_CLAMP_CONFIG, &clamp, 1);
    radio.readRegister(RADIOLIB_SX126X_REG_EVENT_MASK, &evt, 1);
    radio.readRegister(0x08B5, &xta, 1);
    uint16_t dev_err = radio.getDeviceErrors();
    uint32_t irq = radio.getIrqFlags();
    float inst_rssi = radio.getRSSI(false);
    snprintf(
        reply,
        160,
        "{\"rxg\":\"0x%02X\",\"ocp\":\"0x%02X\",\"clmp\":\"0x%02X\",\"evt\":\"0x%02X\",\"xta\":\"0x%02X\",\"err\":\"0x%04X\",\"irq\":\"0x%04lX\",\"rssi\":%.1f,\"qspi\":\"0x%lX\",\"gps\":\"0x%lX\"}",
        (unsigned)rxgain,
        (unsigned)ocp,
        (unsigned)clamp,
        (unsigned)evt,
        (unsigned)xta,
        (unsigned)dev_err,
        (unsigned long)(irq & 0xFFFFU),
        (double)inst_rssi,
        (unsigned long)NRF_P0->PIN_CNF[20],
        (unsigned long)NRF_P1->PIN_CNF[11]);
    return true;
  }

  return false;
}
