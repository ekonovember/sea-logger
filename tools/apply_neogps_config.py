"""PlatformIO pre-build script: configure NeoGPS for this tracker.

NeoGPS is configured by editing #define lines in headers that live inside the
library (`NMEAGPS_cfg.h`, `GPSfix_cfg.h`). Doing that by hand is lost on every
clean build, so this script applies the changes to the downloaded library
before compilation. It is idempotent and fails the build loudly if a line it
expects is missing (i.e. a NeoGPS update changed the headers).

What is enabled and why:
  * GST sentence -> per-axis position error, the tracker's accuracy gate
  * GLL/GSA/GSV/VTG/ZDA parsed, GLL is the last sentence of the u-blox
    NEO-6M's 1 Hz burst, so a fix is complete when GLL arrives
  * lat/lon/heading error fields in gps_fix
"""

import re
from pathlib import Path

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

ENABLE = {
    "NMEAGPS_cfg.h": [f"NMEAGPS_PARSE_{s}" for s in ("GLL", "GSA", "GSV", "GST", "VTG", "ZDA")],
    "GPSfix_cfg.h": ["GPS_FIX_LAT_ERR", "GPS_FIX_LON_ERR", "GPS_FIX_HDG_ERR"],
}
LAST_SENTENCE = ("NMEAGPS::NMEA_RMC", "NMEAGPS::NMEA_GLL")


def configure(src: Path) -> None:
    for header, macros in ENABLE.items():
        path = src / header
        text = path.read_text()
        for macro in macros:
            pattern = re.compile(rf"^\s*(//\s*)?#define {macro}\b.*$", re.MULTILINE)
            if not pattern.search(text):
                raise RuntimeError(f"{header}: no #define {macro} line to enable")
            text = pattern.sub(f"#define {macro}", text, count=1)
        if header == "NMEAGPS_cfg.h":
            old, new = LAST_SENTENCE
            text = text.replace(f"#define LAST_SENTENCE_IN_INTERVAL {old}",
                                f"#define LAST_SENTENCE_IN_INTERVAL {new}")
            if f"#define LAST_SENTENCE_IN_INTERVAL {new}" not in text:
                raise RuntimeError("NMEAGPS_cfg.h: cannot set LAST_SENTENCE_IN_INTERVAL")
        if text != path.read_text():
            path.write_text(text)
            print(f"NeoGPS: configured {header}")


libdeps = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")  # noqa: F821
neogps = libdeps / "NeoGPS" / "src"
if neogps.is_dir():
    configure(neogps)
else:
    raise RuntimeError(f"NeoGPS not found in {libdeps}; run `pio pkg install` first")
