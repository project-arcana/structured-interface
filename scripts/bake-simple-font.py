import freetype
import base64
import math

c_min = 0x20
c_max = 0x7E

fsize = 20 * 64

face = freetype.Face("fonts/FiraMono-Medium.ttf")
face.set_char_size(fsize)
face.load_char('S')
bitmap = face.glyph.bitmap
# print(bitmap)

max_h = 0
max_w = 0

for ci in range(c_min, c_max + 1):
    c = chr(ci)
    # print(chr(c))
    face.load_char(c)
    img = face.glyph.bitmap
    max_h = max(max_h, img.rows)
    max_w = max(max_w, img.width)

print(f"max h: {max_h}")
print(f"max w: {max_w}")

atlas_w = 0
atlas_h = max_h

glyphs = []

atlas = []
for y in range(max_h):
    # 2px white
    atlas.append(255)
    atlas.append(255)
    x = 2

    print(f"row {y}")

    for ci in range(c_min, c_max + 1):
        c = chr(ci)
        face.load_char(c)
        img = face.glyph.bitmap
        atlas.append(0)
        x += 1
        w = img.width
        for sx in range(w):
            if y >= img.rows:
                b = 0
            else:
                b = img.buffer[sx + y * img.pitch]
            atlas.append(b)
            x += 1

        if y == 1:  # atlas_w is valid
            glyphs.append({
                "c": c,
                "adv": face.glyph.advance.x / 64,
                "u_min": (x - w) / atlas_w,
                "u_max": (x - 0) / atlas_w,
                "v_min": 0,
                "v_max": img.rows / atlas_h,
                "bx": face.glyph.bitmap_left,
                "by": face.glyph.bitmap_top,
                "w": w,
                "h": img.rows,
            })
            
        atlas.append(0)
        x += 1

    atlas_w = x

atlas_bytes = bytes(atlas)

print("write /tmp/atlas.ppm")

with open("/tmp/atlas.ppm", "w") as f:
    f.write("P2\n")
    f.write(f"{atlas_w} {atlas_h}\n")
    for y in range(atlas_h):
        s = ""
        for x in range(atlas_w):
            s += f"{atlas[y * atlas_w + x]} "
        s += "\n"
        f.write(s)

atlas_b64 = base64.b64encode(atlas_bytes).decode("utf-8")

print("write /tmp/font.cc")

code = ""
code += f"f.ref_size = {fsize / 64};\n"
code += f"f.ascender = {math.ceil(face.ascender / face.units_per_EM * face.size.y_ppem)};\n"
code += f"f.baseline_height = {math.ceil(face.height / face.units_per_EM * face.size.y_ppem)};\n"
code += f"f.width = {atlas_w};\n"
code += f"f.height = {atlas_h};\n"
code += "f.full_uv = {0, 0};\n"
code += "f.data = cc::base64_decode(\n"
i = 0
while i < len(atlas_b64):
    w = min(len(atlas_b64) - i, 140)
    code += f"  \"{atlas_b64[i:i+w]}\"\n"
    i += w
code += ");\n"
code += "f.glyphs.resize(0x7F);\n"

for g in glyphs:
    gs = "{"
    gs += "{{" + str(g["u_min"]) + "," + str(g["v_min"]) + \
        "},{" + str(g["u_max"]) + "," + str(g["v_max"]) + "}},"
    gs += str(g["bx"]) + ","
    gs += str(g["by"]) + ","
    gs += str(g["w"]) + ","
    gs += str(g["h"]) + ","
    gs += str(g["adv"])
    gs += "}"
    gc = g['c']
    if gc == "'":
        gc = "\\'"
    if gc == '\\':
        gc = "\\\\"
    code += f"f.glyphs['{gc}'] = {gs};\n"

with open("/tmp/font.cc", "w") as f:
    f.write(code)
