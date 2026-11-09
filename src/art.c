#include "art.h"

#include <math.h>

#include "game.h"
#include "gfx.h"

void art_init(void)
{
    /* cores dos marcos quando aparecem no texto (sobre fundo claro) */
    gfx_lm_colors[CASA] = 0x7A45C2;
    gfx_lm_colors[ARVORE] = 0x24934A;
    gfx_lm_colors[LAGO] = 0x1C78CC;
    gfx_lm_colors[LOJA] = 0xD2601A;
}

static unsigned int bg_rng = 0x9E3779B9u;
static unsigned int rnd(void)
{
    bg_rng ^= bg_rng << 13;
    bg_rng ^= bg_rng >> 17;
    bg_rng ^= bg_rng << 5;
    return bg_rng;
}

void art_background(void)
{
    int i;
    bg_rng = 0x9E3779B9u;
    gfx_vgrad(0, 0, SCR_W, SCR_H, 0x120A26, 0x3B2350);
    /* estrelas */
    for (i = 0; i < 90; i++) {
        int x = (int)(rnd() % SCR_W), y = (int)(rnd() % 300);
        int a = 90 + (int)(rnd() % 150);
        gfx_fill(x, y, 1 + (int)(rnd() % 3 == 0), 1, 0xFFF6D8, a);
    }
    /* lua com brilho */
    for (i = 6; i >= 1; i--)
        gfx_circle(560, 70, 34 + i * 9, 0xC8B8FF, 10);
    gfx_circle(560, 70, 36, 0xFFF4D6, 255);
    gfx_circle(548, 62, 7, 0xEFE0BC, 255);
    gfx_circle(572, 84, 5, 0xEFE0BC, 255);
    gfx_circle(566, 56, 4, 0xEFE0BC, 255);
    /* colinas */
    gfx_ellipse(120, 470, 260, 120, 0x2A1840, 255);
    gfx_ellipse(470, 480, 320, 130, 0x24143A, 255);
    /* casinhas em silhueta com janelas acesas */
    for (i = 0; i < 9; i++) {
        int x = 20 + i * 70 + (int)(rnd() % 20);
        int h = 26 + (int)(rnd() % 26);
        int base = 400 + (int)(rnd() % 20);
        gfx_fill(x, base - h, 34, h + 60, 0x1A0E2C, 255);
        gfx_tri((float)x - 5, (float)(base - h), (float)x + 39, (float)(base - h), (float)x + 17,
                (float)(base - h - 18), 0x1A0E2C, 255);
        gfx_fill(x + 8, base - h + 8, 6, 7, 0xFFD45A, 200);
        if (rnd() & 1)
            gfx_fill(x + 20, base - h + 8, 6, 7, 0xFFD45A, 160);
    }
    gfx_fill(0, 430, SCR_W, 18, 0x150A24, 255);
}

/* ------------------------------------------------------------------ */

static void house(float cx, float cy, float s, int a)
{
    gfx_ellipse(cx, cy + 0.42f * s, 0.42f * s, 0.07f * s, 0x000000, a / 4);
    gfx_fill((int)(cx - 0.32f * s), (int)(cy - 0.06f * s), (int)(0.64f * s), (int)(0.48f * s), 0xB9A6DD, a);
    gfx_fill((int)(cx + 0.12f * s), (int)(cy - 0.06f * s), (int)(0.2f * s), (int)(0.48f * s), 0x9F89C9, a);
    gfx_tri(cx - 0.46f * s, cy, cx + 0.46f * s, cy, cx, cy - 0.44f * s, 0x553489, a);
    gfx_fill((int)(cx - 0.08f * s), (int)(cy + 0.16f * s), (int)(0.16f * s), (int)(0.26f * s), 0x3B2760, a);
    gfx_fill((int)(cx - 0.26f * s), (int)(cy + 0.06f * s), (int)(0.13f * s), (int)(0.13f * s), 0xFFE27A, a);
    gfx_fill((int)(cx + 0.14f * s), (int)(cy + 0.06f * s), (int)(0.13f * s), (int)(0.13f * s), 0xFFE27A, a);
}

static void tree(float cx, float cy, float s, int a)
{
    gfx_ellipse(cx, cy + 0.44f * s, 0.34f * s, 0.07f * s, 0x000000, a / 4);
    gfx_fill((int)(cx - 0.06f * s), (int)(cy + 0.04f * s), (int)(0.13f * s), (int)(0.4f * s), 0x6B4A2E, a);
    gfx_circle(cx - 0.2f * s, cy + 0.02f * s, 0.22f * s, 0x237A40, a);
    gfx_circle(cx + 0.2f * s, cy + 0.02f * s, 0.22f * s, 0x237A40, a);
    gfx_circle(cx, cy - 0.16f * s, 0.29f * s, 0x2E9E50, a);
    gfx_circle(cx - 0.08f * s, cy - 0.24f * s, 0.12f * s, 0x6BD487, a);
}

