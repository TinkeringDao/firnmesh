# Security Policy

## Supported Hardware & Firmware Versions

| Product | Target MCU & Radio | Bootloader & SoftDevice | Firmware Version | Supported |
| :--- | :--- | :--- | :--- | :--- |
| Seeed Studio SenseCAP Solar Node P1-Pro | `XIAO nRF52840 Plus` + `Wio-SX1262` + `XIAO L76K` + `P25Q16H` | Adafruit UF2 `0.9.2` / `S140 v7.3.0` | `v1.18.0-unleashed` | Yes |

## Hardware, Flash, & RF Safety Boundaries

`firnmesh` operates strictly on user-owned `SenseCAP Solar Node P1-Pro` hardware via standard open Adafruit `nRF52` UF2 / CDC DFU interfaces:
- **Preserves Factory UF2 Bootloader & SoftDevice**: Application images link at `0x00026000` / `0x00027000` above Nordic SoftDevice `S140 v7.3.0` and leave the factory UF2 bootloader (`0x000F4000`) intact. You can revert to stock Seeed / MeshCore firmware at any time via double-tap Reset USB UF2 drag-and-drop.
- **Hardware Watchdog & Brownout Protection**: Enables the `60 s` hardware watchdog (`NRF_WDT`) and two-stage sub-zero hibernation (`3250 mV` Stage-1 bandgap poll / `3050 mV` Stage-2 `SYSTEMOFF` + `LPCOMP`) to protect the `18650` Li-Ion cell from deep over-discharge.
- **Authorized Local Owner Execution Only (`18 U.S.C. § 1030 (CFAA)`)**: [`p1_tool.py`](p1_tool.py) communicates exclusively with locally attached USB CDC serial devices owned by the operator.
- **RF & Duty Cycle Compliance (`Directive 2014/53/EU (RED)` / `ETSI EN 300 220-1/-2` / `ERC/REC 70-03` / `Decision (EU) 2017/1483` / `FCC 47 C.F.R. Part 15.247 / Part 97`)**: Operators must verify that configured center frequency, transmit power, antenna gain, and duty cycle comply with local spectrum regulations (see [`LEGAL.md`](LEGAL.md)).

## Reporting a Vulnerability

If you discover a memory safety issue, packet parser bug, or power state regression:
1. Open a private security advisory via [GitHub Security Advisories](https://github.com/TinkeringDao/firnmesh/security/advisories/new).
2. Include the firmware commit SHA, `silicon` / `rfdiag` / `battdiag` CLI output from `p1_tool.py`, and steps to reproduce.
