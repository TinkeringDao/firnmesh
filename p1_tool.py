#!/usr/bin/env python3
"""
Seeed Studio SenseCAP Solar Node P1-Pro (XIAO nRF52840 + SX1262 + L76K)
MeshCore Repeater CLI & Management Tool.

Communicates directly with the native USB CDC serial interface (/dev/cu.usbmodem*)
using Python's standard library (no external dependencies required).

Supported subcommands:
  1. `flash`     - Flash Unleashed firmware (.uf2 if bootloader mounted, or .zip via 1200bps serial DFU)
  2. `status`    - Query live Repeater configuration, RF stats, packet stats, pwrmgt, ACL, & neighbors
  3. `configure` - Apply optimal EU-868 Narrow (EU/UK Narrow 869.618 MHz CR8) Repeater config
  4. `cli`       - Run one or more raw MeshCore Repeater CLI commands
  5. `advert`    - Broadcast an immediate Repeater advertisement
  6. `neighbors` - Show discovered neighbor repeaters and signal levels
"""

import argparse
import fcntl
import glob
import os
import select
import shutil
import subprocess
import sys
import termios
import time
import tty

ADMIN_PUBLIC_KEY = os.environ.get("MESHCORE_ADMIN_PUBKEY", "")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_UF2_PATH = os.path.join(
    SCRIPT_DIR, "firmware", "SenseCap_Solar_repeater_unleashed.uf2"
)
DEFAULT_ZIP_PATH = os.path.join(
    SCRIPT_DIR, "firmware", "SenseCap_Solar_repeater_unleashed.zip"
)
NRFUTIL_BIN = shutil.which("adafruit-nrfutil") or os.path.expanduser(
    "~/.platformio/packages/tool-adafruit-nrfutil/bin/adafruit-nrfutil"
)


def find_serial_port(explicit_port: str | None = None) -> str:
    if explicit_port:
        return explicit_port
    candidates = sorted(glob.glob("/dev/cu.usbmodem*"))
    if not candidates:
        raise RuntimeError(
            "No /dev/cu.usbmodem* device found. Ensure the SenseCAP P1-Pro is connected via USB-C."
        )
    return candidates[0]


class P1ProRepeater:
    def __init__(self, port: str | None = None):
        self.port = find_serial_port(port)
        self.fd = os.open(self.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        tty.setraw(self.fd)
        attrs = termios.tcgetattr(self.fd)
        attrs[4] = attrs[5] = termios.B115200
        attrs[2] |= termios.CLOCAL | termios.CREAD
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        flags = fcntl.fcntl(self.fd, fcntl.F_GETFL)
        fcntl.fcntl(self.fd, fcntl.F_SETFL, flags & ~os.O_NONBLOCK)
        self.drain()

    def close(self):
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def drain(self):
        while select.select([self.fd], [], [], 0.04)[0]:
            os.read(self.fd, 4096)

    def cmd(self, command: str, wait_s: float = 0.45) -> str:
        self.drain()
        os.write(self.fd, (command + "\r\n").encode("utf-8"))
        end = time.time() + wait_s
        out = b""
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.08)
            if r:
                out += os.read(self.fd, 4096)
        text = out.decode("utf-8", errors="replace")
        lines = [line.strip() for line in text.splitlines() if line.strip()]
        if lines and lines[0] == command.strip():
            lines = lines[1:]
        return "\n".join(lines)