static void lake(float cx, float cy, float s, int a)
{
    gfx_ellipse(cx, cy + 0.12f * s, 0.48f * s, 0.27f * s, 0x16508E, a);
    gfx_ellipse(cx, cy + 0.1f * s, 0.42f * s, 0.21f * s, 0x2F8FE0, a);
    gfx_ellipse(cx - 0.12f * s, cy + 0.05f * s, 0.15f * s, 0.035f * s, 0xBFE6FF, a);
    gfx_ellipse(cx + 0.16f * s, cy + 0.16f * s, 0.1f * s, 0.03f * s, 0xBFE6FF, a);
}

static void shop(float cx, float cy, float s, int a)
{
    int i;
    float top = cy - 0.24f * s, bot = cy - 0.02f * s;
    float l = cx - 0.44f * s, w = 0.88f * s;
    gfx_ellipse(cx, cy + 0.42f * s, 0.42f * s, 0.07f * s, 0x000000, a / 4);
    gfx_fill((int)(cx - 0.36f * s), (int)(cy - 0.1f * s), (int)(0.72f * s), (int)(0.52f * s), 0xF28C38, a);
    gfx_fill((int)(cx - 0.36f * s), (int)(cy - 0.32f * s), (int)(0.72f * s), (int)(0.12f * s), 0xC9621C, a);
    /* toldo listrado */
    for (i = 0; i < 5; i++) {
        float x0 = l + w * i / 5, x1 = l + w * (i + 1) / 5;
        float t0 = cx + (x0 - cx) * 0.85f, t1 = cx + (x1 - cx) * 0.85f;
        gfx_quad(t0, top, t1, top, x1, bot, x0, bot, (i & 1) ? 0xFFF4E8 : 0xD8344F, a);
        gfx_circle((x0 + x1) * 0.5f, bot, w / 10.0f, (i & 1) ? 0xFFF4E8 : 0xD8344F, a);
    }
    gfx_fill((int)(cx - 0.26f * s), (int)(cy + 0.14f * s), (int)(0.2f * s), (int)(0.16f * s), 0xFFE27A, a);
    gfx_fill((int)(cx + 0.06f * s), (int)(cy + 0.12f * s), (int)(0.17f * s), (int)(0.3f * s), 0x7A3A12, a);
}

void art_landmark(int type, float cx, float cy, float s, int alpha)
{
    switch (type) {
    case CASA: house(cx, cy, s, alpha); break;
    case ARVORE: tree(cx, cy, s, alpha); break;
    case LAGO: lake(cx, cy, s, alpha); break;
    case LOJA: shop(cx, cy, s, alpha); break;
    }
}

void art_ghost(float cx, float cy, float s, int hat, int a)
{
    float hr = 0.42f * s;
    gfx_ellipse(cx, cy + 0.62f * s, 0.32f * s, 0.07f * s, 0x000000, a / 3);
    /* corpo */
    gfx_circle(cx, cy - 0.1f * s, hr, 0xF7F4FF, a);
    gfx_fill((int)(cx - hr), (int)(cy - 0.1f * s), (int)(2 * hr + 0.5f), (int)(0.5f * s), 0xF7F4FF, a);
    gfx_circle(cx - 0.28f * s, cy + 0.4f * s, 0.14f * s, 0xF7F4FF, a);
    gfx_circle(cx, cy + 0.42f * s, 0.14f * s, 0xF7F4FF, a);
    gfx_circle(cx + 0.28f * s, cy + 0.4f * s, 0.14f * s, 0xF7F4FF, a);
    gfx_ellipse(cx + 0.24f * s, cy + 0.05f * s, 0.12f * s, 0.32f * s, 0xDCD4F0, a / 2);
    /* rosto */
    gfx_ellipse(cx - 0.15f * s, cy - 0.14f * s, 0.07f * s, 0.1f * s, COL_INK, a);
    gfx_ellipse(cx + 0.15f * s, cy - 0.14f * s, 0.07f * s, 0.1f * s, COL_INK, a);
    gfx_circle(cx - 0.13f * s, cy - 0.17f * s, 0.025f * s, 0xFFFFFF, a);
    gfx_circle(cx + 0.17f * s, cy - 0.17f * s, 0.025f * s, 0xFFFFFF, a);
    gfx_ellipse(cx, cy + 0.04f * s, 0.1f * s, 0.07f * s, COL_INK, a);
    gfx_ellipse(cx, cy + 0.0f * s, 0.12f * s, 0.055f * s, 0xF7F4FF, a);
    gfx_circle(cx - 0.27f * s, cy + 0.0f * s, 0.06f * s, 0xFFA3C4, a / 2);
    gfx_circle(cx + 0.27f * s, cy + 0.0f * s, 0.06f * s, 0xFFA3C4, a / 2);
    if (hat) {
        /* cartola de prefeito */
        float by = cy - 0.44f * s;
        gfx_ellipse(cx, by, 0.36f * s, 0.07f * s, 0x1A1026, a);
        gfx_fill((int)(cx - 0.22f * s), (int)(by - 0.36f * s), (int)(0.44f * s), (int)(0.36f * s), 0x1A1026, a);
        gfx_fill((int)(cx - 0.22f * s), (int)(by - 0.1f * s), (int)(0.44f * s), (int)(0.07f * s + 1), 0x9C2E58, a);
    }
}

