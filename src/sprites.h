#ifndef SPRITES_H
#define SPRITES_H

#include "sprites_ids.h"

/* Dados comprimidos (gerados por tools/gen_sprites.py) */
typedef struct {
    short w, h;
    const unsigned int *pal;      /* cores 0xRRGGBB */
    const unsigned char *idx;     /* RLE (quantidade, índice da cor) */
    int idx_len;
    const unsigned char *alpha;   /* RLE (quantidade, alfa 0..255) */
    int alpha_len;
} SpriteSrc;

/* Sprite descomprimido: pixels no formato da tela com o alfa (0..255) no byte alto */
typedef struct {
    int w, h;
    unsigned int *px;
} Sprite;

extern const SpriteSrc sprite_src[SPR_COUNT];

void sprites_init(void);  /* descomprime tudo (chamar uma vez) */
const Sprite *spr(int id);

/* desenha com o canto superior esquerdo em (x, y) */
void spr_draw(int id, int x, int y, int alpha);
/* desenha centralizado em x e com a base em y (bom para personagens) */
void spr_draw_base(int id, int cx, int base, int alpha);
void spr_draw_center(int id, int cx, int cy, int alpha);

#endif
