# Legal, Trademark, Interoperability, & RF Regulatory Notice

## 1. Independent Open-Source Project & Non-Affiliation (Nominative Fair Use)

**`firnmesh` — Unofficial Interoperability & Repeater Firmware Suite for Seeed SenseCAP Solar Node P1-Pro** is an **independent, non-commercial, community-developed free and open-source software and hardware documentation project** created solely to improve power management, sub-zero battery protection, peripheral sleep states, and LoRa repeater interoperability on user-owned hardware.

- **No Affiliation or Endorsement:** This project is **not affiliated with, authorized by, sponsored by, or endorsed by Seeed Technology Co., Ltd. (`Seeed Studio`), Nordic Semiconductor ASA, Semtech Corporation, MikroTikls SIA (`MikroTik`), Quectel Wireless Solutions Co., Ltd., Consonance Electronics (`CN3165`), Puya Semiconductor (`P25Q16H`), or Ripple Radios / MeshCore.**
- **Indication of Intended Purpose & Nominative Fair Use:**
  - Pursuant to **Article 14(1)(c) of Regulation (EU) 2017/1001 (EUTMR)**, settled Court of Justice of the European Union jurisprudence (*Case C-228/03 — Gillette v. LA-Laboratories*; *Case C-63/97 — BMW v. Deenik*), and **US Nominative Fair Use (`15 U.S.C. §§ 1115(b)(4), 1125`**, *New Kids on the Block v. News America Publishing, Inc.*, 971 F.2d 302 (9th Cir. 1992)), third-party product names and trademarks are referenced strictly as a necessary indication of intended purpose and target hardware compatibility of this firmware and diagnostic suite:
    - "Seeed", "Seeed Studio", "SenseCAP", "XIAO", and "Wio" are trademarks or registered trademarks of **Seeed Technology Co., Ltd.**
    - "Nordic Semiconductor" and "nRF52840" are trademarks or registered trademarks of **Nordic Semiconductor ASA**.
    - "Semtech", "LoRa", and "SX1262" are trademarks or registered trademarks of **Semtech Corporation**.
    - "MikroTik" and "SXTsq" are trademarks or registered trademarks of **MikroTikls SIA**.
    - "MeshCore" and "Ripple Radios" are trademarks of their respective creators (**Scott Powell / Ripple Radios**).
  - Zero corporate logos, proprietary trade dress, or OEM marketing assets are included in this repository.

---

## 2. Open-Source License Provenance, Statutory Interoperability, & Hardware Reverse-Engineering Safe Harbor

### 2.1 Open-Source License Provenance (`MIT License`, `SPDX-License-Identifier: MIT`, Zero `<REDACTED_OEM_BYTES>`)
- The upstream **MeshCore** repeater codebase and Seeed `sensecap_solar` board support files are open-source software licensed under the **MIT License** (`Copyright (c) 2024-2026 Scott Powell / Ripple Radios and MeshCore contributors`).
- All patches, pure-function TDD seam headers ([`SenseCapSolarSeams.h`](variants/sensecap_solar/SenseCapSolarSeams.h)), board support optimizations ([`SenseCapSolarBoard.cpp`](variants/sensecap_solar/SenseCapSolarBoard.cpp)), diagnostic scripts ([`p1_tool.py`](p1_tool.py)), and hardware audit documentation in this repository are original derivative and companion works distributed under the [MIT License](LICENSE) (`SPDX-License-Identifier: MIT`, `Copyright (c) 2026 TinkeringDao`), containing zero proprietary OEM binaries or verbatim firmware hex slices (`<REDACTED_OEM_BYTES>`).

### 2.2 European Union (`Directive 2009/24/EC` & `Directive (EU) 2016/943`)
1. **Exclusion of Functional Interfaces & Hardware Register Maps from Copyright (`Article 1(2) of Directive 2009/24/EC`):**
   - Under `Article 1(2) of Directive 2009/24/EC` and settled Court of Justice of the European Union precedent (*SAS Institute Inc. v. World Programming Ltd*, Case `C-406/10`, paras. 39–46), ideas, principles, GPIO pin assignments, peripheral register offsets (`NRF_SAADC`, `NRF_LPCOMP`, `NRF_WDT`, `SX1262` SPI opcodes), and serial diagnostic framing are **excluded from copyright protection**.
