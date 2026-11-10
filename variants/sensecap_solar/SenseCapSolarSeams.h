#pragma once

#include <stdint.h>

namespace sensecap {

// S1-F13: Exact voltage divider ratio: (1.00 MOhm + 510 kOhm) / 510 kOhm = 1510 / 510 = 2.9607843f
static constexpr float SENSECAP_ADC_MULTIPLIER = 1510.0F / 510.0F;

// S1-F4 / S1-F14: Fast float conversion using 4096.0f (12-bit SAADC full scale)
inline uint16_t adcToMilliVolts(
    int raw_adc,
    float adc_multiplier = SENSECAP_ADC_MULTIPLIER,
    float aref_volts = 3.0F) {
  if (raw_adc <= 0) {
    return 0;
  }
  float mv = (static_cast<float>(raw_adc) * adc_multiplier * aref_volts * 1000.0F) / 4096.0F;
  return static_cast<uint16_t>(mv + 0.5F);
}

// S1-F4: 1/4 exponential moving average filter to suppress CN3165 MPPT switching ripple
inline uint16_t updateBatteryEma(uint16_t prev_ema_mv, uint16_t sample_mv) {
  if (prev_ema_mv == 0) {
    return sample_mv;
  }
  return static_cast<uint16_t>(
      (static_cast<uint32_t>(prev_ema_mv) * 3U + static_cast<uint32_t>(sample_mv) + 2U) / 4U);
}

// S1-F11: Sub-Zero Winter (-15C winter, Hochnebel stratus fog) runtime battery state machine
struct ColdWeatherBatteryDecision {
  bool shed_gps;
  bool restore_gps;
  bool trigger_shutdown;
};

inline ColdWeatherBatteryDecision evaluateColdWeatherBatteryState(
    uint16_t vbat_mv,
    float mcu_temp_c,
    bool external_powered,
    uint8_t& consecutive_low_strikes) {
  if (external_powered || vbat_mv < 1000U) {
    consecutive_low_strikes = 0;
    return {false, true, false};
  }

  // At < 0 C, Li-Ion charging is thermally inhibited and NMC 18650 internal resistance (R_dc)
  // increases 3-5x (~350-600 mOhm), causing ~70-120 mV transient sag during +22 dBm LoRa TX.
  // Lower critical cutoff to 3050 mV below 0 C (vs 3150 mV above 0 C) and shed 30 mA GPS early at 3450 mV.
  const uint16_t crit_mv = (mcu_temp_c < 0.0F) ? 3050U : 3150U;
  const uint16_t shed_gps_mv = (mcu_temp_c < 0.0F) ? 3450U : 3350U;
  const uint16_t restore_gps_mv = (mcu_temp_c < 0.0F) ? 3550U : 3450U;

  if (vbat_mv < crit_mv) {
    if (consecutive_low_strikes < 255U) {
      consecutive_low_strikes++;
    }
    return {true, false, consecutive_low_strikes >= 3U};
  }

  consecutive_low_strikes = 0;
  if (vbat_mv < shed_gps_mv) {
    return {true, false, false};
  }
  return {false, vbat_mv >= restore_gps_mv, false};
}

// S1-F12: Two-stage Sub-Zero Hochnebel hibernate selector
// Because the 3.3V LDO enters dropout below 3.3V, V_AIN7 / VDD = 510/1510 = 0.3378 > 5/16 VDD,
// forcing hardware LPCOMP to use REFSEL=2 (3/8 VDD = 3.664V = 42% SoC), which strands the repeater
// for 6-11 days during Sub-Zero winter Hochnebel. Stage 1 keeps the MCU in ~3 uA SYSTEMON RTC sleep
// polling the 0.6V internal bandgap SAADC every 30s so the node wakes at 3250 mV (~12% SoC).
enum class HibernateStage : uint8_t {
  DEEP_SYSTEMOFF_LPCOMP = 0,
  SYSTEMON_BANDGAP_POLL = 1,
  WAKE_AND_REBOOT = 2
};

inline HibernateStage selectHibernateStage(uint16_t vbat_mv, bool external_powered = false) {
  if (external_powered || vbat_mv >= 3250U) {
    return HibernateStage::WAKE_AND_REBOOT;
  }
  if (vbat_mv >= 3050U) {
    return HibernateStage::SYSTEMON_BANDGAP_POLL;
  }
  return HibernateStage::DEEP_SYSTEMOFF_LPCOMP;
}

// S2-F3 & S2-F7: Non-zeroing, self-seeding noise floor estimator for high-gain 6.5 dBi antennas
class NoiseFloorTracker {
  int16_t _noise_floor = 0;
  uint16_t _num_samples = 0;
  int32_t _sample_sum = 0;

public:
  static constexpr uint16_t kTargetSamples = 64;
  static constexpr uint16_t kSeedSamples = 8;
  static constexpr int16_t kGateMarginDb = 14;
  static constexpr int16_t kMaxValidNoiseDbm = -50;

