#!/usr/bin/env python3
"""Recorta os sprites das folhas em assets/source/ e salva PNGs com fundo
transparente em assets/sprites/.

Uso: python3 tools/extract_sprites.py
(precisa de Pillow e numpy)

O fundo branco é removido por "inundação" a partir das bordas do recorte:
o branco de dentro dos fantasminhas fica intacto porque o contorno preto
fecha a figura.
"""
import os
from collections import deque

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
SRC = os.path.join(ROOT, "assets", "source")
OUT = os.path.join(ROOT, "assets", "sprites")

# nome: (folha, x0, y0, x1, y1, remover_fundo)
# "busto": o corpo continua abaixo do recorte, então a inundação não começa
# pela borda de baixo e a base ganha um degradê.
CROPS = {
    "logo": ("folha1", 15, 5, 277, 190, True),
    # personagens (sem a plaquinha de nome)
    "char_prefeito": ("folha1", 286, 8, 472, 194, True),
    "char_zaca": ("folha1", 481, 38, 651, 194, True),
    "char_nina": ("folha1", 667, 34, 815, 194, True),
    "char_lili": ("folha1", 825, 33, 960, 194, True),
    "char_beto": ("folha1", 970, 36, 1099, 194, True),
    "char_nena": ("folha1", 1121, 23, 1254, 194, True),
    # expressões do prefeito (sem a etiqueta)
    "expr_normal": ("folha1", 701, 302, 790, 369, "busto"),
    "expr_feliz": ("folha1", 801, 304, 885, 369, "busto"),
    "expr_animado": ("folha1", 897, 303, 986, 369, "busto"),
    "expr_surpreso": ("folha1", 700, 410, 795, 473, "busto"),
    "expr_pensativo": ("folha1", 801, 406, 892, 473, "busto"),
    "expr_preocupado": ("folha1", 900, 409, 993, 473, "busto"),
    "expr_confiante": ("folha1", 701, 513, 794, 573, "busto"),
    "expr_triste": ("folha1", 804, 515, 894, 573, "busto"),
    "expr_rindo": ("folha1", 901, 511, 991, 573, "busto"),
    # marcos
    "casa": ("folha1", 1021, 327, 1121, 409, True),
    "loja": ("folha1", 1129, 323, 1229, 408, True),
    "lago": ("folha1", 1239, 340, 1356, 407, True),
    "arvore": ("folha1", 1385, 303, 1505, 430, True),
    # comemorando (6 quadros)
    "festa0": ("folha1", 326, 481, 375, 535, True),
    "festa1": ("folha1", 382, 482, 429, 535, True),
    "festa2": ("folha1", 436, 482, 482, 534, True),
    "festa3": ("folha1", 494, 481, 539, 534, True),
    "festa4": ("folha1", 550, 480, 597, 533, True),
    "festa5": ("folha1", 606, 481, 649, 533, True),
    # efeitos
    "fx_exclamacao": ("folha1", 1102, 870, 1137, 924, True),
    "fx_pergunta": ("folha1", 1143, 868, 1184, 924, True),
    "fx_brilho": ("folha1", 1187, 869, 1239, 910, True),
    "fx_lampada": ("folha1", 1245, 867, 1311, 935, True),
    "fx_estrelas": ("folha1", 1389, 874, 1443, 934, True),
    "fx_vibra": ("folha1", 1313, 933, 1396, 982, True),
    # cenário
    "bg_noite": ("folha1", 26, 870, 392, 940, False),
    "bg_rua": ("folha1", 419, 866, 684, 944, False),
}


def remove_background(img, busto=False):
    a = np.asarray(img.convert("RGB")).astype(np.int32)
    h, w, _ = a.shape
    mn = a.min(axis=2)
    mx = a.max(axis=2)
    whiteish = (mn > 200) & (mx - mn < 40)
    bg = np.zeros((h, w), bool)
    q = deque()
    for x in range(w):
        for y in ((0,) if busto else (0, h - 1)):
            if whiteish[y, x] and not bg[y, x]:
                bg[y, x] = True
                q.append((y, x))
    for y in range(h):
        for x in (0, w - 1):
            if whiteish[y, x] and not bg[y, x]:
                bg[y, x] = True
                q.append((y, x))
    while q:
        y, x = q.popleft()
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ny, nx = y + dy, x + dx
            if 0 <= ny < h and 0 <= nx < w and not bg[ny, nx] and whiteish[ny, nx]:
                bg[ny, nx] = True
                q.append((ny, nx))
    alpha = np.where(bg, 0, 255).astype(np.int32)
    # borda suave: pixels claros encostados no fundo ficam semitransparentes
    near = np.zeros_like(bg)
    near[1:, :] |= bg[:-1, :]
    near[:-1, :] |= bg[1:, :]
    near[:, 1:] |= bg[:, :-1]
    near[:, :-1] |= bg[:, 1:]
    edge = near & ~bg
    soft = np.clip((255 - mn) * 255 // 200, 0, 255)
    alpha = np.where(edge, np.minimum(alpha, soft), alpha)
    if busto:
        fade = 8
        for i in range(fade):
            alpha[h - 1 - i, :] = alpha[h - 1 - i, :] * (i + 1) // (fade + 1)
    rgba = np.dstack([a, alpha]).astype(np.uint8)
    out = Image.fromarray(rgba, "RGBA")
    box = out.getbbox()
    return out.crop(box) if box else out


def main():
    os.makedirs(OUT, exist_ok=True)
    sheets = {}
    for name, (sheet, x0, y0, x1, y1, rm) in CROPS.items():
        if sheet not in sheets:
            sheets[sheet] = Image.open(os.path.join(SRC, sheet + ".png")).convert("RGB")
        crop = sheets[sheet].crop((x0, y0, x1, y1))
        img = remove_background(crop, rm == "busto") if rm else crop.convert("RGBA")
        img.save(os.path.join(OUT, name + ".png"))
    print(len(CROPS), "sprites em", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
