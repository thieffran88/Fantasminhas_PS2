#!/usr/bin/env python3
"""Gera src/font_data.c a partir de fontes TrueType/OpenType.

Cada glifo vira um mapa de alfa de 4 bits (2 pixels por byte), o que deixa o
texto suavizado (antialiasing) sem pesar no executável do PS2.

Uso: python3 tools/gen_font.py [diretorio_das_fontes]
Fonte padrão: Inter (SIL Open Font License 1.1).
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

FONT_DIR = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/opentype/inter"

# (nome C, arquivo, tamanho em px)
SIZES = [
    ("small", "Inter-SemiBold.otf", 14),
    ("body", "Inter-SemiBold.otf", 18),
    ("big", "Inter-Bold.otf", 26),
    ("title", "InterDisplay-Black.otf", 46),
]

# ASCII imprimível + Latin-1 (acentos do português)
CHARS = list(range(32, 127)) + list(range(160, 256))
FIRST, LAST = 32, 255

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "src", "font_data.c")


def render_font(fname, size):
    font = ImageFont.truetype(os.path.join(FONT_DIR, fname), size)
    ascent, descent = font.getmetrics()
    glyphs = {}
    blob = bytearray()
    for code in range(FIRST, LAST + 1):
        if code not in CHARS:
            glyphs[code] = (0, 0, 0, 0, 0, 0)
            continue
        ch = chr(code)
        adv = int(round(font.getlength(ch)))
        bbox = font.getbbox(ch)  # relativo à origem (topo da linha)
        x0, y0, x1, y1 = bbox
        w, h = max(0, x1 - x0), max(0, y1 - y0)
        if w == 0 or h == 0:
            glyphs[code] = (len(blob), 0, 0, 0, 0, adv)
            continue
        img = Image.new("L", (w, h), 0)
        ImageDraw.Draw(img).text((-x0, -y0), ch, font=font, fill=255)
        px = img.load()
        off = len(blob)
        for y in range(h):
            row = [px[x, y] >> 4 for x in range(w)]
            if w % 2:
                row.append(0)
            for i in range(0, len(row), 2):
                blob.append(row[i] | (row[i + 1] << 4))
        glyphs[code] = (off, w, h, x0, y0, adv)
    return ascent, descent, glyphs, blob


def main():
    lines = [
        "/* Arquivo gerado por tools/gen_font.py - nao editar a mao. */",
        "/* Fonte: Inter, (c) The Inter Project Authors, SIL OFL 1.1. */",
        '#include "font.h"',
        "",
    ]
    for name, fname, size in SIZES:
        ascent, descent, glyphs, blob = render_font(fname, size)
        lines.append(f"static const unsigned char {name}_bits[{len(blob)}] = {{")
        for i in range(0, len(blob), 32):
            lines.append("  " + ",".join(str(b) for b in blob[i:i + 32]) + ",")
        lines.append("};")
        lines.append(f"static const Glyph {name}_glyphs[{LAST - FIRST + 1}] = {{")
        for code in range(FIRST, LAST + 1):
            off, w, h, bx, by, adv = glyphs[code]
            lines.append(f"  {{{off},{w},{h},{bx},{by},{adv}}},")
        lines.append("};")
        lines.append(
            f"const Font font_{name} = {{ {size}, {ascent}, {descent}, {name}_glyphs, {name}_bits }};"
        )
        lines.append("")
    with open(OUT, "w", encoding="ascii") as f:
        f.write("\n".join(lines))
    print("ok:", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
