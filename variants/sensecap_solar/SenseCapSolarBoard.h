#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include <helpers/NRF52Board.h>
#include "SenseCapSolarSeams.h"

class SenseCapSolarBoard : public NRF52BoardDCDC {
protected:
  float _adc_multiplier = ADC_MULTIPLIER;
  uint16_t _ema_vbat_mv = 0;
  float _last_cal_temp_c = -1000.0F;
  uint32_t _last_wdt_feed_ms = 0;
  uint32_t _last_pwr_check_ms = 0;
  uint8_t _low_vbat_streak = 0;
  bool _gps_shed_by_batt = false;

  void calibrateSaadcOffset();
  uint16_t sampleRawBattMilliVolts();

#ifdef NRF52_POWER_MANAGEMENT
  void initiateShutdown(uint8_t reason) override;
#endif

public:
  SenseCapSolarBoard() : NRF52Board("SENSECAP_SOLAR_OTA") {}
  void begin() override;
  void loop() override;
  void shutdownPeripherals() override;
  bool handleCommand(const char* command, uint32_t sender_timestamp, char* reply) override;

#if defined(P_LORA_TX_LED)
  void onBeforeTransmit() override {
    digitalWrite(P_LORA_TX_LED, HIGH);  // turn TX LED on
  }
  void onAfterTransmit() override {
    digitalWrite(P_LORA_TX_LED, LOW);   // turn TX LED off
  }
#endif

  bool setAdcMultiplier(float multiplier) override {
    if (multiplier > 0.0F) {
      _adc_multiplier = multiplier;
    } else {
      _adc_multiplier = ADC_MULTIPLIER;
    }
    return true;
  }

  float getAdcMultiplier() const override {
    return _adc_multiplier;
  }

  bool hasUsbPowerDetect() const override { return true; }
  bool isUsbPowered() override { return isExternalPowered(); }

  uint16_t getBattMilliVolts() override;

  const char* getManufacturerName() const override {
    return "Seeed SenseCap Solar";
  }

  void powerOff() override {
    digitalWrite(LED_WHITE, LOW);
    digitalWrite(LED_BLUE, LOW);

#ifdef PIN_USER_BTN
    while (digitalRead(PIN_USER_BTN) == LOW) {
      delay(10);
    }
    // Keep pull-up enabled in system-off so the wake line doesn't float low.
    nrf_gpio_cfg_sense_input(
        digitalPinToInterrupt(g_ADigitalPinMap[PIN_USER_BTN]),
        NRF_GPIO_PIN_PULLUP,
        NRF_GPIO_PIN_SENSE_LOW);
#elif defined(PIN_BUTTON1)
    while (digitalRead(PIN_BUTTON1) == LOW) {
      delay(10);
    }
    nrf_gpio_cfg_sense_input(
        digitalPinToInterrupt(g_ADigitalPinMap[PIN_BUTTON1]),
        NRF_GPIO_PIN_PULLUP,
        NRF_GPIO_PIN_SENSE_LOW);
#endif

#ifdef NRF52_POWER_MANAGEMENT
    initiateShutdown(SHUTDOWN_REASON_USER);
#else
    NRF52Board::powerOff();
#endif
  }
};