  void startCalibrationWindow() {
    // S2-F3: Never zero _noise_floor during calibration!
    _num_samples = 0;
    _sample_sum = 0;
  }

  void addSample(int rssi_dbm) {
    if (_num_samples >= kTargetSamples || rssi_dbm >= kMaxValidNoiseDbm) {
      return;
    }
    // S2-F7: Accept kSeedSamples un-gated samples first so an artificially low prior floor (-120 dBm)
    // cannot permanently reject true urban ambient noise (-102 dBm on a 6.5 dBi omni antenna).
    if (_num_samples >= kSeedSamples) {
      int running_mean = _sample_sum / static_cast<int32_t>(_num_samples);
      if (rssi_dbm >= (running_mean + kGateMarginDb)) {
        return;
      }
    }
    _num_samples++;
    _sample_sum += rssi_dbm;
    if (_num_samples >= kTargetSamples) {
      _noise_floor = static_cast<int16_t>(_sample_sum / static_cast<int32_t>(kTargetSamples));
      if (_noise_floor < -120) {
        _noise_floor = -120;
      }
      _sample_sum = 0;
    }
  }

  int16_t noiseFloor() const { return _noise_floor; }
};

// S3-F3: SX1262 false-preamble latch watchdog for high-gain omni antennas
class PreambleTimeoutTracker {
  uint32_t _activity_at = 0;
  bool _header_seen = false;

public:
  static constexpr uint32_t IRQ_RX_DONE = 0x0002;
  static constexpr uint32_t IRQ_PREAMBLE_DETECTED = 0x0004;
  static constexpr uint32_t IRQ_HEADER_VALID = 0x0010;

  bool evaluateIrq(
      uint32_t irq,
      bool has_rx_done,
      bool polled_board,
      uint32_t now_ms,
      uint32_t preamble_ms,
      uint32_t max_payload_ms,
      bool& should_clear_irq) {
    should_clear_irq = false;
    if (polled_board) {
      return (irq & (IRQ_PREAMBLE_DETECTED | IRQ_HEADER_VALID)) != 0;
    }
    if (has_rx_done || (irq & IRQ_RX_DONE) != 0) {
      _activity_at = 0;
      _header_seen = false;
      return false;
    }
    const bool has_preamble = (irq & IRQ_PREAMBLE_DETECTED) != 0;
    const bool has_header = (irq & IRQ_HEADER_VALID) != 0;
    if (!has_preamble && !has_header) {
      _activity_at = 0;
      _header_seen = false;
      return false;
    }
    if (_activity_at == 0) {
      _activity_at = (now_ms == 0) ? 1U : now_ms;
      _header_seen = has_header;
      return true;
    }
    if (has_header && !_header_seen) {
      _header_seen = true;
    }
    const uint32_t timeout_ms = _header_seen ? (preamble_ms + max_payload_ms) : preamble_ms;
    if ((uint32_t)(now_ms - _activity_at) > timeout_ms) {
      should_clear_irq = true;
      _activity_at = 0;
      _header_seen = false;
      return false;
    }
    return true;
  }
};

}  // namespace sensecap