2. **Right to Observe, Study, and Test (`Article 5(3) of Directive 2009/24/EC`):**
   - Permits the lawful user of a computer program and hardware device to observe, study, or test its functioning without prior authorization in order to determine the underlying ideas, power states, and peripheral behavior.
3. **Mandatory Right of Interoperability (`Article 6 of Directive 2009/24/EC`):**
   - Permits independent analysis necessary to achieve the **interoperability** of independently created firmware and host diagnostic tools with the `SenseCAP Solar Node P1-Pro` hardware components (`nRF52840`, `Wio-SX1262`, `XIAO L76K`, `P25Q16H`, `CN3165`).
4. **Statutory Nullity of Contrary EULA Provisions (`Article 8 of Directive 2009/24/EC`):**
   - Under mandatory European Union statutory law, any contractual clauses purporting to restrict or waive rights under `Articles 5(3) or 6 of Directive 2009/24/EC` are **statutorily null and void**.
5. **Lawful Hardware Reverse Engineering (`Article 3(1)(b) of Directive (EU) 2016/943`):**
   - `Article 3(1)(b) of Directive (EU) 2016/943` explicitly authorizes obtaining technical information by observing, examining, disassembling, or testing a product (`SenseCAP Solar Node P1-Pro`) that has been made publicly available on the market or is in the lawful possession of the observer, free from any contractual restriction, to extract GPIO pinouts, voltage divider ratios (`1.00 MΩ / 510 kΩ`), and peripheral sleep characteristics.