def cmd_status(dev: P1ProRepeater):
    dev.cmd(f"time {int(round(time.time()))}", wait_s=0.35)
    queries = [
        ("Board", "board"),
        ("Version", "ver"),
        ("Role", "get role"),
        ("Node Name", "get name"),
        ("Public Key", "get public.key"),
        ("Radio (Freq,BW,SF,CR)", "get radio"),
        ("TX Power (dBm)", "get tx"),
        ("RX Gain Boost", "get radio.rxgain"),
        ("AGC Reset (sec)", "get agc.reset.interval"),
        ("Hardware CAD", "get cad"),
        ("Repeat Enabled", "get repeat"),
        ("Loop Detect", "get loop.detect"),
        ("Multi ACKs", "get multi.acks"),
        ("Flood TX Delay", "get txdelay"),
        ("Direct TX Delay", "get direct.txdelay"),
        ("Zero-Hop Advert (min)", "get advert.interval"),
        ("Flood Advert (hours)", "get flood.advert.interval"),
        ("Latitude", "get lat"),
        ("Longitude", "get lon"),
        ("GPS Status", "gps"),
        ("GPS Advert Mode", "gps advert"),
        ("Owner Info", "get owner.info"),
        ("Clock (UTC)", "clock"),
        ("Default Flood Scope", "region default"),
        ("Home Region", "region home"),
        ("Region Table", "region"),
        ("Admin ACL", "get acl"),
        ("Power Management", "pwrmgt"),
        ("Silicon Diagnostics", "silicon"),
        ("Battery Diagnostics", "battdiag"),
        ("RF Diagnostics", "rfdiag"),
        ("Core Stats", "stats-core"),
        ("Radio Stats", "stats-radio"),
        ("Packet Stats", "stats-packets"),
        ("Neighbors", "neighbors"),
    ]
    print(f"=== SenseCAP Solar Node P1-Pro ({dev.port}) ===")
    for label, q in queries:
        resp = dev.cmd(q, wait_s=0.4)
        if "\n" in resp:
            print(f"{label:24s}:")
            for line in resp.splitlines():
                print(f"  {line}")
        else:
            print(f"{label:24s}: {resp}")


def cmd_configure(dev: P1ProRepeater, args):
    epoch = int(time.time())
    commands = [
        f"time {epoch}",
        f"set name {args.name}",
        f"set radio {args.freq},{args.bw},{args.sf},{args.cr}",
        f"set tx {args.tx}",
        "set radio.rxgain on",
        "set agc.reset.interval 4",
        "set cad on",
        "set flood.max.advert 16",
        "set repeat on",
        "set loop.detect minimal",
        "set multi.acks 1",
        "set txdelay 0.5",
        "set direct.txdelay 0.2",
        "set flood.advert.interval 12",
        "set advert.interval 120",
        f"set lat {args.lat}",
        f"set lon {args.lon}",
        "gps off",
        "gps advert prefs",
        f"set owner.info {args.name} | sub-zero field site | P1-Pro + MikroTik 6.5dBi",
    ]
    if args.admin_pubkey:
        commands.append(f"setperm {args.admin_pubkey} 3")
    commands.append("region allowf *")
    if args.region:
        commands.extend(
            [
                f"region put {args.region}",
                f"region allowf {args.region}",
                f"region default {args.region}",
            ]
        )
        if args.home_region:
            commands.extend(
                [
                    f"region put {args.home_region} {args.region}",
                    f"region allowf {args.home_region}",
                    f"region home {args.home_region}",
                ]
            )
    commands.append("region save")
    print(f"=== Applying Optimal Repeater Configuration on {dev.port} ===")
    for c in commands:
        resp = dev.cmd(c, wait_s=0.45)
        print(f"  {c:65s} -> {resp}")


