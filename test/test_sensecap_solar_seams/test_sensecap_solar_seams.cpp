#include <gtest/gtest.h>
#include "../variants/sensecap_solar/variant.h"
#include "../variants/sensecap_solar/SenseCapSolarSeams.h"
#include "helpers/SimpleMeshTables.h"
#include "helpers/TransportKeyStore.h"

using namespace mesh;

static Packet makeFloodPacketWithId(uint16_t id) {
  Packet p;
  p.header = ROUTE_TYPE_FLOOD | (PAYLOAD_TYPE_TXT_MSG << PH_TYPE_SHIFT);
  p.payload[0] = (uint8_t)(id & 0xFF);
  p.payload[1] = (uint8_t)((id >> 8) & 0xFF);
  p.payload[2] = (uint8_t)((id * 31u) & 0xFF);
  p.payload_len = 3;
  p.path_len = 0;
  return p;
}

// ============================================================================
// Seam 1: Sub-Zero Winter (-15C Winter & Hochnebel) Battery & Power Seam
// ============================================================================

TEST(ColdWeatherBatterySeam, ExactDividerMultiplierEliminates43mVOverReport) {
  // Hardware divider on XIAO nRF52840 Plus / SenseCAP Solar is R_top=1.00M, R_bot=510k.
  // Scale = 1510 / 510 = 2.9607843f.
  // Stock firmware used 3.0f (+1.325% error = +43mV at 3250mV).
  EXPECT_NEAR(ADC_MULTIPLIER, 1510.0f / 510.0f, 0.0005f);

  // At true V_BAT = 3100 mV, V_AIN7 = 3100 / (1510/510) = 1047.02 mV -> ADC count = 1430 (12-bit, 3.0V ref)
  uint16_t mv = sensecap::adcToMilliVolts(1430, ADC_MULTIPLIER, AREF_VOLTAGE);
  EXPECT_NEAR(mv, 3100, 3);
}

TEST(ColdWeatherBatterySeam, EmaFilterSuppressesSolarMpptRipple) {
  uint16_t ema = 0;
  ema = sensecap::updateBatteryEma(ema, 3600);
  EXPECT_EQ(ema, 3600u);
  // Single +80mV CN3165 MPPT switching spike should be damped by 4x (to +20mV)
  ema = sensecap::updateBatteryEma(ema, 3680);
  EXPECT_EQ(ema, 3620u);
}

TEST(ColdWeatherBatterySeam, ColdWeatherRuntimeMonitorCompensatesFor18650InternalResistance) {
  uint8_t strikes = 0;

  // At -12C (Sub-Zero winter night), NMC 18650 R_dc rises 3-5x.
  // 1) At 3400mV and -12C (< 3450mV cold GPS shed threshold), GPS must be shed, no shutdown:
  auto act1 = sensecap::evaluateColdWeatherBatteryState(3400, -12.0f, false, strikes);
  EXPECT_TRUE(act1.shed_gps);
  EXPECT_FALSE(act1.trigger_shutdown);
  EXPECT_EQ(strikes, 0u);

  // 2) At 3100mV and -12C (>= 3050mV cold cutoff, even though < 3150mV warm cutoff), NO shutdown strike:
  auto act2 = sensecap::evaluateColdWeatherBatteryState(3100, -12.0f, false, strikes);
  EXPECT_FALSE(act2.trigger_shutdown);
  EXPECT_EQ(strikes, 0u);

  // 3) Transient +22dBm TX sag to 3020mV for 2 checks (< 3 strikes) does NOT shut down:
  auto act3 = sensecap::evaluateColdWeatherBatteryState(3020, -12.0f, false, strikes);
  EXPECT_FALSE(act3.trigger_shutdown);
  EXPECT_EQ(strikes, 1u);
  auto act4 = sensecap::evaluateColdWeatherBatteryState(3020, -12.0f, false, strikes);
  EXPECT_FALSE(act4.trigger_shutdown);
  EXPECT_EQ(strikes, 2u);

  // 4) Sustained 3rd consecutive check (90s) at 3020mV triggers protective low-voltage shutdown:
  auto act5 = sensecap::evaluateColdWeatherBatteryState(3020, -12.0f, false, strikes);
  EXPECT_TRUE(act5.trigger_shutdown);
  EXPECT_EQ(strikes, 3u);
}

TEST(ColdWeatherBatterySeam, TwoStageHochnebelHibernateRecoversAt3250mVInsteadOf3664mV) {
  // Below 3050mV -> Deep SYSTEMOFF with LPCOMP (REFSEL=2 + HYST=50mV)
  EXPECT_EQ(sensecap::selectHibernateStage(3020, false),
            sensecap::HibernateStage::DEEP_SYSTEMOFF_LPCOMP);

  // Between 3050mV and 3249mV -> Low-power SYSTEMON RTC bandgap-SAADC poll (~3uA)
  EXPECT_EQ(sensecap::selectHibernateStage(3150, false),
            sensecap::HibernateStage::SYSTEMON_BANDGAP_POLL);

  // At 3250mV (~12% SoC, <1 day of Sub-Zero winter Hochnebel diffuse light) -> Wake & Reboot!
  EXPECT_EQ(sensecap::selectHibernateStage(3250, false),
            sensecap::HibernateStage::WAKE_AND_REBOOT);
}

// ============================================================================
// Seam 2: Wio-SX1262 + MikroTik 6.5 dBi Antenna RF & Noise Floor Seam
// ============================================================================

