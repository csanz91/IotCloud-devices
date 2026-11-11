"""Run with: python3 -m unittest discover -s tools -p 'test_*.py'"""

import unittest

from ota_size_check import (
    check_esp32,
    check_esp8266,
    esp32_app_slot_size,
    esp8266_ota_space,
    parse_size,
)

DEFAULT_CSV = """\
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x140000,
app1,     app,  ota_1,   0x150000,0x140000,
spiffs,   data, spiffs,  0x290000,0x160000,
"""

# eagle.flash.1m.ld (no filesystem)
LD_1M = "PROVIDE ( _FS_start = 0x402FB000 );\nPROVIDE ( _FS_end = 0x402FB000 );\n"


class ParseSizeTest(unittest.TestCase):
    def test_formats(self):
        self.assertEqual(parse_size("0x140000"), 1310720)
        self.assertEqual(parse_size("1280K"), 1310720)
        self.assertEqual(parse_size("1M"), 1048576)
        self.assertEqual(parse_size("4096"), 4096)

    def test_rejects_garbage(self):
        with self.assertRaises(ValueError):
            parse_size("big")


class Esp32Test(unittest.TestCase):
    def test_default_slot(self):
        self.assertEqual(esp32_app_slot_size(DEFAULT_CSV), 1310720)

    def test_factory_only(self):
        csv = "nvs,data,nvs,0x9000,0x5000,\nfactory,app,factory,0x10000,3M,\n"
        self.assertEqual(esp32_app_slot_size(csv), 3 * 1024 * 1024)

    def test_no_app_partition(self):
        with self.assertRaises(ValueError):
            esp32_app_slot_size("nvs,data,nvs,0x9000,0x5000,\n")

    def test_madrid_v1_2_0_fits_with_low_margin(self):
        result = check_esp32(1307424, DEFAULT_CSV)
        self.assertTrue(result.fits)
        self.assertEqual(result.free, 3296)
        self.assertIn("low margin", result.message())

    def test_one_byte_over(self):
        result = check_esp32(1310721, DEFAULT_CSV)
        self.assertFalse(result.fits)
        self.assertIn("TOO BIG by 1 bytes", result.message())


class Esp8266Test(unittest.TestCase):
    def test_space_1m(self):
        self.assertEqual(esp8266_ota_space(LD_1M), 0xFB000)

    def test_typical_1m_device_fits(self):
        # sonoff_mini: ~505 KB, uncompressed
        result = check_esp8266(505104, 505104, LD_1M)
        self.assertTrue(result.fits)
        self.assertEqual(result.used, 2 * 507904)

    def test_too_big_uncompressed(self):
        result = check_esp8266(560064, 560064, LD_1M)
        self.assertFalse(result.fits)

    def test_same_image_fits_gzipped(self):
        result = check_esp8266(560064, 380000, LD_1M)
        self.assertTrue(result.fits)
        self.assertIn("gzip", result.message())

    def test_missing_fs_start(self):
        with self.assertRaises(ValueError):
            esp8266_ota_space("MEMORY {}")


if __name__ == "__main__":
    unittest.main()
