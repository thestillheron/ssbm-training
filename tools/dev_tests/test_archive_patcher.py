import shutil
import struct
import tempfile
import unittest
import zlib
from pathlib import Path

from tools.archive_patcher import (
    Image,
    PatchError,
    StringReplacement,
    TextureReplacement,
    patch_archive,
    read_png,
    read_sis_lines,
)

from .support import REPO_ROOT, sha1

ORIG_FILES = REPO_ROOT / "orig" / "GALE01" / "files"
MNMA = ORIG_FILES / "MnMaAll.usd"
SDMENU = ORIG_FILES / "SdMenu.usd"
DATA = 0x20  # the archive header is 0x20 bytes, then the data section

# Image descriptors in MnMaAll.usd (offsets into the data section).
LABEL_IA4 = 0xBFFA8  # a 176x30 IA4 label frame
HEADING_I4 = 0x562DC  # a 168x28 I4 heading frame (its height is not a multiple of 8)


def label(image):
    """A replacement for the IA4 label frame, declared as the patcher expects."""
    return TextureReplacement(LABEL_IA4, image, "IA4", (176, 30))


def heading(image):
    return TextureReplacement(HEADING_I4, image, "I4", (168, 28))


def tex(descriptor, image):
    return label(image) if descriptor == LABEL_IA4 else heading(image)


requires_archives = unittest.skipUnless(
    MNMA.is_file() and SDMENU.is_file(),
    "no extracted disc: run `dev.py setup` first",
)


def checker(width, height, levels=16):
    """A deterministic test picture whose pixels are all on the 16 levels of
    IA4 and I4: (r, g, b) = intensity, alpha varies independently."""
    rows = []
    for y in range(height):
        row = []
        for x in range(width):
            v = ((x * 7 + y * 3) % levels) * 17
            a = ((x * 5 + y * 11 + 3) % levels) * 17
            row.append((v, v, v, a))
        rows.append(row)
    return Image(width, height, rows)


def u32(raw, offset):
    return struct.unpack(">I", raw[DATA + offset : DATA + offset + 4])[0]


def texture_bytes(raw, descriptor):
    """(data offset, width, height, format) of a descriptor, read by hand."""
    return (
        u32(raw, descriptor),
        *struct.unpack(">HH", raw[DATA + descriptor + 4 : DATA + descriptor + 8]),
        u32(raw, descriptor + 8),
    )


def decode_ia4(raw, descriptor):
    """Decoded (intensity, alpha) rows of an IA4 texture: 8x4 tiles of bytes
    with alpha in the high nibble."""
    at, w, h, fmt = texture_bytes(raw, descriptor)
    assert fmt == 2
    rows = [[None] * w for _ in range(h)]
    p = DATA + at
    for ty in range(0, h, 4):
        for tx in range(0, w, 8):
            for y in range(4):
                for x in range(8):
                    b = raw[p]
                    p += 1
                    if ty + y < h and tx + x < w:
                        rows[ty + y][tx + x] = ((b & 15) * 17, (b >> 4) * 17)
    return rows


def decode_i4(raw, descriptor):
    """Decoded intensity rows of an I4 texture: 8x8 tiles, left pixel in the
    high nibble."""
    at, w, h, fmt = texture_bytes(raw, descriptor)
    assert fmt == 0
    rows = [[None] * w for _ in range(h)]
    n = 0
    for ty in range(0, h, 8):
        for tx in range(0, w, 8):
            for y in range(8):
                for x in range(8):
                    b = raw[DATA + at + n // 2]
                    v = (b >> 4) if n % 2 == 0 else (b & 15)
                    n += 1
                    if ty + y < h and tx + x < w:
                        rows[ty + y][tx + x] = v * 17
    return rows


def texture_extent(raw, descriptor):
    """The byte range of a texture's data, tile padding included."""
    at, w, h, fmt = texture_bytes(raw, descriptor)
    tw, th, bpp = {0: (8, 8, 4), 2: (8, 4, 8)}[fmt]
    size = -(-w // tw) * tw * -(-h // th) * th * bpp // 8
    return DATA + at, DATA + at + size


def changed(a, b):
    """Byte offsets where two equal-length byte strings differ."""
    assert len(a) == len(b)
    return [i for i in range(len(a)) if a[i] != b[i]]


class ArchivePatcherCase(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="ssbm-patch-"))
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)

    def patch(self, src, replacements, name="out"):
        dest = self.tmp / name
        patch_archive(src, dest, replacements)
        return dest.read_bytes()


