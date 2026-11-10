#include "variant.h"
#include "Arduino.h"
#include "wiring_constants.h"
#include "wiring_digital.h"
#include "nrf.h"

const uint32_t g_ADigitalPinMap[] = {
    // D0 .. D10 - Peripheral control pins
    2,  // D0  P0.02 (A0)    GNSS_WAKEUP
    3,  // D1  P0.03 (A1)    LORA_DIO1
    28, // D2  P0.28 (A2)    LORA_RESET
    29, // D3  P0.29 (A3)    LORA_BUSY
    4,  // D4  P0.04 (A4/SDA) LORA_CS
    5,  // D5  P0.05 (A5/SCL) LORA_SW
    43, // D6  P1.11 (UART_TX) GNSS_TX
    44, // D7  P1.12 (UART_RX) GNSS_RX
    45, // D8  P1.13 (SPI_SCK) LORA_SCK
    46, // D9  P1.14 (SPI_MISO) LORA_MISO
    47, // D10 P1.15 (SPI_MOSI) LORA_MOSI

    // D11-D12 - LED outputs
    15, // D11 P0.15 User LED (White LED)
    19, // D12 P0.19 Breathing LED (Blue LED - LoRa TX)

    // D13 - User input
    33, // D13 P1.01 User Button

    // D14-D15 - Grove/NFC interface
    9,  // D14 P0.09 NFC1/GROVE_D1
    10, // D15 P0.10 NFC2/GROVE_D0

    // D16 - Power management
    31, // D16 P0.31 VBAT_ADC (Battery voltage)

    // D17-D20 - GNSS & Power control
    35, // D17 P1.03 GNSS_RESET
    37, // D18 P1.05 GNSS_ENABLE
    14, // D19 P0.14 BAT_READ
    39, // D20 P1.07 USER_BUTTON

    // D21-D26 - P25Q16H QSPI Flash
    21, // D21 P0.21 (QSPI_SCK)
    25, // D22 P0.25 (QSPI_CSN)
    20, // D23 P0.20 (QSPI_SIO_0 DI)
    24, // D24 P0.24 (QSPI_SIO_1 DO)
    22, // D25 P0.22 (QSPI_SIO_2 WP)
    23, // D26 P0.23 (QSPI_SIO_3 HOLD)
};

static_assert(sizeof(g_ADigitalPinMap) / sizeof(g_ADigitalPinMap[0]) == PINS_COUNT,
              "g_ADigitalPinMap size mismatch with PINS_COUNT");

static void qspiShiftOutByte(uint8_t byte_val) {
    for (int8_t bit = 7; bit >= 0; bit--) {
        digitalWrite(PIN_QSPI_SCK, LOW);
        digitalWrite(PIN_QSPI_IO0, (byte_val & (1U << bit)) ? HIGH : LOW);
        digitalWrite(PIN_QSPI_SCK, HIGH);
    }
    digitalWrite(PIN_QSPI_SCK, LOW);
}

// S1-F5: Place onboard P25Q16H 2MB QSPI NOR Flash into Deep Power-Down (0xB9, <1 uA)
// using bit-banged SPI Mode 0 to avoid nRF52840 QSPI peripheral Errata 122 (+1.2 mA leak).
void sensecapQspiFlashDeepPowerDown() {
    pinMode(PIN_QSPI_CS, OUTPUT);
    pinMode(PIN_QSPI_SCK, OUTPUT);
    pinMode(PIN_QSPI_IO0, OUTPUT);
    pinMode(PIN_QSPI_IO1, INPUT_PULLUP);
    pinMode(PIN_QSPI_IO2, OUTPUT);
    pinMode(PIN_QSPI_IO3, OUTPUT);

    digitalWrite(PIN_QSPI_CS, HIGH);
    digitalWrite(PIN_QSPI_SCK, LOW);
    digitalWrite(PIN_QSPI_IO2, HIGH); // WP# deasserted
    digitalWrite(PIN_QSPI_IO3, HIGH); // HOLD# deasserted

    // 1. Issue Release from Deep Power-Down (0xAB) in case flash is in an unknown state
    digitalWrite(PIN_QSPI_CS, LOW);
    qspiShiftOutByte(0xAB);
    digitalWrite(PIN_QSPI_CS, HIGH);
    delayMicroseconds(30);

    // 2. Issue Deep Power-Down command (0xB9) -> reduces standby from ~35 uA to < 1 uA
    digitalWrite(PIN_QSPI_CS, LOW);
    qspiShiftOutByte(0xB9);
    digitalWrite(PIN_QSPI_CS, HIGH);
    delayMicroseconds(10);

    // Disconnect CMOS input buffers on all non-CS QSPI pins (P0.20..P0.24) to eliminate
    // floating-input shoot-through current while keeping CS# (P0.25) driven HIGH.
    const uint32_t kDisconnectPinCnf =
        (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
        (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
        (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos);
    NRF_P0->PIN_CNF[20] = kDisconnectPinCnf; // QSPI_SIO_0 (DI)
    NRF_P0->PIN_CNF[21] = kDisconnectPinCnf; // QSPI_SCK
    NRF_P0->PIN_CNF[22] = kDisconnectPinCnf; // QSPI_SIO_2 (WP#)
    NRF_P0->PIN_CNF[23] = kDisconnectPinCnf; // QSPI_SIO_3 (HOLD#)
    NRF_P0->PIN_CNF[24] = kDisconnectPinCnf; // QSPI_SIO_1 (DO)
}

void initVariant() {
    // S4-F4: Ensure P0.09 (Grove SDA) and P0.10 (Grove SCL) are decoupled from the NFCT peripheral
    if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk) ==
        (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos)) {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
        NRF_UICR->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
        NVIC_SystemReset();
    }

    // S4-F1 & S4-F3: Power off XIAO L76K and disconnect CMOS input buffers on all GNSS I/O lines
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

    // S1-F4: Keep battery divider disabled (HIGH) when idle; enabled only during SAADC read or LPCOMP sleep
    pinMode(BATTERY_PIN, INPUT);
    pinMode(VBAT_ENABLE, OUTPUT);
    digitalWrite(VBAT_ENABLE, HIGH);

    // S4-F5: Configure both user buttons with internal pull-ups to prevent floating input leakage
    pinMode(PIN_BUTTON1, INPUT_PULLUP);
    pinMode(PIN_BUTTON2, INPUT_PULLUP);

    // S1-F5: Put P25Q16H QSPI flash into Deep Power-Down (<1 uA)
    sensecapQspiFlashDeepPowerDown();

    pinMode(LED_WHITE, OUTPUT);
    digitalWrite(LED_WHITE, LOW);

    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, LOW);
}
