"""Patch copies of the main menu's HSD archives (ADR-0007).

The interface is a source archive plus a list of replacements, producing the
patched archive:

    patch_archive(src, dest, [TextureReplacement(...), StringReplacement(...)])

`src` is only read. `dest` is written as a fresh regular file (never a link
to `src`), so the extracted disc under orig/ is never touched.

Everything is overwritten in place: the archive keeps its size, every offset
and every relocation. A replacement that does not fit is a `PatchError`,
never resized or truncated.

- A `TextureReplacement` names an image descriptor (its offset in the archive's
  data section) and gives the image to put there. The image must have exactly
  the descriptor's width and height; it is encoded to the descriptor's format
  (I4, I8, IA4 or IA8). Only the pixels of the image are written, so a
  texture's tile padding keeps its bytes.
- A `StringReplacement` names an entry of the archive's `SIS_MenuData` table
  and gives new text lines for it (ASCII letters, digits, spaces and `,.:!?`).
  The entry's own control codes are kept; only the glyphs of its text lines
  change. If the text does not fit the entry's slot it may take over the space
  of unused entries listed in `spill`: the entry's table pointer is moved there
  and the donors' pointers follow it, so no donor can show leftover bytes.
"""

import struct
import zlib
from dataclasses import dataclass
from pathlib import Path

DATA_START = 0x20  # the archive header's size: the data section follows it
SIS_ROOT = "SIS_MenuData"


class PatchError(Exception):
    """A replacement cannot be applied. The destination is not written."""


# ---------------------------------------------------------------- images


class Image:
    """`width` x `height` pixels, `pixels[y][x] = (r, g, b, a)`."""

    def __init__(self, width, height, pixels):
        self.width = width
        self.height = height
        self.pixels = pixels


def read_png(path):
    """An 8-bit, non-interlaced PNG as an Image (gray, RGB, palette, and
    either with alpha)."""
    data = Path(path).read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise PatchError(f"{path} is not a PNG")
    pos = 8
    idat = b""
    palette = []
    trns = b""
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos : pos + 8])
        body = data[pos + 8 : pos + 8 + n]
        pos += 12 + n
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"PLTE":
            palette = [tuple(body[i : i + 3]) for i in range(0, n, 3)]
        elif kind == b"tRNS":
            trns = body
        elif kind == b"IDAT":
            idat += body
    if depth != 8 or interlace:
        raise PatchError(f"{path}: only 8-bit, non-interlaced PNGs are supported")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    stride = width * channels
    raw = zlib.decompress(idat)
    rows = []
    prev = bytearray(stride)
    for y in range(height):
        base = y * (stride + 1)
        ftype = raw[base]
        line = bytearray(raw[base + 1 : base + 1 + stride])
        for i in range(stride):
            a = line[i - channels] if i >= channels else 0
            b = prev[i]
            c = prev[i - channels] if i >= channels else 0
            if ftype == 1:
                line[i] = (line[i] + a) & 255
            elif ftype == 2:
                line[i] = (line[i] + b) & 255
            elif ftype == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif ftype == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 255
        rows.append(line)
        prev = line
    pixels = []
    for line in rows:
        row = []
        for x in range(width):
            px = line[x * channels : (x + 1) * channels]
            if ctype == 0:
                row.append((px[0], px[0], px[0], 255))
            elif ctype == 2:
                row.append((px[0], px[1], px[2], 255))
            elif ctype == 3:
                r, g, b = palette[px[0]]
                row.append((r, g, b, trns[px[0]] if px[0] < len(trns) else 255))
            elif ctype == 4:
                row.append((px[0], px[0], px[0], px[1]))
            else:
                row.append(tuple(px))
        pixels.append(row)
    return Image(width, height, pixels)


# ---------------------------------------------------------------- textures

# GX image formats we can write: format id -> (name, tile width, tile height,
# bits per pixel).
FORMATS = {0: ("I4", 8, 8, 4), 1: ("I8", 8, 4, 8), 2: ("IA4", 8, 4, 8), 3: ("IA8", 4, 4, 16)}


def _intensity(px):
    r, g, b, _ = px
    return (r * 299 + g * 587 + b * 114 + 500) // 1000