@requires_archives
class TextureTests(ArchivePatcherCase):
    def test_ia4_replacement_decodes_back_to_the_input_image(self):
        image = checker(176, 30)
        out = self.patch(MNMA, [label(image)])
        expected = [[(v, a) for (v, _, _, a) in row] for row in image.pixels]
        self.assertEqual(decode_ia4(out, LABEL_IA4), expected)

    def test_i4_replacement_decodes_back_to_the_input_image(self):
        image = checker(168, 28)
        out = self.patch(MNMA, [heading(image)])
        expected = [[v for (v, _, _, _) in row] for row in image.pixels]
        self.assertEqual(decode_i4(out, HEADING_I4), expected)

    def test_every_byte_outside_the_texture_is_unchanged(self):
        original = MNMA.read_bytes()
        for descriptor, image in ((LABEL_IA4, checker(176, 30)), (HEADING_I4, checker(168, 28))):
            out = self.patch(MNMA, [tex(descriptor, image)])
            lo, hi = texture_extent(original, descriptor)
            self.assertEqual(len(out), len(original))
            diff = changed(original, out)
            self.assertTrue(diff)
            self.assertTrue(all(lo <= i < hi for i in diff), "bytes outside the texture changed")

    def test_tile_padding_below_the_image_keeps_its_bytes(self):
        # 168x28 I4 is stored as 21x4 tiles of 8x8: the last 4 rows of the
        # bottom tiles belong to no pixel.
        original = MNMA.read_bytes()
        out = self.patch(MNMA, [heading(checker(168, 28))])
        at, w, h, _ = texture_bytes(original, HEADING_I4)
        tile = 8 * 8 // 2
        for tx in range(21):
            base = DATA + at + (3 * 21 + tx) * tile
            # rows 4..7 of the tile are the padding: bytes 16..31
            self.assertEqual(out[base + 16 : base + 32], original[base + 16 : base + 32])

    def test_png_files_are_read_and_replace_a_texture(self):
        image = checker(176, 30)
        png = self.tmp / "label.png"
        write_rgba_png(png, image)
        read = read_png(png)
        self.assertEqual((read.width, read.height), (176, 30))
        self.assertEqual(read.pixels, image.pixels)
        out = self.patch(MNMA, [label(png)])
        expected = [[(v, a) for (v, _, _, a) in row] for row in image.pixels]
        self.assertEqual(decode_ia4(out, LABEL_IA4), expected)

    def test_a_wrong_sized_image_is_an_error_and_writes_nothing(self):
        for size in ((175, 30), (176, 29), (30, 176)):
            with self.assertRaises(PatchError) as ctx:
                self.patch(MNMA, [label(checker(*size))], name="bad")
            self.assertIn("176x30", str(ctx.exception))
        self.assertFalse((self.tmp / "bad").exists())

    def test_a_declared_format_that_is_not_the_descriptors_is_an_error(self):
        wrong = TextureReplacement(LABEL_IA4, checker(176, 30), "I4", (176, 30))
        with self.assertRaises(PatchError) as ctx:
            self.patch(MNMA, [wrong], name="bad")
        self.assertIn("IA4", str(ctx.exception))
        self.assertFalse((self.tmp / "bad").exists())

    def test_a_declared_size_that_is_not_the_descriptors_is_an_error(self):
        wrong = TextureReplacement(LABEL_IA4, checker(168, 28), "IA4", (168, 28))
        with self.assertRaises(PatchError) as ctx:
            self.patch(MNMA, [wrong], name="bad")
        self.assertIn("176x30", str(ctx.exception))

    def test_an_image_that_is_not_the_declared_size_is_an_error(self):
        wrong = TextureReplacement(LABEL_IA4, checker(168, 28), "IA4", (176, 30))
        with self.assertRaises(PatchError):
            self.patch(MNMA, [wrong], name="bad")

    def test_something_that_is_not_a_descriptor_is_an_error(self):
        with self.assertRaises(PatchError):
            self.patch(MNMA, [TextureReplacement(0x1, checker(176, 30), "IA4", (176, 30))])
        with self.assertRaises(PatchError):  # a data offset with no pointer in it
            self.patch(MNMA, [TextureReplacement(LABEL_IA4 + 4, checker(176, 30), "IA4", (176, 30))])


