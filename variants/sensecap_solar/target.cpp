#include <Arduino.h>
#include "target.h"
#include <helpers/ArduinoHelpers.h>
#ifdef ENV_INCLUDE_GPS
#include <helpers/sensors/MicroNMEALocationProvider.h>
#endif
SenseCapSolarBoard board;

// S2-F9: Request 8 MHz SPI clock (4x faster than RadioLib's 2 MHz default while avoiding nRF52840 SPIM3 Errata 198 at 16/32 MHz)
RADIO_CLASS radio = new Module(
    P_LORA_NSS,
    P_LORA_DIO_1,
    P_LORA_RESET,
    P_LORA_BUSY,
    SPI,
    SPISettings(8000000, MSBFIRST, SPI_MODE0));

WRAPPER_CLASS radio_driver(radio, board);

VolatileRTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
#ifdef ENV_INCLUDE_GPS
MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1, &rtc_clock);
EnvironmentSensorManager sensors = EnvironmentSensorManager(nmea);
#else
EnvironmentSensorManager sensors = EnvironmentSensorManager();
#endif

bool radio_init() {
    if (digitalRead(PIN_WIRE_SDA) == HIGH && digitalRead(PIN_WIRE_SCL) == HIGH) {
        rtc_clock.begin(Wire);
    }

    return radio.std_init(&SPI);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
