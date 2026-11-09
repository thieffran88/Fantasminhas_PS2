#include "sprites.h"

#include <stdlib.h>

#include "gfx.h"

static Sprite sprites[SPR_COUNT];

static void unrle(const unsigned char *src, int len, unsigned char *dst, int max)
{
    int i, n = 0;
    for (i = 0; i + 1 < len && n < max; i += 2) {
        int c = src[i], k;
        for (k = 0; k < c && n < max; k++)
            dst[n++] = src[i + 1];
    }
}

void sprites_init(void)
{
    int id;
    for (id = 0; id < SPR_COUNT; id++) {
        const SpriteSrc *s = &sprite_src[id];
        int n = s->w * s->h, i;
        unsigned char *idx = (unsigned char *)malloc((size_t)n * 2);
        unsigned char *al = idx + n;
        u32 *px = (u32 *)malloc((size_t)n * 4);
        unrle(s->idx, s->idx_len, idx, n);
        unrle(s->alpha, s->alpha_len, al, n);
        for (i = 0; i < n; i++) {
            unsigned int rgb = s->pal[idx[i]];
            u32 p = PIX(rgb) & 0x00FFFFFFu;
            px[i] = p | ((u32)al[i] << 24);
        }
        free(idx);
        sprites[id].w = s->w;
        sprites[id].h = s->h;
        sprites[id].px = px;
    }
}

const Sprite *spr(int id) { return &sprites[id]; }

void spr_draw(int id, int x, int y, int alpha)
{
    const Sprite *s = &sprites[id];
    gfx_blit(s->px, s->w, s->h, x, y, alpha);
}

void spr_draw_base(int id, int cx, int base, int alpha)
{
    const Sprite *s = &sprites[id];
    gfx_blit(s->px, s->w, s->h, cx - s->w / 2, base - s->h, alpha);
}

void spr_draw_center(int id, int cx, int cy, int alpha)
{
    const Sprite *s = &sprites[id];
    gfx_blit(s->px, s->w, s->h, cx - s->w / 2, cy - s->h / 2, alpha);
}
