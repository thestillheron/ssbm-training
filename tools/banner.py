"""Patch a copy of the disc banner, `opening.bnr` (ADR-0007, ADR-0008).

    patch_banner(src, dest, image_path, text_path)

`src` (the vanilla `BNR1` banner) is only read. `dest` is written as a fresh
regular file, so the extracted disc under orig/ is never touched. The banner
keeps its size and layout: a 0x20-byte header, the 96x32 image in RGB5A3,
then the text fields. Only the image and the text are replaced.

The image is a 96x32 PNG. The text file holds the short name on its first
line, the maker on its second and the description on the rest (one or two
lines). The long name and long maker repeat the short ones. Anything that
does not fit is a `PatchError`; nothing is resized or truncated.
"""

from pathlib import Path

from tools.archive_patcher import PatchError, read_png

MAGIC = b"BNR1"
WIDTH, HEIGHT = 96, 32
IMAGE = 0x20
SIZE = 0x1960
# (offset, size) of each text field, in the order the text file feeds them.
SHORT_NAME = (0x1820, 0x20)
SHORT_MAKER = (0x1840, 0x20)
LONG_NAME = (0x1860, 0x40)
LONG_MAKER = (0x18A0, 0x40)
DESCRIPTION = (0x18E0, 0x80)
ENCODING = "cp1252"  # NTSC-U banners are Windows-1252


def _rgb5a3(r, g, b, a):
    # Opaque: flag bit 15 set + 5 bits per channel; else 3-bit alpha + 4 bits per channel.
    a3 = (a * 7 + 127) // 255
    if a3 == 7:
        return 0x8000 | ((r * 31 + 127) // 255) << 10 | ((g * 31 + 127) // 255) << 5 | (b * 31 + 127) // 255
    return a3 << 12 | ((r * 15 + 127) // 255) << 8 | ((g * 15 + 127) // 255) << 4 | (b * 15 + 127) // 255


def _encode_image(image):
    """The 96x32 image as RGB5A3: 4x4-pixel tiles, big-endian pixels."""
    out = bytearray()
    for ty in range(0, HEIGHT, 4):
        for tx in range(0, WIDTH, 4):
            for y in range(4):
                for x in range(4):
                    out += _rgb5a3(*image.pixels[ty + y][tx + x]).to_bytes(2, "big")
    return bytes(out)


def _put_text(data, field, text, what, path):
    offset, size = field
    try:
        raw = text.encode(ENCODING)
    except UnicodeEncodeError:
        raise PatchError(f"{path}: the {what} {text!r} has characters a banner cannot show")
    if len(raw) > size:
        raise PatchError(f"{path}: the {what} {text!r} is {len(raw)} bytes; it must fit in {size}")
    data[offset : offset + size] = raw.ljust(size, b"\0")


def patch_banner(src, dest, image_path, text_path):
    data = bytearray(Path(src).read_bytes())
    if len(data) != SIZE or data[:4] != MAGIC:
        raise PatchError(f"{src} is not a BNR1 banner")
    image = read_png(image_path)
    if (image.width, image.height) != (WIDTH, HEIGHT):
        raise PatchError(
            f"{image_path} is {image.width}x{image.height}; the banner must be {WIDTH}x{HEIGHT}"
        )
    lines = Path(text_path).read_text(encoding="utf-8").splitlines()
    if len(lines) < 3:
        raise PatchError(f"{text_path} needs three lines or more: short name, maker, description")
    name, maker, description = lines[0], lines[1], "\n".join(lines[2:])
    data[IMAGE : IMAGE + WIDTH * HEIGHT * 2] = _encode_image(image)
    _put_text(data, SHORT_NAME, name, "short name", text_path)
    _put_text(data, SHORT_MAKER, maker, "maker", text_path)
    _put_text(data, LONG_NAME, name, "short name", text_path)
    _put_text(data, LONG_MAKER, maker, "maker", text_path)
    _put_text(data, DESCRIPTION, description, "description", text_path)
    Path(dest).unlink(missing_ok=True)
    Path(dest).write_bytes(data)
