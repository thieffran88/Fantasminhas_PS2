#!/usr/bin/env python3
"""Converte assets/sprites/*.png em src/sprites_data.c e src/sprites_ids.h.

Cada sprite é redimensionado para o tamanho exato em que aparece na tela
(o PS2 só copia pixels, sem escalar), reduzido a uma paleta de até 256 cores
e comprimido com RLE (cor e alfa em fluxos separados).

Uso: python3 tools/gen_sprites.py
"""
import os

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
SPR = os.path.join(ROOT, "assets", "sprites")

LM = ["casa", "arvore", "lago", "loja"]  # mesma ordem de CASA, ARVORE, LAGO, LOJA
CHARS = ["prefeito", "zaca", "nina", "lili", "beto", "nena"]
EXPRS = ["normal", "feliz", "animado", "surpreso", "pensativo", "preocupado", "confiante", "triste", "rindo"]

# (ID em C, arquivo, ("w"|"h", tamanho))
TABLE = [
    ("CAPA", "capa", ("w", 640)),
    ("BG_NOITE", "bg_noite", ("w", 640)),
]
for c in CHARS:
    TABLE.append((f"AVATAR_{c.upper()}", f"char_{c}", ("h", 66)))
for e in EXPRS:
    TABLE.append((f"EXPR_{e.upper()}", f"expr_{e}", ("h", 38)))
for e in ["feliz", "triste", "rindo", "pensativo"]:
    TABLE.append((f"EXPR_BIG_{e.upper()}", f"expr_{e}", ("h", 80)))
# marcos cabem numa caixa (largura, altura): o lago é largo e fica mais baixo
for size, box in ((18, (22, 18)), (24, (30, 24)), (34, (42, 34)), (56, (68, 56))):
    for name in LM:
        TABLE.append((f"LM_{name.upper()}_{size}", name, ("box", box)))
for i in range(6):
    TABLE.append((f"FESTA{i}", f"festa{i}", ("h", 72)))
TABLE += [
    ("FX_EXCLAMACAO", "fx_exclamacao", ("h", 26)),
    ("FX_PERGUNTA", "fx_pergunta", ("h", 26)),
    ("FX_BRILHO", "fx_brilho", ("h", 24)),
    ("FX_LAMPADA", "fx_lampada", ("h", 28)),
    ("FX_ESTRELAS", "fx_estrelas", ("h", 34)),
    ("FX_VIBRA", "fx_vibra", ("h", 28)),
]


def rle(values):
    out = bytearray()
    i, n = 0, len(values)
    while i < n:
        v = values[i]
        j = i
        while j < n and values[j] == v and j - i < 255:
            j += 1
        out += bytes((j - i, v))
        i = j
    return out


def fit(img, mode, size):
    if mode == "box":
        k = min(size[0] / img.width, size[1] / img.height)
        w, h = max(1, round(img.width * k)), max(1, round(img.height * k))
    elif mode == "w":
        w, h = size, max(1, round(img.height * size / img.width))
    else:
        w, h = max(1, round(img.width * size / img.height)), size
    # redimensiona com alfa pré-multiplicado para não sujar as bordas
    return img.convert("RGBa").resize((w, h), Image.LANCZOS).convert("RGBA")


def encode(img):
    w, h = img.size
    alpha = list(img.getchannel("A").tobytes())
    rgb = Image.new("RGB", img.size, (0, 0, 0))
    rgb.paste(img.convert("RGB"), mask=img.getchannel("A").point(lambda a: 255 if a else 0))
    q = rgb.quantize(colors=256, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = q.getpalette()[: 256 * 3]
    idx = list(q.tobytes())
    for k in range(len(idx)):
        if alpha[k] == 0:
            idx[k] = 0
    ncol = max(idx) + 1
    colors = [(pal[3 * i], pal[3 * i + 1], pal[3 * i + 2]) for i in range(ncol)]
    return w, h, colors, rle(idx), rle(alpha)


def carr(name, data):
    lines = [f"static const unsigned char {name}[{len(data)}] = {{"]
    for i in range(0, len(data), 40):
        lines.append("  " + ",".join(str(b) for b in data[i:i + 40]) + ",")
    lines.append("};")
    return lines


def main():
    cache = {}
    c = ["/* Gerado por tools/gen_sprites.py a partir de assets/sprites - nao editar. */",
         '#include "sprites.h"', ""]
    entries = []
    total = 0
    for cid, fname, (mode, size) in TABLE:
        if fname not in cache:
            cache[fname] = Image.open(os.path.join(SPR, fname + ".png")).convert("RGBA")
        w, h, colors, ri, ra = encode(fit(cache[fname], mode, size))
        low = cid.lower()
        c.append(f"static const unsigned int {low}_pal[{len(colors)}] = {{")
        c.append("  " + ",".join("0x%06X" % ((r << 16) | (g << 8) | b) for r, g, b in colors) + ",")
        c.append("};")
        c += carr(f"{low}_idx", ri)
        c += carr(f"{low}_alpha", ra)
        entries.append(f"  {{ {w}, {h}, {low}_pal, {low}_idx, {len(ri)}, {low}_alpha, {len(ra)} }},")
        total += len(ri) + len(ra) + 4 * len(colors)
    c.append(f"const SpriteSrc sprite_src[SPR_COUNT] = {{")
    c += entries
    c.append("};")
    with open(os.path.join(ROOT, "src", "sprites_data.c"), "w") as f:
        f.write("\n".join(c) + "\n")

    hdr = ["/* Gerado por tools/gen_sprites.py - nao editar. */", "#ifndef SPRITES_IDS_H",
           "#define SPRITES_IDS_H", "", "enum {"]
    for cid, _, _ in TABLE:
        hdr.append(f"    SPR_{cid},")
    hdr += ["    SPR_COUNT", "};", "", "#endif", ""]
    with open(os.path.join(ROOT, "src", "sprites_ids.h"), "w") as f:
        f.write("\n".join(hdr))
    print(f"{len(TABLE)} sprites, {total // 1024} KB comprimidos")


if __name__ == "__main__":
    main()
