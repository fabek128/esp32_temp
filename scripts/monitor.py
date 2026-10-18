#!/usr/bin/env python3
"""Display live MAX6675 readings from the ESP32 serial console."""

from __future__ import annotations

import argparse
import glob
import re
import statistics
import sys
import time

import serial
from serial.tools import list_ports

TEMP_RE = re.compile(rb"^\[TEMP\]\s+([0-9]+(?:\.[0-9]+)?)\s+C\s*$")
BAUD = 115200


def find_port(explicit: str | None) -> str | None:
    if explicit:
        return explicit
    # Prefer macOS callout devices; list_ports may also contain tty aliases.
    candidates = sorted(glob.glob("/dev/cu.usbmodem*"))
    if not candidates:
        ports = list(list_ports.comports())
        candidates = [p.device for p in ports if "usbmodem" in p.device]
    return candidates[0] if len(candidates) == 1 else None


def run(port_arg: str | None) -> int:
    readings: list[float] = []
    port = None
    ser: serial.Serial | None = None
    last_temp: float | None = None
    last_update: float | None = None
    state = "DESCONECTADO"
    last_error = ""
    last_render = 0.0
    try:
        while True:
            if ser is None:
                port = find_port(port_arg)
                if port is None:
                    state = "BUSCANDO DISPOSITIVO"
                    last_error = "Puerto USB serie no identificado o hay varios candidatos"
                    time.sleep(2)
                    continue
                try:
                    ser = serial.Serial(port, BAUD, timeout=1)
                    state = "CONECTADO"
                    last_error = ""
                except (serial.SerialException, OSError) as exc:
                    state = "DESCONECTADO"
                    last_error = str(exc)
                    time.sleep(2)
                    continue
            try:
                line = ser.readline()
                if not line:
                    continue
                if line.startswith(b"[ERROR]"):
                    last_error = line.decode("ascii", errors="replace").strip()
                match = TEMP_RE.match(line.strip())
                if match:
                    last_temp = float(match.group(1))
                    readings.append(last_temp)
                    last_update = time.monotonic()
                    last_error = ""
                if line.startswith(b"[INIT]"):
                    state = "CONECTADO"
                if line.startswith(b"[TEMP]") or line.startswith(b"[ERROR]"):
                    render(port, state, last_temp, readings, last_update, last_error)
                    last_render = time.monotonic()
                elif ser is not None and time.monotonic() - last_render >= 1:
                    render(port, state, last_temp, readings, last_update, last_error)
                    last_render = time.monotonic()
            except (serial.SerialException, OSError) as exc:
                last_error = str(exc)
                state = "DESCONECTADO"
                try:
                    ser.close()
                except Exception:
                    pass
                ser = None
                time.sleep(2)
    except KeyboardInterrupt:
        print("\nMonitor finalizado.")
        return 0
    finally:
        if ser is not None:
            ser.close()


def render(port: str | None, state: str, current: float | None, values: list[float],
           updated: float | None, error: str) -> None:
    age = "—" if updated is None else f"hace {int(time.monotonic() - updated)} s"
    avg = statistics.fmean(values) if values else None
    def fmt(value: float | None) -> str:
        return "—" if value is None else f"{value:.2f} °C"
    print("\033[2J\033[H", end="")
    print("ESP32-S3 TEMPERATURE MONITOR\n")
    print(f"Puerto: {port or '—'}")
    print(f"Sensor: MAX6675\nEstado: {state}\n")
    print(f"Temperatura actual:  {fmt(current)}")
    print(f"Temperatura mínima:  {fmt(min(values) if values else None)}")
    print(f"Temperatura máxima:  {fmt(max(values) if values else None)}")
    print(f"Promedio:            {fmt(avg)}\n")
    print(f"Lecturas recibidas:  {len(values)}")
    print(f"Última actualización: {age}")
    if error:
        print(f"Último error: {error}")
    sys.stdout.flush()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="Puerto serie explícito; por defecto se detecta automáticamente")
    args = parser.parse_args()
    return run(args.port)


if __name__ == "__main__":
    raise SystemExit(main())
