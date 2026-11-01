#!/usr/bin/env python3
"""Merge Sea Logger's daily CSV logs into one GPX 1.1 track.

    python tools/csv_to_gpx.py SD_CARD_DIR track.gpx
    python tools/csv_to_gpx.py 2026-11-01.csv 2026-11-02.csv -o track.gpx

Reads both log formats the firmware has written:

* current (header line, ISO 8601 UTC):
  ``time_utc,latitude,longitude,sog_kn,cog_deg``
* legacy 2022 firmware (no header):
  ``latitude,longitude,speed,heading,DD-MM-YYYY,HH:MM:SS``

Points from all files are sorted by time and de-duplicated. Standard library
only — no pandas needed on the laptop you take sailing.
"""

from __future__ import annotations

import argparse
import csv
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Iterator

GPX_NS = "http://www.topografix.com/GPX/1/1"


@dataclass(frozen=True, order=True)
class Point:
    time: datetime
    latitude: float
    longitude: float
    sog_kn: float
    cog_deg: float


def _parse_row(row: list[str]) -> Point | None:
    if not row or row[0].startswith("time_utc"):
        return None
    if len(row) == 5:  # current format
        time = datetime.strptime(row[0], "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
        lat, lon, sog, cog = (float(v) for v in row[1:5])
    elif len(row) == 6:  # legacy format
        lat, lon, sog, cog = (float(v) for v in row[0:4])
        time = datetime.strptime(f"{row[4]} {row[5]}", "%d-%m-%Y %H:%M:%S").replace(
            tzinfo=timezone.utc
        )
    else:
        raise ValueError(f"unexpected column count {len(row)}: {row!r}")
    return Point(time, lat, lon, sog, cog)


def read_points(path: Path) -> Iterator[Point]:
    with path.open(newline="", encoding="ascii") as handle:
        for line_no, row in enumerate(csv.reader(handle), start=1):
            try:
                point = _parse_row(row)
            except ValueError as error:
                # A power cut can truncate the last line of a file; skip it, say so.
                print(f"{path}:{line_no}: skipped ({error})", file=sys.stderr)
                continue
            if point is not None:
                yield point


def merge(points: Iterable[Point]) -> list[Point]:
    """Sorted by time; when two files hold the same instant, keep one point."""
    unique = {p.time: p for p in points}
    return [unique[t] for t in sorted(unique)]


def to_gpx(points: list[Point], name: str) -> ET.ElementTree:
    ET.register_namespace("", GPX_NS)
    gpx = ET.Element(f"{{{GPX_NS}}}gpx", version="1.1", creator="ekonovember/sea-logger")
    trk = ET.SubElement(gpx, f"{{{GPX_NS}}}trk")
    ET.SubElement(trk, f"{{{GPX_NS}}}name").text = name
    segment = ET.SubElement(trk, f"{{{GPX_NS}}}trkseg")
    for p in points:
        trkpt = ET.SubElement(
            segment, f"{{{GPX_NS}}}trkpt", lat=f"{p.latitude:.6f}", lon=f"{p.longitude:.6f}"
        )
        ET.SubElement(trkpt, f"{{{GPX_NS}}}time").text = p.time.strftime("%Y-%m-%dT%H:%M:%SZ")
    tree = ET.ElementTree(gpx)
    ET.indent(tree, space="  ")
    return tree


def collect_inputs(inputs: list[Path]) -> list[Path]:
    files: list[Path] = []
    for item in inputs:
        files.extend(sorted(item.glob("*.csv")) if item.is_dir() else [item])
    return files


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("inputs", nargs="+", type=Path, help="CSV files or directories")
    parser.add_argument("-o", "--output", type=Path, help="GPX file to write")
    parser.add_argument("--name", default="Sea Logger track", help="track name in the GPX")
    args = parser.parse_args(argv)

    inputs = list(args.inputs)
    output = args.output
    if output is None:  # `csv_to_gpx.py DIR out.gpx` form
        if len(inputs) < 2 or inputs[-1].suffix.lower() != ".gpx":
            parser.error("give the output as -o FILE.gpx or as the last argument")
        output = inputs.pop()

    files = collect_inputs(inputs)
    if not files:
        parser.error("no CSV files found")
    points = merge(p for f in files for p in read_points(f))
    to_gpx(points, args.name).write(output, encoding="utf-8", xml_declaration=True)
    print(f"{len(points)} points from {len(files)} file(s) -> {output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