def cmd_flash(explicit_port: str | None = None, uf2_path: str = DEFAULT_UF2_PATH, zip_path: str = DEFAULT_ZIP_PATH):
    uf2_mounts = [
        v for v in ("/Volumes/XIAO-SENSE", "/Volumes/SENSECAP", "/Volumes/NRF52BOOT") if os.path.isdir(v)
    ]
    if uf2_mounts:
        target_vol = uf2_mounts[0]
        print(f"=== Flashing UF2 image {uf2_path} -> {target_vol} ===")
        shutil.copy2(uf2_path, os.path.join(target_vol, os.path.basename(uf2_path)))
        print("UF2 copy complete. Waiting 4s for node reboot...")
        time.sleep(4.0)
        return

    port = find_serial_port(explicit_port)
    print(f"=== Flashing DFU package {zip_path} via {port} (1200bps auto-touch) ===")
    cmd = [
        NRFUTIL_BIN if os.path.exists(NRFUTIL_BIN) else "adafruit-nrfutil",
        "--verbose",
        "dfu",
        "serial",
        "--package",
        zip_path,
        "-p",
        port,
        "-b",
        "115200",
        "--singlebank",
        "--touch",
        "1200",
    ]
    subprocess.run(cmd, check=True)
    print("DFU flash complete. Waiting 4s for node reboot...")
    time.sleep(4.0)


def main():
    parser = argparse.ArgumentParser(
        description="SenseCAP Solar Node P1-Pro MeshCore Repeater Utility"
    )
    parser.add_argument("-p", "--port", help="Explicit serial port (/dev/cu.usbmodem*)")
    sub = parser.add_subparsers(dest="subcommand", required=True)

    p_flash = sub.add_parser("flash", help="Flash Unleashed Repeater firmware via UF2 or 1200bps serial DFU")
    p_flash.add_argument("--uf2", default=DEFAULT_UF2_PATH, help="Path to .uf2 firmware")
    p_flash.add_argument("--zip", default=DEFAULT_ZIP_PATH, help="Path to .zip DFU package")

    sub.add_parser("status", help="Show full Repeater status")

    p_cfg = sub.add_parser("configure", help="Configure Repeater with optimal EU-868 Narrow settings")
    p_cfg.add_argument("--name", default="MeshCore-Repeater", help="Repeater node name (default: MeshCore-Repeater)")
    p_cfg.add_argument("--freq", default="869.618", help="Frequency in MHz (default: 869.618)")
    p_cfg.add_argument("--bw", default="62.5", help="Bandwidth in kHz (default: 62.5)")
    p_cfg.add_argument("--sf", default="8", help="Spreading Factor (default: 8)")
    p_cfg.add_argument("--cr", default="8", help="Coding Rate (default: 8)")
    p_cfg.add_argument("--tx", default="22", help="TX power in dBm (default: 22)")
    p_cfg.add_argument("--lat", default="0.000000", help="Latitude (default: 0.000000)")
    p_cfg.add_argument("--lon", default="0.000000", help="Longitude (default: 0.000000)")
    p_cfg.add_argument("--region", default="", help="Optional default flood region tag (e.g. eu-central)")
    p_cfg.add_argument("--home-region", default="", help="Optional child home region tag")
    p_cfg.add_argument(
        "--admin-pubkey",
        default=ADMIN_PUBLIC_KEY,
        help="Optional admin Ed25519 public key (or set MESHCORE_ADMIN_PUBKEY)",
    )

    p_cli = sub.add_parser("cli", help="Execute raw CLI command(s)")
    p_cli.add_argument("commands", nargs="+", help="CLI command string(s)")

    sub.add_parser("advert", help="Broadcast immediate advertisement")
    sub.add_parser("neighbors", help="List discovered neighbors")

    args = parser.parse_args()
    if args.subcommand == "flash":
        cmd_flash(args.port, args.uf2, args.zip)
        return
    with P1ProRepeater(args.port) as dev:
        if args.subcommand == "status":
            cmd_status(dev)
        elif args.subcommand == "configure":
            cmd_configure(dev, args)
        elif args.subcommand == "cli":
            for c in args.commands:
                print(f"$ {c}")
                print(dev.cmd(c, wait_s=0.6))
        elif args.subcommand == "advert":
            dev.cmd(f"time {int(round(time.time()))}", wait_s=0.35)
            print(dev.cmd("advert", wait_s=0.6))
        elif args.subcommand == "neighbors":
            print(dev.cmd("neighbors", wait_s=0.6))


if __name__ == "__main__":
    main()
