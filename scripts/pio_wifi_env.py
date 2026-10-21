"""Pass Wi-Fi credentials from environment/.env to the firmware build only."""

from __future__ import annotations

import os
from pathlib import Path

Import("env")  # noqa: F821 - PlatformIO/SCons build hook


def read_dotenv(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not path.exists():
        return values
    for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        key, separator, value = line.partition("=")
        key = key.strip()
        if not separator or not key or not key.replace("_", "").isalnum():
            raise ValueError(f"Formato inválido en {path.name}, línea {line_number}")
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
            value = value[1:-1]
        values[key] = value
    return values


project_dir = Path(env["PROJECT_DIR"])  # noqa: F821 - injected by PlatformIO/SCons
file_values = read_dotenv(project_dir / ".env")
ssid = os.environ.get("WIFI_SSID") or file_values.get("WIFI_SSID") or file_values.get("wifi_net")
password = (
    os.environ.get("WIFI_PASSWORD")
    or file_values.get("WIFI_PASSWORD")
    or file_values.get("wifi_pass")
)

if bool(ssid) != bool(password):
    raise ValueError("Configura juntas WIFI_SSID y WIFI_PASSWORD (o sus claves legacy wifi_net/wifi_pass)")

if ssid and password:
    env.Append(  # noqa: F821 - injected by PlatformIO/SCons
        CPPDEFINES=[
            ("WIFI_SSID", env.StringifyMacro(ssid)),
            ("WIFI_PASSWORD", env.StringifyMacro(password)),
        ]
    )
    print("Wi-Fi configurado para firmware (credenciales omitidas)")
else:
    print("Wi-Fi no configurado: firmware compilado sin conexión Wi-Fi")