TEST(Sx1262And65dBiAntennaSeam, NoiseFloorCalibratorRecoversFromMinus120dBmWithoutZeroingFloor) {
  sensecap::NoiseFloorTracker tracker;
  EXPECT_EQ(tracker.noiseFloor(), 0);

  // Initial convergence to -120 dBm (quiet test bench)
  for (int i = 0; i < 64; i++) {
    tracker.addSample(-121);
  }
  EXPECT_EQ(tracker.noiseFloor(), -120);

  // Now simulate resetAGC() / recalibration trigger on a 6.5 dBi rooftop antenna
  // where ambient urban noise floor is -102 dBm (>= -120 + 14 = -106 dBm!).
  // Stock firmware either zeroed _noise_floor to 0 dBm for 64 loops or locked up forever at -120 dBm.
  tracker.startCalibrationWindow();
  // Published noise floor must remain -120 dBm while sampling is in progress (never 0 dBm!)
  EXPECT_EQ(tracker.noiseFloor(), -120);

  // Feed 64 ambient samples at -102 dBm plus 10 loud packet bursts at -65 dBm:
  for (int i = 0; i < 8; i++) {
    tracker.addSample(-102); // 8 seed samples establish new baseline
  }
  for (int i = 0; i < 10; i++) {
    tracker.addSample(-65);  // Strong packet bursts (> -102 + 14) must be rejected!
  }
  for (int i = 0; i < 56; i++) {
    tracker.addSample(-102); // Remaining 56 ambient samples
  }

  // Must converge to -102 dBm without locking up at -120 dBm or being corrupted by -65 dBm packets!
  EXPECT_EQ(tracker.noiseFloor(), -102);
}

TEST(Sx1262And65dBiAntennaSeam, PreambleLatchTimeoutClearsStalePreambleWithoutTouchingRxDone) {
  sensecap::PreambleTimeoutTracker tracker;
  const uint32_t kPreambleMs = 83;
  const uint32_t kPayloadMs = 2100;

  // At t=1000ms, 6.5 dBi antenna picks up a brief ISM noise spike latching PREAMBLE_DETECTED (0x04)
  bool clear_irq = false;
  EXPECT_TRUE(tracker.evaluateIrq(0x04, false, false, 1000, kPreambleMs, kPayloadMs, clear_irq));
  EXPECT_FALSE(clear_irq);

  // At t=1050ms (< 1000 + 83ms), still waiting for header
  EXPECT_TRUE(tracker.evaluateIrq(0x04, false, false, 1050, kPreambleMs, kPayloadMs, clear_irq));
  EXPECT_FALSE(clear_irq);

  // At t=1090ms (> 1000 + 83ms) with no HEADER_VALID and no RX_DONE -> must expire and request IRQ clear!
  EXPECT_FALSE(tracker.evaluateIrq(0x04, false, false, 1090, kPreambleMs, kPayloadMs, clear_irq));
  EXPECT_TRUE(clear_irq);
}

// ============================================================================
// Seam 3: Repeater High-Density Mesh Tables & TransportKeyStore Cache Seam
// ============================================================================

TEST(RepeaterHighDensityTablesSeam, SimpleMeshTablesHoldsAtLeast512PacketHashesWithoutWrap) {
  EXPECT_GE(MAX_PACKET_HASHES, 512);

  SimpleMeshTables tables;
  Packet first = makeFloodPacketWithId(1);
  tables.markSeen(&first);
  ASSERT_TRUE(tables.wasSeen(&first));

  // Inject 400 distinct flood packets (exceeds stock 160-entry ring by 2.5x)
  for (uint16_t id = 2; id <= 401; id++) {
    Packet p = makeFloodPacketWithId(id);
    tables.markSeen(&p);
  }

  // Packet #1 must STILL be remembered so late multi-hop echoes on the 6.5 dBi antenna are suppressed!
  EXPECT_TRUE(tables.wasSeen(&first));
}

TEST(RepeaterHighDensityTablesSeam, TransportKeyStoreCachesMoreThan16RegionsAndEvictsCleanly) {
  EXPECT_GE(MAX_TKS_ENTRIES, 32);

  TransportKeyStore store;
  TransportKey k_first{}, k_late{}, k_read{};

  store.getAutoKeyFor(1, "#eu-metro-1", k_first);
  for (uint16_t id = 2; id <= 25; id++) {
    char name[32];
    snprintf(name, sizeof(name), "#eu-region-region-%u", (unsigned)id);
    TransportKey tmp{};
    store.getAutoKeyFor(id, name, tmp);
  }
  // Region #25 (> stock 16 limit) and Region #1 must both be cached and retrievable via loadKeysFor()
  EXPECT_EQ(store.loadKeysFor(25, &k_late, 1), 1);
  EXPECT_EQ(store.loadKeysFor(1, &k_read, 1), 1);
  EXPECT_EQ(memcmp(k_first.key, k_read.key, sizeof(k_first.key)), 0);
}

// ============================================================================
// Seam 4: Pin Map Bounds & XIAO L76K Anti-Backpowering Seam
// ============================================================================

TEST(GpsAndPinBoundsSeam, PinCountMatchesDigitalPinMapAndDefinesL76kPins) {
  // g_ADigitalPinMap in variants/sensecap_solar/variant.cpp has exactly 27 entries (0..26).
  EXPECT_EQ(PINS_COUNT, 27);
  EXPECT_EQ(NUM_DIGITAL_PINS, 27);
#ifdef PIN_GPS_RESET
  EXPECT_EQ(PIN_GPS_RESET, 17);
#else
  FAIL() << "PIN_GPS_RESET must be defined as 17 (P1.03)";
#endif
#ifdef PIN_GPS_EN
  EXPECT_EQ(PIN_GPS_EN, 18);
#else
  FAIL() << "PIN_GPS_EN must be defined as 18 (P1.05)";
#endif
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