### 2.3 United States (`17 U.S.C. §§ 102(b), 107, 1201(f)`, `18 U.S.C. § 1839(6)(B) (DTSA)`)
1. **Uncopyrightable Systems, Hardware Pinouts, & Methods of Operation (`17 U.S.C. § 102(b)`):**
   - Microcontroller register addresses, SPI command opcodes, and hardware initialization sequences required for peripheral operation constitute uncopyrightable functional facts and methods of operation (*Baker v. Selden*, 101 U.S. 99 (1879); *Google LLC v. Oracle America, Inc.*, 593 U.S. 1 (2021); *Lexmark Int'l, Inc. v. Static Control Components, Inc.*, 387 F.3d 522 (6th Cir. 2004)).
2. **Interoperability Exemptions (`17 U.S.C. § 1201(f)` & `17 U.S.C. § 107`):**
   - Developed exclusively for achieving hardware-software interoperability on lawfully owned embedded devices (*Sega Enterprises Ltd. v. Accolade, Inc.*, 977 F.2d 1510 (9th Cir. 1992); *Sony Computer Entertainment, Inc. v. Connectix Corp.*, 203 F.3d 596 (9th Cir. 2000)).
3. **Defend Trade Secrets Act Reverse-Engineering Safe Harbor (`18 U.S.C. § 1839(6)(B) (DTSA)`):**
   - Under federal law, reverse engineering, independent derivation, and board-level electrical observation of a lawfully acquired product on the open market are explicitly excluded from the definition of "improper means".

---

## 3. Non-Circumvention of Technological Protection Measures (`Directive 2009/24/EC` / `17 U.S.C. §§ 1201(a), 1201(f)`)

Under **`Directive 2009/24/EC`** and **US `17 U.S.C. §§ 1201(a), 1201(f)`**, this project **does not circumvent, bypass, decrypt, or defeat any Technological Protection Measure (TPM)**:

- **Open Developer Hardware Interface (UF2 / CDC DFU / SWD):** The Seeed Studio `SenseCAP Solar Node P1-Pro` (`XIAO nRF52840 Plus`) ships from the factory with the open-source **Adafruit nRF52 UF2 Bootloader** (`0.9.2`, USB mass-storage drag-and-drop and `adafruit-nrfutil` serial DFU) and exposed SWD debug pads expressly intended for user firmware installation.
- **Zero Cryptographic Key Extraction or Security Bypass:** No readback protection (`APPROTECT`), cryptographic signature verification, encrypted bootloader, or proprietary access control mechanism is bypassed or defeated.

---

## 4. Authorized Local Owner Execution Only & US CFAA Compliance (`18 U.S.C. § 1030 (CFAA)`)

In accordance with the **Authorized Local Owner Execution Only** principle and the US Computer Fraud and Abuse Act (`18 U.S.C. § 1030 (CFAA)`, *Van Buren v. United States*, 593 U.S. 374 (2021)):

- **Authorized Local Owner Execution Only:** The firmware images (`.uf2`, `.zip`, `.hex`, `.elf`) and USB serial diagnostic tool ([`p1_tool.py`](p1_tool.py)) are designed strictly for installation and execution on `SenseCAP Solar Node P1-Pro` hardware owned or explicitly authorized by the operator.
- **No Unauthorized Access or Interception:** This software does not bypass any access control mechanism, does not interact with any third-party cloud infrastructure, and operates solely on local USB serial interfaces and standard open MeshCore repeater packets. Flashing or probing third-party hardware without the owner's authorization is strictly prohibited.

---

## 5. Non-Commercial Open-Source Software Exemption (EU CRA `Regulation (EU) 2024/2847` & EU PLD `Directive (EU) 2024/2853`)

- **EU Cyber Resilience Act Exemption (`Regulation (EU) 2024/2847`, Recitals 15 & 18; `Art. 2(5)`):** This repository is developed, maintained, and supplied strictly as **non-commercial free and open-source software outside the course of any commercial activity**. No fee is charged, no paid support or commercial services are offered, and no personal data is collected or monetized. Accordingly, the authors are neither "manufacturers" (`Art. 3(13)`) nor "open-source software stewards" (`Art. 3(14)`), and CE-marking (`Art. 28`), conformity assessment, and commercial manufacturer obligations under Regulation (EU) 2024/2847 do not apply.
- **EU Product Liability Directive Exemption (`Directive (EU) 2024/2853`, `Art. 2(2)` & Recitals 14, 15, 24):** Pursuant to **Article 2(2)** and **Recitals 14–15 of Directive (EU) 2024/2853**, strict liability under this Directive does not apply to free and open-source software developed or supplied outside the course of a commercial activity. Any person or entity that subsequently integrates, bundles, or supplies this firmware in the course of a commercial activity assumes sole legal and regulatory responsibility as a manufacturer.

---

## 6. Radio Frequency (RF), Duty Cycle, & Spectrum Regulatory Compliance (`Directive 2014/53/EU (RED)` / `ETSI EN 300 220-1/-2` & `EN 300 328` / `ERC/REC 70-03` / `Decision (EU) 2017/1483` / `FCC 47 C.F.R. Part 15.247 / Part 97`)

Installing custom firmware or attaching an external antenna to a radio transceiver alters its radiated RF characteristics. **The hardware owner and radio operator are solely and exclusively responsible for ensuring that their station configuration complies with all applicable national and regional spectrum regulations**, including **Directive 2014/53/EU (RED)**, **ETSI EN 300 220-1/-2** (Sub-GHz SRD), **ETSI EN 300 328** (2.4 GHz Wideband BLE on the `nRF52840` + `S140` SoftDevice), **ERC/REC 70-03**, **Decision (EU) 2017/1483**, and **FCC 47 C.F.R. Part 15.247 / Part 97**:

- **2.4 GHz BLE Radio (`ETSI EN 300 328` / `FCC 47 C.F.R. Part 15.247`):** The onboard `nRF52840` Bluetooth Low Energy subsystem operates within standard 2.4 GHz ISM limits using the unmodified Nordic `S140` SoftDevice stack and never modifies factory RF calibration tables.
- **High-Power SRD Sub-Band `g3` (`869.400–869.650 MHz`) vs. Sub-Band `g1` (`868.000–868.600 MHz`) in the European Union:**
  - The Seeed `Wio-SX1262` module is capable of up to **`+22 dBm` (`158 mW`)** conducted RF output power.
  - When paired with a passive gain antenna (e.g., `+5 dBi` to `+6.5 dBi` fiberglass omnidirectional collinear antenna minus coaxial pigtail/connector insertion loss), total **Effective Radiated Power (`ERP`)** approaches **`500 mW` (`+27 dBm` ERP / `+29.15 dBm` EIRP)**.
  - Under **ETSI EN 300 220-1/-2**, **ERC/REC 70-03 Annex 1**, and **Decision (EU) 2017/1483**, operation up to **`500 mW` (`+27 dBm`) ERP** without an individual license in the European Union is permitted **exclusively within the `869.400–869.650 MHz` Narrowband/Wideband SRD Sub-band `g3`** (e.g., `869.525 MHz` or `869.618 MHz` with bandwidth fully contained within `869.400–869.650 MHz`) at a strict maximum **duty cycle of `<= 10%` (`<= 360 seconds per hour`)** or equivalent LBT+AFA.
  - By contrast, operation in the **`868.000–868.600 MHz` Sub-band `g1`** is strictly limited to **`<= 25 mW` (`+14 dBm`) ERP** and **`<= 1%` duty cycle (`<= 36 seconds per hour`)**. Operating at `+22 dBm` conducted output or with a high-gain external antenna in `868.0–868.6 MHz` violates EU spectrum limits unless transmit power is attenuated to keep total ERP `<= +14 dBm`.
- **Amateur Radio Service Restrictions (`FCC 47 C.F.R. Part 97`):** Where operated under an amateur radio license rather than license-exempt SRD/ISM rules (`ERC/REC 70-03` / `Decision (EU) 2017/1483` / `FCC 47 C.F.R. Part 15.247`), operators must observe station identification rules and restrictions on encrypted or obscured payloads (`47 C.F.R. § 97.113(a)(4)`).
- **Operator Responsibility:** Before transmitting, you must configure the center frequency, bandwidth, transmit power (`txpower`), and airtime/duty-cycle limits via the MeshCore CLI to match your jurisdiction's authorized SRD/ISM band plan and your specific antenna/feedline gain budget.

---

## 7. AS IS Warranty Disclaimer, Lithium-Ion 18650 Battery, Solar MPPT Charging, & Mast Mounting Safety

THE SOFTWARE, FIRMWARE, SCHEMATIC NOTES, AND DOCUMENTATION ARE PROVIDED **"AS IS"**, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT (SEE [`LICENSE`](LICENSE)). Under this **AS IS Warranty Disclaimer**, any liability of the authors for negligence or consequential hardware/property damage is expressly excluded to the maximum extent permitted by applicable law:

1. **Lithium-Ion (`18650` NMC) Sub-Zero Charging Hazard (`0°C` to `+45°C` Charge Window):**
   - Charging lithium-ion cells below **`0°C` (`32°F`)** causes metallic **lithium plating** on the graphite anode, which can permanently degrade cell capacity, cause internal short circuits, and lead to thermal runaway, fire, or explosion.
   - While this firmware implements **discharge** load-shedding and two-stage sub-zero hibernation (`3250 mV` Stage-1 / `3050 mV` Stage-2), solar **charging** on the `SenseCAP Solar Node P1-Pro` is controlled in hardware by the onboard **`CN3165` Solar MPPT Charger IC** and the battery pack's NTC thermistor / PCM protection circuit. Always verify that your hardware's NTC thermal charge cutoff (`0°C..45°C`) and battery protection circuit are intact and functional before deploying in freezing climates.
2. **Outdoor Mast, Lightning, & Wind-Load Safety:**
   - Rooftop or mast installations of fiberglass antennas and solar nodes are subject to wind load, ice accumulation, water ingress, and lightning/electrostatic discharge (ESD). Operators are solely responsible for proper mechanical mounting, coaxial surge arrestors, earthing/grounding in accordance with local electrical codes (e.g., `IEC 62305`), and weatherproofing.

---

## Addendum: German Statutory References (Applicable to Residents of Germany)

For users, contributors, and maintainers residing or operating in Germany, the European Union directives and legal principles set forth above are implemented into German national law via the following corresponding statutory provisions:

- **Trademark Nominative Fair Use / Indication of Intended Purpose (`§ 23 Abs. 1 Nr. 3 MarkenG`, BGH `I ZR 6/10`):** Under Section 23(1) No. 3 of the German Trademark Act (*Markengesetz*), third-party trademarks and product designations may be used in the course of trade without the proprietor's consent where necessary as an indication of the intended purpose (*Bestimmungsangabe*) of accessories, spare parts, or interoperable software/firmware, provided such use complies with honest practices.
- **Software Copyright, Observation, Interoperability, & Non-Circumvention (`§§ 69a Abs. 2 & 5, 69d Abs. 3, 69e, 69f Abs. 2, 69g Abs. 2, 95a UrhG`):** Under the German Copyright Act (*Urheberrechtsgesetz*), ideas, principles, and functional interfaces underlying a computer program are excluded from copyright protection (`§ 69a Abs. 2 Satz 2 UrhG`). Lawful users are entitled to observe, study, and test program behavior (`§ 69d Abs. 3 UrhG`) and perform analysis necessary for interoperability (`§ 69e UrhG`); any contractual clauses purporting to waive or restrict these statutory rights are null and void (`§ 69g Abs. 2 UrhG` — *nichtig*). Computer programs are governed by the special provisions of `§§ 69a–69g UrhG` (`§ 69a Abs. 5`, `§ 69f Abs. 2 UrhG`) rather than `§ 95a UrhG`, and no technological protection measure is circumvented.
- **Hardware Reverse Engineering & Trade Secrets (`§ 3 Abs. 1 Nr. 2 lit. a & b GeschGehG`):** Section 3(1) No. 2 of the German Trade Secrets Act (*Geschäftsgeheimnisgesetz*) expressly permits obtaining technical information by observing, examining, disassembling, or testing a product or object that has been made publicly available (`lit. a`) or is in the lawful possession of the observer free from any valid duty to limit acquisition (`lit. b`).
- **IT Criminal Law & Property Ownership Scope (`§§ 202a–202c, 303a StGB`, `§ 903 BGB`, *BVerfG 2 BvR 2233/07*):** Under the German Criminal Code (*Strafgesetzbuch*) and Civil Code (*Bürgerliches Gesetzbuch*), all firmware and diagnostic utilities are intended strictly for execution by the lawful hardware owner (`§ 903 BGB`) on their own device data (*eigene Daten*, `§ 303a StGB`) and do not bypass any access security mechanism (*Zugangssicherung*, `§§ 202a–202c StGB`).
- **Radio Equipment & Spectrum Regulations (`FuAG`, `TKG`, `BNetzA Vfg 99/2022`, `AFuG` / `§ 16 Abs. 1 Nr. 4 AFuV`):** Within Germany, radio transceivers are regulated under the German Radio Equipment Act (*Funkanlagengesetz* — `FuAG`), the Telecommunications Act (*Telekommunikationsgesetz* — `TKG`), and the Federal Network Agency (*Bundesnetzagentur* — `BNetzA`) General Allocation for Short-Range Devices (`BNetzA Vfg 99/2022`), which permits up to `500 mW` (`+27 dBm`) ERP at `<= 10%` duty cycle exclusively in the `869.400–869.650 MHz` sub-band (`g3`) and limits `868.000–868.600 MHz` (`g1`) to `<= 25 mW` (`+14 dBm`) ERP at `<= 1%` duty cycle. Operation in amateur radio bands is subject to the Amateur Radio Act (`AFuG`) and Ordinance (`§ 16 Abs. 1 Nr. 4 AFuV`).
- **Gratuitous Open-Source Liability Standard (`§§ 521, 523, 524, 599, 600 BGB`):** Under the German Civil Code provisions governing the gratuitous provision of open-source software and documentation, statutory liability for simple negligence (*einfache Fahrlässigkeit*) and implied warranty defects is excluded.
