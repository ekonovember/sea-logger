---
description: Build and flash the firmware to the NodeMCU, then show the boot output.
allowed-tools: Bash(pio run:*), Bash(pio device:*)
---

1. Confirm the device on the serial port is an ESP8266 (`pio device list`; the NodeMCU
   v3 shows up as a CH340, USB ID 1a86:7523) — do not flash an unknown device.
2. Make sure `include/config.h` has release settings (`kSerialDebug = false`,
   `kRequireMovement = true`) unless the user asked for a debug build.
3. Build and upload: `pio run -e nodemcuv2 -t upload`.
4. Tell the user what to look for on the screen: "Sea Logger v2.0", then the
   instrument screen; the button toggles the display.
