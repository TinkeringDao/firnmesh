# firnmesh

[![CI](https://github.com/TinkeringDao/firnmesh/actions/workflows/ci.yml/badge.svg)](https://github.com/TinkeringDao/firnmesh/actions/workflows/ci.yml)
[![Security](https://github.com/TinkeringDao/firnmesh/actions/workflows/security.yml/badge.svg)](https://github.com/TinkeringDao/firnmesh/actions/workflows/security.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Sub-zero solar Li-Ion MPPT power management, `< 1 µA` QSPI sleep, C++17 host seam tests, and a zero-dependency USB serial/DFU utility (`p1_tool.py`) for **Seeed SenseCAP Solar Node P1-Pro** running as an unattended [MeshCore](https://github.com/meshcore-dev/MeshCore) LoRa repeater through continental winter frost (`-15 °C` nights and multi-week stratus fog).

Target hardware profile: `XIAO nRF52840 Plus` + `Wio-SX1262` (`+22 dBm`) + `XIAO L76K GNSS` + `P25Q16H` 2 MB QSPI flash + `CN3165` solar MPPT charger + `3350 mAh` 18650 NMC cell, tested on a rooftop mast with a `6.5 dBi` 868 MHz fiberglass omnidirectional antenna.

## What Stock `SenseCap_Solar_repeater` (`v1.18.0`) Gets Wrong on Hardware

Inspecting the stock `v1.18.0-81dbae5` UF2 binary and measuring live rail currents turned up several board-level bugs that drain a solar repeater in winter:

1. `SenseCapSolarBoard` inherited `NRF52Board` (LDO regulator) instead of `NRF52BoardDCDC`, omitted `-D NRF52_POWER_MANAGEMENT=1` (`0` `NRF_LPCOMP` references in the binary), left `NRF_WDT` disabled, and spun the CPU with `powersaving_enabled=0` (`3.6 mA` idle).
2. Unpowered `XIAO L76K` GNSS drew `1.8 mA` (`~43 mAh/day`) continuously via ESD protection diodes because `UART1 TX` (`P1.11`) stayed driven at `3.3 V` when `GPS_EN` went low, and calling `_location->stop()` before `Serial1.end()` wedged `NRF_UARTE0` on a `0 V` RX break condition (`S4-F7`).
3. The onboard `P25Q16H` 2 MB QSPI flash was never placed into Deep Power-Down (`0xB9`), while `PINS_COUNT=21` blocked `D21..D26`. Using the hardware `NRF_QSPI` peripheral triggers nRF52840 Errata 122 (`+1.2 mA` leak), so this port bit-bangs SPI Mode 0 `0xAB` + `0xB9` and disconnects `P0.20..24` (`< 1 µA`).
4. Below `3.30 V`, the board's `3.3 V` LDO enters dropout (`VDD ≈ VBAT`), locking the `1.00 MΩ / 510 kΩ` battery divider (`0.3378 × VBAT`) to a constant fraction of `VDD`. The lowest usable `LPCOMP` wake threshold (`REFSEL=2`, `3/8 VDD`) therefore requires **`3,664 mV` (`~42%` SoC)**, stranding a depleted repeater offline for 6 to 11 winter fog days (`0.15–0.45 PSH/day`).
5. On a rooftop `6.5 dBi` omni, `resetAGC()` zeroed `_noise_floor` (`0 dBm` stuck for 95 hours on stock firmware), recalibrated `SX1262` on `RC64k` instead of the `1.8 V` `DIO3` TCXO, and lacked a timeout for false LoRa preamble latches (`PREAMBLE_DETECTED` wedging TX for 4 s at a time).

### Summary of Subsystem Fixes

| Subsystem | Stock `v1.18.0` Behavior | Patched Behavior | Measured Result |
| :--- | :--- | :--- | :--- |
| **nRF52840 DCDC & Idle Sleep** | LDO mode, `powersaving_enabled=0`, no `LPCOMP` | `NRF52BoardDCDC` (`DCDCEN=1`), single `sd_app_evt_wait()`, `LPCOMP` `50 mV` hysteresis | Continuous repeater idle draw cut from `~10.6 mA` to `~5.2 mA` (`-130 mAh/day`) |
| **P25Q16H 2 MB QSPI Flash** | Uninitialized (`~35 µA` standby) | Bit-banged SPI `0xAB` + `0xB9` + `GPIO_PIN_CNF_INPUT_Disconnect` | `< 1 µA` QSPI sleep without nRF52840 Errata 122 |
| **SAADC Battery Sense (`1 MΩ / 510 kΩ`)** | `3.00f` / `1.476f` divider (`+43 mV` error), `10 µs` TACQ, `50 ms` blocking delay | Exact `1510/510` ratio, `40 µs` TACQ, `8×` oversampling, `10 °C` auto-offset cal, `1/4` EMA | `330 µs` non-blocking read (`151×` faster), `σ_ema = 0.77 mV` |
| **Sub-Zero Winter Hibernate** | Single-stage `LPCOMP` wake at `3,664 mV` (`42%` SoC) | Cold-compensated cutoff (`3,050 mV` `< 0 °C` across 3×30 s checks) + Stage-1 `~3 µA` `SYSTEMON` `0.6 V`-bandgap poll waking at **`3,250 mV` (`12%` SoC)** | Immune to `-15 °C` `+22 dBm` TX IR sag; `5.5×` faster wake under winter stratus fog |
| **Wio-SX1262 RF Front-End** | `rxg=0x94`, `clmp=0xE0`, `RC64k` post-reset cal, `0 dBm` stuck floor | `DIO3` `1.8 V` TCXO + `Calibrate(0x7F)` + `CalibrateImage`, `RxGain=0x96`, `OCP=140 mA`, `Errata 15.2` (`0xFE`), 8-sample un-gated floor seed, preamble timeout | Noise floor converges to `-117 dBm` (`-127.5 dBm` quiet battery RSSI), `0` TX clamp faults |
| **Mesh Dedup & Routing Tables** | `160` packet hashes, `16` region keys (no eviction), `50` neighbors | `512` packet hashes, `64`-slot FIFO `TransportKeyStore` cache, `128` neighbors | Zero routing-loop storms or AES key recalculation thrash under wide-area traffic |
| **L76K GNSS & Watchdog** | `1.8 mA` UART back-powering, `Uart::end()` break hang, no `NRF_WDT` | `PSEL.RXD` disconnect before `Serial1.end()` + tri-state pins, `$PCAS04,7` multi-GNSS, `60 s` `NRF_WDT` | `0.000 mA` GNSS off-state leak and automatic recovery from bus hangs |

## 7-Day Rooftop Mast Telemetry (`166.85 h` Continuous Uptime)

Full checkpoints are archived in [`docs/sensecap_p1_pro_baseline_and_checkpoint_20261105.json`](docs/sensecap_p1_pro_baseline_and_checkpoint_20261105.json).

| Metric | Stock `v1.18.0` (`95.31 h`) | Day-1 (`4.58 h`) | Day-2 (`19.98 h`, Post-Frost) | Day-7 (`166.85 h` / `6.95 d`) |
| :--- | :--- | :--- | :--- | :--- |
| **Antenna & Flood Scope** | `2 dBi` dipole, untagged | `6.5 dBi` omni, `eu-region` | `6.5 dBi` omni, `eu-region` | `6.5 dBi` omni, `eu-region` |
| **Battery Voltage (`EMA` / `raw`)** | `4,120 mV` (uncalibrated) | `4,063` / `4,065 mV` | `4,059` / `4,061 mV` | `4,041` / `4,040 mV` (`-3.2 mV/day` net) |
| **16-Sample SAADC Burst (`us16`)** | `50,000 µs` (single sample) | `4,883 µs` (`305 µs/read`) | `5,860 µs` (`366 µs/read`) | `5,860 µs` (`366 µs/read`) |
| **Die Temp / Last SAADC Cal** | Unmonitored | `7.25 °C` (cal `15.8 °C`) | `2.25 °C` (re-cal `5.8 °C`) | `13.25 °C` (cal `5.8 °C`) |
| **Peripheral Gate (`dcdc`/`qspi`/`gps`)** | `0` / `ON` / `ON` | `1` / `0x2` / `0x2` | `1` / `0x2` / `0x2` | `1` / `0x2` / `0x2` |
| **Noise Floor / Link SNR** | `0 dBm` (broken) / `+9.0 dB` | `-117 dBm` / `+13.75 dB` | `-117 dBm` / `+14.25 dB` | `-117 dBm` / `+16.00 dB` |
| **Decoded RX Rate** | `21.2 pkt/h` (`2,016`) | `132.1 pkt/h` (`605`) | `92.5 pkt/h` (`1,848`) | `101.5 pkt/h` (`16,928`) |
| **Forwarded TX Rate (`TX Errs`)** | `15.2 pkt/h` (`1,445`, `0` err) | `120.3 pkt/h` (`551`, `0` err) | `81.4 pkt/h` (`1,627`, `0` err) | `82.4 pkt/h` (`13,754`, `0` err) |
| **Discovered Nodes (`>50 km` / `>100 km`)** | `29` (`6` / `5`, max `190.5 km`) | `66` (`19` / `13`) | `78` (`28` / `19`, max `354.7 km`) | `112` (`48` / `35`, max `354.7 km`) |

## Limitations & Trade-Offs

- **No prebuilt binaries in Git:** Compiled `.uf2`, `.zip`, `.hex`, and `.elf` images are excluded from version control so you build from auditable source with your own regional frequency and duty-cycle settings.
- **Hardware solar charge cutoff:** Firmware manages *discharge* load-shedding (`3,450 mV` GPS shed, `3,050 mV` Stage-1 hibernate). Sub-zero *charge* inhibition below `0 °C` depends on the onboard `CN3165` MPPT charger IC and the 18650 pack's NTC thermistor/PCM; verify NTC continuity before winter deployment.
- **Stage-1 vs. Stage-2 sleep floor:** Stage-1 `SYSTEMON` bandgap polling draws `~3 µA` (vs. `< 1 µA` in Stage-2 `SYSTEMOFF`) in exchange for waking at `3,250 mV` (`12%` SoC) instead of waiting for `3,664 mV` (`42%` SoC). If the cell drops below `3,050 mV`, the node falls back to Stage-2 `LPCOMP` protection.

## Building, Testing & Flashing

Run the standalone C++17 host seam test suite (`9/9` tests, no external dependencies):

```bash
c++ -std=c++17 -Wall -Wextra -I test -I test/host_mocks -I variants/sensecap_solar \
  test/test_sensecap_solar_seams/test_sensecap_solar_seams.cpp -o /tmp/seam_tests && /tmp/seam_tests
```

Apply the patch to an upstream [MeshCore](https://github.com/meshcore-dev/MeshCore) checkout and build with PlatformIO:

```bash
git clone https://github.com/TinkeringDao/firnmesh.git
git clone https://github.com/meshcore-dev/MeshCore.git
cd MeshCore
git apply ../firnmesh/patches/sensecap_solar_p1_pro_unleashed.patch
pio run -e SenseCap_Solar_repeater
```

Flash via USB-C UF2 drag-and-drop (double-tap Reset to mount `XIAO-SENSE`) or via 1200 bps serial DFU using `p1_tool.py`:

```bash
python3 p1_tool.py flash --uf2 /path/to/firmware.uf2
python3 p1_tool.py status
python3 p1_tool.py configure --name MeshCore-Repeater --freq 869.618 --bw 62.5 --sf 8 --cr 8 --tx 22
```

## Repository Layout

- [`variants/sensecap_solar/`](variants/sensecap_solar/): nRF52840 board support, pure-function seam header [`SenseCapSolarSeams.h`](variants/sensecap_solar/SenseCapSolarSeams.h), and `platformio.ini`.
- [`patches/sensecap_solar_p1_pro_unleashed.patch`](patches/sensecap_solar_p1_pro_unleashed.patch): unified patch against upstream `MeshCore`.
- [`p1_tool.py`](p1_tool.py): Python 3 standard-library CLI for UF2/DFU flashing, serial status queries, and radio configuration.
- [`test/`](test/): host mocks and C++17 unit tests covering cold cutoffs, SAADC calibration, preamble timeouts, and key-store eviction.
- [`docs/SENSECAP_P1_PRO_FIRMWARE_AUDIT.md`](docs/SENSECAP_P1_PRO_FIRMWARE_AUDIT.md): 36-point hardware-vs-firmware audit and disassembly notes.

## Component Support

Passive and interconnect components used in bench testing:

- [Coilcraft](https://www.coilcraft.com) (`XAL4020-222MEC` shielded power inductors), [Samtec](https://www.samtec.com) (`TSW-110-07-G-S` board-to-board headers), and [WAGO](https://www.wago.com) (`221 + Gelbox IPX8` outdoor splices and `2060` SMD PCB terminal blocks).

## License & Legal

Licensed under the [MIT License](LICENSE). Independent, non-commercial open-source project; product names identify compatible target hardware only.
Operators are solely responsible for local RF spectrum compliance (such as EU `869.400–869.650 MHz` SRD sub-band `g3` duty-cycle and ERP limits) and lithium-ion safety (see [`LEGAL.md`](LEGAL.md) and [`SECURITY.md`](SECURITY.md)).
