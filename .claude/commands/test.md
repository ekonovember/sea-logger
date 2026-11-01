---
description: Run every test suite (core on the host, converter) and build the firmware.
allowed-tools: Bash(pio test:*), Bash(pio run:*), Bash(python -m unittest:*)
---

Run the three checks CI runs and report pass/fail for each. On a failure, show the
failing assertion or compiler error and point at the file and line to fix.

!`pio test -e native`
!`python -m unittest discover -s tools/tests`
!`pio run -e nodemcuv2`
