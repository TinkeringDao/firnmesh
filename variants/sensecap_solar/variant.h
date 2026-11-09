#ifndef _SEEED_SENSECAP_SOLAR_H_
#define _SEEED_SENSECAP_SOLAR_H_

/** Master clock frequency */
#define VARIANT_MCK             (64000000ul)

#define USE_LFXO                // Board uses 32khz crystal for LF

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"
#ifdef __cplusplus
#include "SenseCapSolarSeams.h"
#endif

#ifndef MAX_PACKET_HASHES
#define MAX_PACKET_HASHES       (512)
#endif
#ifndef MAX_TKS_ENTRIES
#define MAX_TKS_ENTRIES         (64)
#endif

// S1-F7: Match exact g_ADigitalPinMap[] length (27 entries: D0..D26)
#define PINS_COUNT              (27)
#define NUM_DIGITAL_PINS        (27)
#define NUM_ANALOG_INPUTS       (8)
#define NUM_ANALOG_OUTPUTS      (0)

// LEDs
#define PIN_LED                 (12)
#define LED_PWR                 (PINS_COUNT)

#define LED_BUILTIN             (PIN_LED)

#define LED_RED                 (PINS_COUNT)
#define LED_WHITE               (11)
#define LED_BLUE                (12)    // LoRa TX indicator

#define LED_STATE_ON            (1)     // State when LED is litted

// Buttons
#define PIN_BUTTON1             (13)
#define PIN_BUTTON2             (20)
#define PIN_USER_BTN            PIN_BUTTON1

#define VBAT_ENABLE             (19)    // Output LOW to enable reading of the BAT voltage.

// Analog pins
// S1-F13: Exact 1.00 MOhm / 510 kOhm voltage divider ratio = 1510 / 510 = 2.9607843f
// Combined with AREF_VOLTAGE = 3.0V (0.6V internal bandgap * 5x gain = 3000 mV full scale)
#define BATTERY_PIN             (16)    // Read the BAT voltage (P0.31 / AIN7).
#define AREF_VOLTAGE            (3.0F)
#define ADC_MULTIPLIER          (1510.0F / 510.0F)
#define ADC_RESOLUTION          (12)

// nRF52 power management & Sub-Zero winter battery settings
// S1-F11 & S1-F12: Lower bootlock to 3100 mV so Stage-1 Sub-Zero Hochnebel hibernate
// wake at 3250 mV (~12% SoC) is never trapped in a boot-protection loop.
#define PWRMGT_VOLTAGE_CRITICAL (3050)  // Deep SYSTEMOFF threshold (<0C compensated)
#define PWRMGT_VOLTAGE_BOOTLOCK (3100)  // Won't cold-boot below this voltage (mV)
#define PWRMGT_VOLTAGE_RECOVERY (3250)  // Stage-1 SYSTEMON RTC SAADC wake threshold (mV)
#define PWRMGT_LPCOMP_AIN       (7)     // AIN7 = P0.31 = BATTERY_PIN
#define PWRMGT_LPCOMP_REFSEL    (2)     // 3/8 VDD (Stage-2 deep SYSTEMOFF hardware fallback)

// Serial interfaces
#define PIN_SERIAL1_RX          (7)
#define PIN_SERIAL1_TX          (6)

// SPI Interfaces
#define SPI_INTERFACES_COUNT    (1)

#define PIN_SPI_MISO            (9)
#define PIN_SPI_MOSI            (10)
#define PIN_SPI_SCK             (8)

// Lora SPI is on SPI0
#define P_LORA_SCLK             PIN_SPI_SCK
#define P_LORA_MISO             PIN_SPI_MISO
#define P_LORA_MOSI             PIN_SPI_MOSI

// Wire Interfaces
#define WIRE_INTERFACES_COUNT   (1)

#define PIN_WIRE_SDA            (14)
#define PIN_WIRE_SCL            (15)

// GPS XIAO L76K
#define GPS_BAUDRATE            9600
#define GPS_BAUD_RATE           GPS_BAUDRATE
#define GPS_THREAD_INTERVAL     50
#define PIN_GPS_TX              PIN_SERIAL1_RX
#define PIN_GPS_RX              PIN_SERIAL1_TX
#define PIN_GPS_STANDBY         (0)     // D0 = P0.02 (GNSS_WAKEUP)
#define PIN_GPS_RESET           (17)    // D17 = P1.03 (GNSS_RESET)
#define GPS_RESET               (-1)    // Managed in board/sensor init to prevent 3.3V back-powering when GPS_EN is LOW
#define PIN_GPS_EN              (18)    // D18 = P1.05 (GNSS_ENABLE)
#define GPS_EN                  PIN_GPS_EN
#define PIN_GPS_EN_ACTIVE       HIGH
#define GPS_EN_ACTIVE           HIGH

// QSPI Pins (Onboard P25Q16H 2MB NOR Flash)
#define PIN_QSPI_SCK            (21)
#define PIN_QSPI_CS             (22)
#define PIN_QSPI_IO0            (23)
#define PIN_QSPI_IO1            (24)
#define PIN_QSPI_IO2            (25)
#define PIN_QSPI_IO3            (26)

#define EXTERNAL_FLASH_DEVICES  P25Q16H
#define EXTERNAL_FLASH_USE_QSPI

#ifdef __cplusplus
extern "C" {
#endif
void sensecapQspiFlashDeepPowerDown(void);
#ifdef __cplusplus
}
#endif

#endif