@requires_archives
class StringTests(ArchivePatcherCase):
    def test_retail_descriptions_read_as_text(self):
        self.assertEqual(read_sis_lines(SDMENU, 0x84), ["Game setup."])
        self.assertEqual(read_sis_lines(SDMENU, 0x85), ["View game records."])
        self.assertEqual(
            read_sis_lines(SDMENU, 0x8D), ["Conquer all enemies using", "limited recovery items."]
        )

    def test_a_shorter_string_replaces_in_place_and_reads_back(self):
        out = self.patch(SDMENU, [StringReplacement(0x84, "Hi.")])
        self.assertEqual(read_sis_lines(out, 0x84), ["Hi."])
        # H, i, . in the menu font, 20 xx each.
        self.assertIn(bytes.fromhex("18 20 11 20 2c 20 e7 19"), out)

    def test_every_byte_outside_the_slot_is_unchanged(self):
        original = SDMENU.read_bytes()
        out = self.patch(SDMENU, [StringReplacement(0x84, "Hi.")])
        lo = DATA + u32(original, 4 * 0x84)
        hi = DATA + u32(original, 4 * 0x85)
        diff = changed(original, out)
        self.assertTrue(diff)
        self.assertTrue(all(lo <= i < hi for i in diff))

    def test_an_over_long_string_is_an_error_and_writes_nothing(self):
        with self.assertRaises(PatchError) as ctx:
            self.patch(SDMENU, [StringReplacement(0x84, "Far too long for the Options slot.")], name="bad")
        self.assertIn("0x84", str(ctx.exception))
        self.assertFalse((self.tmp / "bad").exists())

    def test_the_wrong_number_of_lines_is_an_error(self):
        with self.assertRaises(PatchError):
            self.patch(SDMENU, [StringReplacement(0x84, "one\ntwo")])

    def test_text_with_no_menu_glyph_is_an_error(self):
        with self.assertRaises(PatchError):
            self.patch(SDMENU, [StringReplacement(0x84, "100%")])

    def test_a_long_description_spills_into_unused_entries(self):
        original = SDMENU.read_bytes()
        rep = StringReplacement(
            0x81,
            "Learn to tech chase on reaction,\none step at a time.",
            template=0x8D,
            spill=(0x86, 0x88),
        )
        out = self.patch(SDMENU, [rep])
        self.assertEqual(
            read_sis_lines(out, 0x81), ["Learn to tech chase on reaction,", "one step at a time."]
        )
        # Entries outside the spill range still read as they did.
        for i in (0x80, 0x82, 0x83, 0x84, 0x85, 0x89, 0x8A):
            self.assertEqual(read_sis_lines(out, i), read_sis_lines(original, i))
        # Only the entry's slot, the donors' slots and the table pointers moved.
        allowed = set()
        for i in (0x81, 0x86, 0x87, 0x88):
            allowed |= set(range(DATA + u32(original, 4 * i), DATA + u32(original, 4 * i + 4)))
            allowed |= set(range(DATA + 4 * i, DATA + 4 * i + 4))
        self.assertEqual(len(out), len(original))
        self.assertTrue(set(changed(original, out)) <= allowed)

    def test_a_layout_can_be_copied_from_the_english_archive_for_the_japanese_one(self):
        dat = ORIG_FILES / "SdMenu.dat"
        rep = StringReplacement(0x81, "One.\nTwo.", template=("SdMenu.usd", 0x8D), spill=(0x86, 0x88))
        out = self.patch(dat, [rep])
        self.assertEqual(read_sis_lines(out, 0x81), ["One.", "Two."])

    def test_a_long_string_without_room_is_an_error(self):
        rep = StringReplacement(0x81, "x" * 200 + "\n.", template=0x8D, spill=(0x86, 0x86))
        with self.assertRaises(PatchError):
            self.patch(SDMENU, [rep])