void art_star(float cx, float cy, float r, int rgb, int a)
{
    int i;
    float px[10], py[10];
    for (i = 0; i < 10; i++) {
        float ang = -1.5707963f + i * 0.6283185f;
        float rr = (i & 1) ? r * 0.45f : r;
        px[i] = cx + cosf(ang) * rr;
        py[i] = cy + sinf(ang) * rr;
    }
    for (i = 0; i < 10; i++) {
        int j = (i + 1) % 10;
        gfx_tri(cx, cy, px[i], py[i], px[j], py[j], rgb, a);
    }
}

void art_check(float cx, float cy, float s, int rgb)
{
    gfx_line(cx - 0.45f * s, cy, cx - 0.12f * s, cy + 0.32f * s, 0.22f * s, rgb, 255);
    gfx_line(cx - 0.12f * s, cy + 0.32f * s, cx + 0.48f * s, cy - 0.36f * s, 0.22f * s, rgb, 255);
}

void art_cross(float cx, float cy, float s, int rgb)
{
    gfx_line(cx - 0.35f * s, cy - 0.35f * s, cx + 0.35f * s, cy + 0.35f * s, 0.22f * s, rgb, 255);
    gfx_line(cx + 0.35f * s, cy - 0.35f * s, cx - 0.35f * s, cy + 0.35f * s, 0.22f * s, rgb, 255);
}

void art_status_badge(float cx, float cy, int status, int t)
{
    float pulse = 1.0f + 0.08f * sinf((float)t * 0.12f);
    switch (status) {
    case ST_MET:
        gfx_circle(cx, cy, 13, 0x0E2A18, 255);
        gfx_circle(cx, cy, 11, COL_OK, 255);
        art_check(cx, cy, 13, 0xFFFFFF);
        break;
    case ST_BROKEN:
        gfx_circle(cx, cy, 13 * pulse, 0x3A0A12, 255);
        gfx_circle(cx, cy, 11 * pulse, COL_BAD, 255);
        art_cross(cx, cy, 14, 0xFFFFFF);
        break;
    default:
        gfx_circle(cx, cy, 13, 0x3A2A10, 255);
        gfx_circle(cx, cy, 11, 0x6B4FA8, 255);
        art_star(cx, cy, 8, COL_GOLD, 255);
        break;
    }
}

void art_card_face(int x, int y, int w, int h, int card, int edge)
{
    const Card *c = &card_db[card];
    float s = (float)(w < h / 2 ? w : h / 2) * 0.95f;
    gfx_rrect(x + 2, y + 3, w, h, 5, 0x000000, 70);
    gfx_rrect_border(x, y, w, h, 5, 2, edge, COL_CARD, 255);
    art_landmark(c->lm[0], x + w * 0.5f, y + h * 0.27f, s, 255);
    art_landmark(c->lm[1], x + w * 0.5f, y + h * 0.71f, s, 255);
}

void art_card_back(int x, int y, int w, int h, int star)
{
    gfx_rrect(x + 2, y + 3, w, h, 6, 0x000000, 80);
    gfx_rrect_border(x, y, w, h, 6, 2, 0x8C6FC9, 0x3B2760, 255);
    gfx_rrect(x + 5, y + 5, w - 10, h - 10, 4, 0x4A3277, 255);
    if (star)
        art_star(x + w * 0.5f, y + h * 0.5f, w * 0.28f, COL_GOLD, 255);
    else
        art_ghost(x + w * 0.5f, y + h * 0.5f, w * 0.5f, 1, 230);
}

void art_button(int type, int x, int y)
{
    gfx_circle(x, y, 9, 0x140C22, 255);
    gfx_circle(x, y, 8, 0x3A2E52, 255);
    switch (type) {
    case BTN_ICON_CROSS:
        gfx_line(x - 3.5f, y - 3.5f, x + 3.5f, y + 3.5f, 2, 0x8FB8FF, 255);
        gfx_line(x + 3.5f, y - 3.5f, x - 3.5f, y + 3.5f, 2, 0x8FB8FF, 255);
        break;
    case BTN_ICON_CIRCLE:
        gfx_circle(x, y, 4.5f, 0xFF6B7A, 255);
        gfx_circle(x, y, 2.7f, 0x3A2E52, 255);
        break;
    case BTN_ICON_TRIANGLE:
        gfx_tri(x, y - 5, x - 5, y + 3.5f, x + 5, y + 3.5f, 0x5FE0B0, 255);
        gfx_tri(x, y - 2, x - 2.4f, y + 2, x + 2.4f, y + 2, 0x3A2E52, 255);
        break;
    case BTN_ICON_SQUARE:
        gfx_fill(x - 4, y - 4, 8, 8, 0xFF9AD8, 255);
        gfx_fill(x - 2, y - 2, 4, 4, 0x3A2E52, 255);
        break;
    }
}
