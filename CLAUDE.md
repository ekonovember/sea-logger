# CLAUDE.md — Sea Logger

Agent instructions for this repository (Claude Code reads this file automatically;
`AGENTS.md` points other tools here). Keep it short and true — update it in the same
change as the code it describes.

## What this is

Firmware for a GNSS track logger: NodeMCU v3 (ESP8266) + u-blox NEO-6M + SH1106 OLED
+ HW-125 SD card, built with PlatformIO (Arduino framework), plus a Python CSV → GPX tool.

## Layout and rules

| Path | Role | Rule |
|---|---|---|
| `lib/tracker_core/` | all logic and formatting | **No Arduino headers.** Time is passed in (`nowMs`), never read from `millis()`. Everything here must be unit-testable on the host. |
| `src/` | drivers (`gps_receiver`, `instrument_display`, `track_logger`) + `main.cpp` | Thin adapters only: translate library types to core types. No decisions here — if you need an `if` about policy, it belongs in the core with a test. |
| `include/config.h` | pins, thresholds, intervals | Every tunable is a `constexpr` here, nowhere else. |
| `tools/apply_neogps_config.py` | pre-build script | NeoGPS is configured by patching the downloaded library. Never ask the user to edit `.pio/libdeps` by hand. |
| `tools/csv_to_gpx.py` | log converter | Standard library only. Must keep reading the legacy 2022 log format. |

Embedded constraints: no heap allocation in the loop (use `RingBuffer`), no blocking
`delay()` outside `setup()`, `millis()` arithmetic must be wrap-safe (`now - last >= period`).

## Commands

```bash
pio test -e native                         # core unit tests on the host (Unity)
pio run  -e nodemcuv2                      # firmware build
pio run  -e nodemcuv2 -t upload            # flash (board on /dev/ttyUSB0, CH340)
python -m unittest discover -s tools/tests # converter tests
```

CI (`.github/workflows/ci.yml`) runs all three; keep them green.

## Hardware notes

- The NEO-6M emits GST (position error) only after `PUBX,40,GST,...`; `GpsReceiver`
  re-sends it every 30 s. Without GST no point is ever logged — by design.
- Indoors the receiver gets time but rarely a position; test logging outdoors or set
  `kRequireMovement = false` temporarily. Do not commit debug settings.

## Conventions

- C++17, 4-space indent, `camelCase` functions, `PascalCase` types, `kConstant` constants.
- New core behaviour comes with a test in `test/test_core/test_main.cpp`.
- Commit messages: imperative subject, body explains *why*.