@requires_archives
class CommittedPatchesTests(ArchivePatcherCase):
    def test_every_committed_patch_applies_to_the_disc_archives(self):
        from tools.archive_patches import ARCHIVE_PATCHES

        for name, replacements in ARCHIVE_PATCHES.items():
            src = ORIG_FILES / name
            if not src.is_file():
                continue
            with self.subTest(archive=name):
                out = self.patch(src, replacements, name=name)
                self.assertEqual(len(out), src.stat().st_size)
                self.assertNotEqual(out, src.read_bytes())


@requires_archives
class PatchedFileTests(ArchivePatcherCase):
    def test_the_source_is_never_written_and_the_copy_is_a_regular_file(self):
        before = (sha1(MNMA), sha1(SDMENU))
        dest = self.tmp / "copy"
        patch_archive(MNMA, dest, [])
        self.assertTrue(dest.is_file() and not dest.is_symlink())
        patch_archive(SDMENU, self.tmp / "copy2", [StringReplacement(0x84, "Hi.")])
        self.assertEqual((sha1(MNMA), sha1(SDMENU)), before)

    def test_an_existing_link_at_the_destination_is_replaced_not_followed(self):
        victim = self.tmp / "victim"
        shutil.copyfile(MNMA, victim)
        link = self.tmp / "link"
        try:
            link.hardlink_to(victim)
        except OSError:
            self.skipTest("cannot make hard links here")
        patch_archive(MNMA, link, [label(checker(176, 30))])
        self.assertEqual(victim.read_bytes(), MNMA.read_bytes())
        self.assertNotEqual(link.read_bytes(), MNMA.read_bytes())

    def test_the_source_cannot_be_the_destination(self):
        copy = self.tmp / "copy"
        shutil.copyfile(MNMA, copy)
        with self.assertRaises(PatchError):
            patch_archive(copy, copy, [])


def write_rgba_png(path, image):
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    raw = b""
    for y, row in enumerate(image.pixels):
        # Alternate filter types so the reader's filters are exercised.
        ftype = y % 5
        line = bytearray(b"".join(bytes(px) for px in row))
        prev = bytearray(b"".join(bytes(px) for px in image.pixels[y - 1])) if y else bytearray(len(line))
        out = bytearray()
        for i, v in enumerate(line):
            a = line[i - 4] if i >= 4 else 0
            b = prev[i]
            c = prev[i - 4] if i >= 4 else 0
            if ftype == 0:
                pred = 0
            elif ftype == 1:
                pred = a
            elif ftype == 2:
                pred = b
            elif ftype == 3:
                pred = (a + b) // 2
            else:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
            out.append((v - pred) & 255)
        raw += bytes([ftype]) + bytes(out)
    ihdr = struct.pack(">IIBBBBB", image.width, image.height, 8, 6, 0, 0, 0)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b"")
    )


if __name__ == "__main__":
    unittest.main()
