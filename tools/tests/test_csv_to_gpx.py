import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import csv_to_gpx  # noqa: E402

NS = {"g": csv_to_gpx.GPX_NS}

CURRENT = (
    "time_utc,latitude,longitude,sog_kn,cog_deg\n"
    "2026-11-01T10:00:03Z,50.253800,19.034500,5.3,123.0\n"
    "2026-11-01T10:00:00Z,50.253700,19.034400,5.1,121.0\n"
)
LEGACY = "50.253900,19.034600,5.200000,122.000000,01-11-2026,10:00:06\n"


class CsvToGpxTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def write(self, name: str, text: str) -> Path:
        path = self.dir / name
        path.write_text(text, encoding="ascii")
        return path

    def test_reads_both_formats_and_sorts_by_time(self) -> None:
        self.write("2026-11-01.csv", CURRENT)
        self.write("legacy.csv", LEGACY)
        files = csv_to_gpx.collect_inputs([self.dir])
        points = csv_to_gpx.merge(p for f in files for p in csv_to_gpx.read_points(f))
        self.assertEqual([p.time.second for p in points], [0, 3, 6])
        self.assertAlmostEqual(points[2].latitude, 50.2539)

    def test_duplicates_are_merged(self) -> None:
        a = self.write("a.csv", CURRENT)
        b = self.write("b.csv", CURRENT)
        points = csv_to_gpx.merge(p for f in (a, b) for p in csv_to_gpx.read_points(f))
        self.assertEqual(len(points), 2)

    def test_truncated_line_is_skipped(self) -> None:
        path = self.write("cut.csv", CURRENT + "2026-11-01T10:00:06Z,50.25")
        self.assertEqual(len(list(csv_to_gpx.read_points(path))), 2)

    def test_writes_valid_gpx_with_utc_times(self) -> None:
        self.write("2026-11-01.csv", CURRENT)
        out = self.dir / "track.gpx"
        self.assertEqual(csv_to_gpx.main([str(self.dir), str(out)]), 0)
        root = ET.parse(out).getroot()
        self.assertEqual(root.get("version"), "1.1")
        points = root.findall("g:trk/g:trkseg/g:trkpt", NS)
        self.assertEqual(len(points), 2)
        self.assertEqual(points[0].find("g:time", NS).text, "2026-11-01T10:00:00Z")
        self.assertEqual(points[0].get("lat"), "50.253700")


if __name__ == "__main__":
    unittest.main()