def _to4(v):
    return (v * 15 + 127) // 255


def _encode_texture(image, fmt, data):
    """Write `image` into the bytearray `data` (the texture's current bytes)
    in format `fmt`. Padding pixels outside the image keep their bytes."""
    _, tw, th, bpp = FORMATS[fmt]
    pos = 0
    for ty in range(0, image.height, th):
        for tx in range(0, image.width, tw):
            for y in range(th):
                for x in range(tw):
                    px = None
                    if ty + y < image.height and tx + x < image.width:
                        px = image.pixels[ty + y][tx + x]
                    if fmt == 0:  # I4: two pixels a byte, left one in the high nibble
                        i = pos // 2
                        if px is not None:
                            v = _to4(_intensity(px))
                            if pos % 2 == 0:
                                data[i] = (data[i] & 0x0F) | (v << 4)
                            else:
                                data[i] = (data[i] & 0xF0) | v
                    elif px is not None:
                        i = pos * bpp // 8
                        if fmt == 1:
                            data[i] = _intensity(px)
                        elif fmt == 2:
                            data[i] = (_to4(px[3]) << 4) | _to4(_intensity(px))
                        else:
                            data[i] = px[3]
                            data[i + 1] = _intensity(px)
                    pos += 1


def _texture_size(width, height, fmt):
    _, tw, th, bpp = FORMATS[fmt]
    return ((width + tw - 1) // tw) * tw * ((height + th - 1) // th) * th * bpp // 8


@dataclass(frozen=True)
class TextureReplacement:
    """Put `image` (an Image or the path of a PNG) at the image descriptor
    whose offset in the data section is `descriptor`.

    `format` (an `FORMATS` name: "I4", "I8", "IA4" or "IA8") and `size`
    (`(width, height)`) say what the descriptor is expected to be. A
    descriptor offset alone only proves a pointer lives there; the patcher
    refuses to write unless the descriptor's actual format and size match
    both these and the image, so an offset that is off (a different archive
    build, a wrong copy-paste) fails loudly instead of corrupting a texture."""

    descriptor: int
    image: object
    format: str
    size: tuple


# ---------------------------------------------------------------- SIS strings

# Glyph codes of the menu font's latin letters and the punctuation the
# descriptions use. Both language variants (SdMenu.usd and .dat) share them.
_PUNCTUATION = {",": 0xE6, ".": 0xE7, ":": 0xE9, "!": 0xEC, "?": 0xEB}
LINE_START, LINE_END, SPACE = 0x18, 0x19, 0x1A
_GLYPH_LEAD = 0x20


def encode_sis_text(text):
    """The SIS bytes of one line of ASCII text."""
    out = bytearray()
    for ch in text:
        if ch == " ":
            out.append(SPACE)
            continue
        if "0" <= ch <= "9":
            code = ord(ch) - ord("0")
        elif "A" <= ch <= "Z":
            code = 0x0A + ord(ch) - ord("A")
        elif "a" <= ch <= "z":
            code = 0x24 + ord(ch) - ord("a")
        elif ch in _PUNCTUATION:
            code = _PUNCTUATION[ch]
        else:
            raise PatchError(f"no menu glyph for {ch!r} in {text!r}")
        out += bytes([_GLYPH_LEAD, code])
    return bytes(out)


def decode_sis_text(data):
    """Inverse of encode_sis_text; a glyph it has no character for shows as
    `<xx>` (two-byte codes from other pages as `<pppp>`)."""
    out = ""
    i = 0
    punct = {v: k for k, v in _PUNCTUATION.items()}
    while i < len(data):
        b = data[i]
        if b == SPACE:
            out += " "
            i += 1
        elif b == _GLYPH_LEAD:
            code = data[i + 1]
            i += 2
            if code < 10:
                out += chr(ord("0") + code)
            elif code < 0x24:
                out += chr(ord("A") + code - 0x0A)
            elif code < 0x3E:
                out += chr(ord("a") + code - 0x24)
            elif code in punct:
                out += punct[code]
            else:
                out += f"<{code:02x}>"
        else:
            out += f"<{b:02x}{data[i + 1]:02x}>"
            i += 2
    return out


def _is_lead(byte):
    return 0x20 <= byte <= 0x4F


def split_entry(entry):
    """Split an SIS entry into its fixed bytes and its text lines: returns
    (chrome, lines), where chrome[i] precedes line i and the last chrome
    follows the last line."""
    chrome, lines = [], []
    i = 0
    start = 0
    while True:
        try:
            i = entry.index(bytes([LINE_START]), i)
        except ValueError:
            break
        chrome.append(entry[start : i + 1])
        i += 1
        text_start = i
        while i < len(entry) and entry[i] != LINE_END:
            i += 2 if _is_lead(entry[i]) else 1
        lines.append(entry[text_start:i])
        start = i
    chrome.append(entry[start:])
    return chrome, lines


@dataclass(frozen=True)
class StringReplacement:
    """Replace the text of SIS entry `index` with `text`, one line a `\\n`.

    `template` is the entry whose control codes and line count to use
    (default: the entry itself); a longer or two-line description is built on
    another entry's layout. It is an entry number, or `(file name, entry
    number)` for an entry of another archive in the source's folder (the
    Japanese menu has no two-line description of its own to copy). `spill` is an inclusive (first, last) range of
    consecutive entries that nothing shows any more, whose space the entry may
    take when its own slot is too small."""

    index: int
    text: str
    template: int = None
    spill: tuple = None


# ---------------------------------------------------------------- the archive


class _Archive:
    def __init__(self, raw):
        self.raw = bytearray(raw)
        if len(raw) < DATA_START:
            raise PatchError("not an archive: too short")
        file_size, data_size, reloc_count, root_count, ref_count = struct.unpack(">5I", raw[:20])
        if file_size != len(raw):
            raise PatchError(f"archive header says {file_size} bytes, the file has {len(raw)}")
        self.data_size = data_size
        reloc_at = DATA_START + data_size
        self.relocs = set(struct.unpack(f">{reloc_count}I", raw[reloc_at : reloc_at + 4 * reloc_count]))
        root_at = reloc_at + 4 * reloc_count
        names_at = root_at + 8 * (root_count + ref_count)
        self.roots = {}
        for n in range(root_count):
            offset, name_off = struct.unpack(">II", raw[root_at + 8 * n : root_at + 8 * n + 8])
            end = raw.index(b"\0", names_at + name_off)
            self.roots[raw[names_at + name_off : end].decode()] = offset

    def u32(self, offset):
        return struct.unpack(">I", self.raw[DATA_START + offset : DATA_START + offset + 4])[0]

    def set_u32(self, offset, value):
        self.raw[DATA_START + offset : DATA_START + offset + 4] = struct.pack(">I", value)

    def slice(self, start, end):
        return bytes(self.raw[DATA_START + start : DATA_START + end])

    def write(self, offset, data):
        self.raw[DATA_START + offset : DATA_START + offset + len(data)] = data

    def pointer(self, offset):
        if offset not in self.relocs:
            raise PatchError(f"{offset:#x} is not a pointer in this archive")
        return self.u32(offset)

    # textures

    def replace_texture(self, descriptor, image, expected_format, expected_size):
        if descriptor % 4 or descriptor + 0x18 > self.data_size:
            raise PatchError(f"{descriptor:#x} is not an image descriptor")
        data_at = self.pointer(descriptor)
        width, height = struct.unpack(">HH", self.slice(descriptor + 4, descriptor + 8))
        fmt = self.u32(descriptor + 8)
        if fmt not in FORMATS:
            raise PatchError(f"image {descriptor:#x} has format {fmt}, which cannot be written")
        if FORMATS[fmt][0] != expected_format or (width, height) != tuple(expected_size):
            raise PatchError(
                f"image {descriptor:#x} is {width}x{height} {FORMATS[fmt][0]}; "
                f"expected {expected_size[0]}x{expected_size[1]} {expected_format}"
            )
        if (image.width, image.height) != (width, height):
            raise PatchError(
                f"image {descriptor:#x} is {width}x{height} {FORMATS[fmt][0]}; "
                f"the replacement is {image.width}x{image.height}"
            )
        end = data_at + _texture_size(width, height, fmt)
        if end > self.data_size:
            raise PatchError(f"image {descriptor:#x} runs past the end of the data")
        data = bytearray(self.slice(data_at, end))
        _encode_texture(image, fmt, data)
        self.write(data_at, data)

    # strings

    def _sis_table(self):
        if SIS_ROOT not in self.roots:
            raise PatchError(f"no {SIS_ROOT} root in this archive")
        return self.roots[SIS_ROOT]

    def entry_span(self, index):
        table = self._sis_table()
        start = self.pointer(table + 4 * index)
        # An entry runs up to the nearest entry that starts after it.
        later = [p for p in self.sis_pointers() if p > start]
        if not later:
            raise PatchError(f"SIS entry {index:#x} has no slot")
        return start, min(later)

    def sis_pointers(self):
        """The values of every pointer in the SIS table (which runs for as
        long as consecutive fields are pointers)."""
        table = self._sis_table()
        found = []
        while table + 4 * len(found) in self.relocs:
            found.append(self.u32(table + 4 * len(found)))
        return found

    def entry(self, index):
        return self.slice(*self.entry_span(index))

    def replace_string(self, rep, folder):
        table = self._sis_table()
        start, end = self.entry_span(rep.index)
        if rep.template is None:
            template = self.entry(rep.index)
        elif isinstance(rep.template, tuple):
            name, number = rep.template
            template = _Archive((folder / name).read_bytes()).entry(number)
        else:
            template = self.entry(rep.template)
        chrome, old_lines = split_entry(template)
        lines = rep.text.split("\n")
        if len(lines) != len(old_lines):
            raise PatchError(
                f"entry {rep.index:#x} has {len(old_lines)} text lines; "
                f"the replacement has {len(lines)}"
            )
        new = bytearray()
        for fixed, line in zip(chrome, lines):
            new += fixed + encode_sis_text(line)
        new += chrome[-1]
        if len(new) <= end - start:
            self.write(start, bytes(new) + bytes(end - start - len(new)))
            return
        if rep.spill is None:
            raise PatchError(
                f"entry {rep.index:#x} holds {end - start} bytes; {rep.text!r} needs {len(new)}"
            )
        first, last = rep.spill
        if first <= rep.index <= last:
            raise PatchError("an entry cannot spill into itself")
        donor_start = self.entry_span(first)[0]
        donor_end = self.entry_span(last)[1]
        if len(new) > donor_end - donor_start:
            raise PatchError(
                f"entry {rep.index:#x} needs {len(new)} bytes; "
                f"entries {first:#x}-{last:#x} hold {donor_end - donor_start}"
            )
        self.write(donor_start, bytes(new) + bytes(donor_end - donor_start - len(new)))
        # The old slot is unused now; the donors' pointers follow the entry so
        # none of them points into the middle of the new text.
        self.write(start, bytes(end - start))
        for i in [rep.index, *range(first, last + 1)]:
            self.set_u32(table + 4 * i, donor_start)


def read_sis_lines(archive, index):
    """The text lines of SIS entry `index` of an archive (path or bytes)."""
    raw = Path(archive).read_bytes() if isinstance(archive, (str, Path)) else archive
    _, lines = split_entry(_Archive(raw).entry(index))
    return [decode_sis_text(line) for line in lines]


def patch_archive(src, dest, replacements):
    """Write `src` with `replacements` applied to `dest` (a fresh file)."""
    src, dest = Path(src), Path(dest)
    if dest.exists() and src.samefile(dest):
        raise PatchError(f"{dest} is the source archive; patched copies go elsewhere")
    archive = _Archive(src.read_bytes())
    for rep in replacements:
        if isinstance(rep, TextureReplacement):
            image = rep.image if isinstance(rep.image, Image) else read_png(rep.image)
            archive.replace_texture(rep.descriptor, image, rep.format, rep.size)
        elif isinstance(rep, StringReplacement):
            archive.replace_string(rep, src.parent)
        else:
            raise PatchError(f"unknown replacement {rep!r}")
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists() or dest.is_symlink():
        dest.unlink()  # never write through a link to the original
    dest.write_bytes(bytes(archive.raw))
