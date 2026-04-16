"""Generate a stylized cute-squirrel app icon (ICO with multiple sizes)."""
from PIL import Image, ImageDraw
import os

OUT = os.path.join(os.path.dirname(__file__), "icons", "app.ico")
os.makedirs(os.path.dirname(OUT), exist_ok=True)

BROWN_DARK  = (0x6E, 0x45, 0x21, 255)
BROWN       = (0xB0, 0x6F, 0x37, 255)
BROWN_LIGHT = (0xD8, 0x9C, 0x60, 255)
CREAM       = (0xFB, 0xE8, 0xCC, 255)
NUT_BROWN   = (0x80, 0x4A, 0x1E, 255)
NUT_HI      = (0xC8, 0x8A, 0x4A, 255)
EYE         = (0x10, 0x10, 0x10, 255)
WHITE       = (0xFF, 0xFF, 0xFF, 255)
NOSE        = (0x33, 0x1A, 0x0E, 255)
CHEEK       = (0xF2, 0xA8, 0x9A, 255)
BG_TOP      = (0xF6, 0xE7, 0xC9, 255)
BG_BOT      = (0xE3, 0xC9, 0x9F, 255)


def draw_squirrel(size: int) -> Image.Image:
    s = size
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Soft circular background
    pad = max(1, s // 32)
    for i in range(s):
        t = i / max(1, s - 1)
        r = int(BG_TOP[0] * (1 - t) + BG_BOT[0] * t)
        g = int(BG_TOP[1] * (1 - t) + BG_BOT[1] * t)
        b = int(BG_TOP[2] * (1 - t) + BG_BOT[2] * t)
        d.line([(0, i), (s, i)], fill=(r, g, b, 255))
    # Round mask
    mask = Image.new("L", (s, s), 0)
    ImageDraw.Draw(mask).ellipse([pad, pad, s - pad, s - pad], fill=255)
    rounded = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    rounded.paste(img, mask=mask)
    img = rounded
    d = ImageDraw.Draw(img)

    def f(x):
        return int(round(x * s / 64.0))

    # Tail (big curl behind body)
    tail_box = [f(34), f(8), f(62), f(46)]
    d.ellipse(tail_box, fill=BROWN_DARK)
    d.ellipse([f(38), f(12), f(58), f(42)], fill=BROWN)
    d.ellipse([f(42), f(18), f(54), f(36)], fill=BROWN_LIGHT)

    # Body
    d.ellipse([f(14), f(28), f(46), f(58)], fill=BROWN)
    # Belly
    d.ellipse([f(20), f(36), f(38), f(56)], fill=CREAM)

    # Hind foot
    d.ellipse([f(16), f(50), f(28), f(60)], fill=BROWN_DARK)

    # Head
    d.ellipse([f(8), f(12), f(36), f(40)], fill=BROWN)
    # Cheek lighter
    d.ellipse([f(11), f(20), f(22), f(34)], fill=BROWN_LIGHT)
    d.ellipse([f(22), f(20), f(33), f(34)], fill=BROWN_LIGHT)

    # Ears
    d.polygon([(f(9), f(13)), (f(13), f(6)), (f(17), f(14))], fill=BROWN_DARK)
    d.polygon([(f(27), f(14)), (f(31), f(6)), (f(35), f(13))], fill=BROWN_DARK)
    d.polygon([(f(11), f(13)), (f(14), f(9)), (f(16), f(14))], fill=CHEEK)
    d.polygon([(f(28), f(14)), (f(31), f(9)), (f(33), f(13))], fill=CHEEK)

    # Eyes (whites + pupils + highlight)
    d.ellipse([f(13), f(20), f(20), f(27)], fill=WHITE)
    d.ellipse([f(24), f(20), f(31), f(27)], fill=WHITE)
    d.ellipse([f(15), f(22), f(19), f(26)], fill=EYE)
    d.ellipse([f(26), f(22), f(30), f(26)], fill=EYE)
    if s >= 32:
        d.ellipse([f(16), f(22), f(17), f(23)], fill=WHITE)
        d.ellipse([f(27), f(22), f(28), f(23)], fill=WHITE)

    # Cheek blush
    d.ellipse([f(11), f(28), f(15), f(31)], fill=CHEEK)
    d.ellipse([f(29), f(28), f(33), f(31)], fill=CHEEK)

    # Nose
    d.polygon([(f(20), f(28)), (f(24), f(28)), (f(22), f(31))], fill=NOSE)
    # Mouth (small smile)
    if s >= 24:
        d.arc([f(20), f(30), f(24), f(34)], 0, 180, fill=NOSE, width=max(1, s // 64))

    # Nut (acorn) in front paws
    d.ellipse([f(24), f(40), f(36), f(52)], fill=NUT_BROWN)
    d.chord([f(24), f(38), f(36), f(46)], 180, 360, fill=NUT_BROWN)
    d.rectangle([f(28), f(36), f(32), f(40)], fill=BROWN_DARK)
    d.ellipse([f(27), f(43), f(31), f(47)], fill=NUT_HI)

    # Front paws hugging the nut
    d.ellipse([f(20), f(42), f(27), f(50)], fill=BROWN_DARK)
    d.ellipse([f(33), f(42), f(40), f(50)], fill=BROWN_DARK)

    return img


sizes = [16, 20, 24, 32, 40, 48, 64, 128, 256]
imgs = [draw_squirrel(s) for s in sizes]

# Pillow's ICO saver silently collapses multi-resolution writes, so emit the
# ICONDIR/ICONDIRENTRY headers by hand and embed each frame as PNG (Windows
# supports PNG-compressed ICO entries and it keeps the file compact).
import io, struct
frames = []
for im in imgs:
    buf = io.BytesIO()
    im.save(buf, format="PNG", optimize=True)
    frames.append(buf.getvalue())

header = struct.pack("<HHH", 0, 1, len(frames))
entries = bytearray()
offset = 6 + 16 * len(frames)
for im, data in zip(imgs, frames):
    w = im.width if im.width < 256 else 0
    h = im.height if im.height < 256 else 0
    entries += struct.pack("<BBBBHHII", w, h, 0, 0, 1, 32, len(data), offset)
    offset += len(data)

with open(OUT, "wb") as f:
    f.write(header)
    f.write(entries)
    for data in frames:
        f.write(data)
print("wrote", OUT, "with sizes", sizes)
