"""Checks that a firmware image fits the device it will be sent to over OTA.

PlatformIO's "Flash:" line counts code and data only. The OTA image also has
headers and alignment padding (about 20 KB on ESP32), so a build that looks
fine can still be too big. These checks use the real .bin size.

- ESP32: the image must fit one app (OTA) slot of the partition table.
- ESP8266: the new image is staged next to the running one, so both must fit
  below the filesystem. The running image's size isn't known here; the check
  assumes it is about the size of this build, which holds when updating a
  device to the next version of the same firmware.

Pure functions only, so it can be tested without PlatformIO.
"""

import re

SECTOR = 4096
ESP8266_FLASH_BASE = 0x40200000
# Below this, every small change needs a size check by hand.
LOW_MARGIN_BYTES = 16 * 1024


def parse_size(value: str) -> int:
    """Parses a partition table size: 0x140000, 1280K, 1M or 1310720."""
    value = value.strip()
    match = re.fullmatch(r"(0x[0-9a-fA-F]+|\d+)\s*([kKmM]?)", value)
    if not match:
        raise ValueError(f"unknown size: {value!r}")
    number = int(match.group(1), 0)
    unit = match.group(2).upper()
    return number * {"": 1, "K": 1024, "M": 1024 * 1024}[unit]


def esp32_app_slot_size(partitions_csv: str) -> int:
    """Size of the smallest OTA app slot (or the factory app if there's no OTA)."""
    ota_sizes = []
    factory_sizes = []
    for line in partitions_csv.splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        fields = [field.strip() for field in line.split(",")]
        if len(fields) < 5 or fields[1] != "app":
            continue
        size = parse_size(fields[4])
        if fields[2].startswith("ota_"):
            ota_sizes.append(size)
        else:
            factory_sizes.append(size)
    sizes = ota_sizes or factory_sizes
    if not sizes:
        raise ValueError("no app partition in the partition table")
    return min(sizes)


def esp8266_ota_space(ldscript: str) -> int:
    """Bytes the running and the staged image share (flash start to _FS_start)."""
    match = re.search(r"_FS_start\s*=\s*(0x[0-9a-fA-F]+)", ldscript)
    if not match:
        raise ValueError("no _FS_start in the linker script")
    return int(match.group(1), 16) - ESP8266_FLASH_BASE


def round_to_sector(size: int) -> int:
    return (size + SECTOR - 1) // SECTOR * SECTOR


class SizeCheck:
    def __init__(self, used: int, limit: int, detail: str):
        self.used = used
        self.limit = limit
        self.detail = detail

    @property
    def fits(self) -> bool:
        return self.used <= self.limit

    @property
    def free(self) -> int:
        return self.limit - self.used

    def message(self) -> str:
        if not self.fits:
            return (
                f"OTA size: TOO BIG by {-self.free} bytes "
                f"({self.used} of {self.limit}; {self.detail})"
            )
        line = f"OTA size: {self.used} of {self.limit} bytes, {self.free} free ({self.detail})"
        if self.free < LOW_MARGIN_BYTES:
            line += " - low margin"
        return line


def check_esp32(image_size: int, partitions_csv: str) -> SizeCheck:
    slot = esp32_app_slot_size(partitions_csv)
    return SizeCheck(image_size, slot, "image vs. app slot")


def check_esp8266(image_size: int, upload_size: int, ldscript: str) -> SizeCheck:
    """`upload_size` is the gzip size when compression is on, else `image_size`."""
    space = esp8266_ota_space(ldscript)
    used = round_to_sector(image_size) + round_to_sector(upload_size)
    detail = "running image + staged update, assuming the device runs a build this size"
    if upload_size != image_size:
        detail = "running image + gzip update, assuming the device runs a build this size"
    return SizeCheck(used, space, detail)
