---
description: Convert the SD card's daily CSV logs into one GPX track.
argument-hint: <folder-or-csv-files> <output.gpx>
allowed-tools: Bash(python tools/csv_to_gpx.py:*)
---

Convert the logs with `python tools/csv_to_gpx.py $ARGUMENTS` and report how many
points and files went in, the first and last timestamp, and any skipped lines.
