#include "gfx.h"

#include <math.h>
#include <string.h>

static u32 *fb;

int gfx_lm_colors[4];

void gfx_target(u32 *pixels) { fb = pixels; }
u32 *gfx_pixels(void) { return fb; }
void gfx_copy(const u32 *src) { memcpy(fb, src, SCR_W * SCR_H * 4); }

/* mistura sem divisões (o EE do PS2 divide devagar) */
static inline void blend_px(int x, int y, u32 src, int a)
{
    u32 s, *d;
    int r, g, b;
    if ((unsigned)x >= SCR_W || (unsigned)y >= SCR_H || a <= 0)
        return;
    d = &fb[y * SCR_W + x];
    if (a >= 255) {
        *d = src;
        return;
    }
    a += a >> 7; /* 0..255 -> 0..256 */
    s = *d;
    r = (int)(s & 0xFF);
    g = (int)((s >> 8) & 0xFF);
    b = (int)((s >> 16) & 0xFF);
    r += (((int)(src & 0xFF) - r) * a) >> 8;
    g += (((int)((src >> 8) & 0xFF) - g) * a) >> 8;
    b += (((int)((src >> 16) & 0xFF) - b) * a) >> 8;
    *d = (u32)r | ((u32)g << 8) | ((u32)b << 16) | 0x80000000u;
}

void gfx_fill(int x, int y, int w, int h, int rgb, int alpha)
{
    int x1 = x + w, y1 = y + h, i, j;
    u32 c = PIX(rgb);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > SCR_W) x1 = SCR_W;
    if (y1 > SCR_H) y1 = SCR_H;
    if (x >= x1 || y >= y1 || alpha <= 0)
        return;
    for (j = y; j < y1; j++) {
        u32 *row = fb + j * SCR_W;
        if (alpha >= 255) {
            for (i = x; i < x1; i++)
                row[i] = c;
        } else {
            for (i = x; i < x1; i++)
                blend_px(i, j, c, alpha);
        }
    }
}

void gfx_vgrad(int x, int y, int w, int h, int top, int bot)
{
    int j;
    for (j = 0; j < h; j++) {
        int t = h > 1 ? j * 256 / (h - 1) : 0;
        int r = ((top >> 16) & 255) + ((((bot >> 16) & 255) - ((top >> 16) & 255)) * t >> 8);
        int g = ((top >> 8) & 255) + ((((bot >> 8) & 255) - ((top >> 8) & 255)) * t >> 8);
        int b = (top & 255) + (((bot & 255) - (top & 255)) * t >> 8);
        gfx_fill(x, y + j, w, 1, (r << 16) | (g << 8) | b, 255);
    }
}

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void gfx_circle(float cx, float cy, float r, int rgb, int alpha)
{
    gfx_ellipse(cx, cy, r, r, rgb, alpha);
}

void gfx_ellipse(float cx, float cy, float rx, float ry, int rgb, int alpha)
{
    int x0 = (int)floorf(cx - rx - 1), x1 = (int)ceilf(cx + rx + 1);
    int y0 = (int)floorf(cy - ry - 1), y1 = (int)ceilf(cy + ry + 1);
    float mr = rx < ry ? rx : ry;
    float inner, outer, irx, iry;
    int x, y;
    u32 c = PIX(rgb);
    if (rx <= 0 || ry <= 0)
        return;
    /* limiares em espaço normalizado (|p| = 1 na borda) */
    inner = 1.0f - 0.75f / mr;
    outer = 1.0f + 0.75f / mr;
    inner = inner > 0 ? inner * inner : 0;
    outer = outer * outer;
    y0 = clampi(y0, 0, SCR_H);
    y1 = clampi(y1, 0, SCR_H);
    x0 = clampi(x0, 0, SCR_W);
    x1 = clampi(x1, 0, SCR_W);
    irx = 1.0f / rx;
    iry = 1.0f / ry;
    for (y = y0; y < y1; y++) {
        float dy = ((float)y + 0.5f - cy) * iry;
        float dy2 = dy * dy;
        if (dy2 > outer)
            continue;
        for (x = x0; x < x1; x++) {
            float dx = ((float)x + 0.5f - cx) * irx;
            float d2 = dx * dx + dy2;
            if (d2 >= outer)
                continue;
            if (d2 <= inner) {
                blend_px(x, y, c, alpha);
            } else {
                float cov = (1.0f - sqrtf(d2)) * mr + 0.5f;
                if (cov > 0)
                    blend_px(x, y, c, cov >= 1.0f ? alpha : (int)(alpha * cov));
            }
        }
    }
}

static float edgef(float ax, float ay, float bx, float by, float px, float py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void gfx_tri(float x0, float y0, float x1, float y1, float x2, float y2, int rgb, int alpha)
{
    float area = edgef(x0, y0, x1, y1, x2, y2);
    int minx, maxx, miny, maxy, x, y;
    u32 c = PIX(rgb);
    if (area == 0)
        return;
    if (area < 0) { /* garante orientação positiva */
        float tx = x1, ty = y1;
        x1 = x2; y1 = y2; x2 = tx; y2 = ty;
    }
    minx = clampi((int)floorf(fminf(x0, fminf(x1, x2))), 0, SCR_W);
    maxx = clampi((int)ceilf(fmaxf(x0, fmaxf(x1, x2))), 0, SCR_W);
    miny = clampi((int)floorf(fminf(y0, fminf(y1, y2))), 0, SCR_H);
    maxy = clampi((int)ceilf(fmaxf(y0, fmaxf(y1, y2))), 0, SCR_H);
    for (y = miny; y < maxy; y++) {
        float py = (float)y + 0.5f;
        for (x = minx; x < maxx; x++) {
            float px = (float)x + 0.5f;
            if (edgef(x0, y0, x1, y1, px, py) >= 0 && edgef(x1, y1, x2, y2, px, py) >= 0 &&
                edgef(x2, y2, x0, y0, px, py) >= 0)
                blend_px(x, y, c, alpha);
        }
    }
}

void gfx_quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, int rgb, int alpha)
{
    gfx_tri(x0, y0, x1, y1, x2, y2, rgb, alpha);
    /* evita pintar duas vezes a diagonal quando há transparência */
    if (alpha >= 255) {
        gfx_tri(x0, y0, x2, y2, x3, y3, rgb, alpha);
    } else {
        float area = edgef(x0, y0, x2, y2, x3, y3);
        int minx = clampi((int)floorf(fminf(x0, fminf(x2, x3))), 0, SCR_W);
        int maxx = clampi((int)ceilf(fmaxf(x0, fmaxf(x2, x3))), 0, SCR_W);
        int miny = clampi((int)floorf(fminf(y0, fminf(y2, y3))), 0, SCR_H);
        int maxy = clampi((int)ceilf(fmaxf(y0, fmaxf(y2, y3))), 0, SCR_H);
        int x, y;
        float s = area < 0 ? -1.0f : 1.0f;
        u32 c = PIX(rgb);
        for (y = miny; y < maxy; y++) {
            float py = (float)y + 0.5f;
            for (x = minx; x < maxx; x++) {
                float px = (float)x + 0.5f;
                if (s * edgef(x0, y0, x2, y2, px, py) > 0 && s * edgef(x2, y2, x3, y3, px, py) >= 0 &&
                    s * edgef(x3, y3, x0, y0, px, py) >= 0)
                    blend_px(x, y, c, alpha);
            }
        }
    }
}

void gfx_rrect(int x, int y, int w, int h, int r, int rgb, int alpha)
{
    int j, i;
    u32 c = PIX(rgb);
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    if (r <= 0) {
        gfx_fill(x, y, w, h, rgb, alpha);
        return;
    }
    gfx_fill(x, y + r, w, h - 2 * r, rgb, alpha);
    for (j = 0; j < r; j++) {
        float dy = (float)r - ((float)j + 0.5f);
        for (i = 0; i < r; i++) {
            float dx = (float)r - ((float)i + 0.5f);
            float cov = (float)r - sqrtf(dx * dx + dy * dy) + 0.5f;
            int a;
            if (cov <= 0)
                continue;
            a = cov >= 1 ? alpha : (int)(alpha * cov);
            blend_px(x + i, y + j, c, a);
            blend_px(x + w - 1 - i, y + j, c, a);
            blend_px(x + i, y + h - 1 - j, c, a);
            blend_px(x + w - 1 - i, y + h - 1 - j, c, a);
        }
        gfx_fill(x + r, y + j, w - 2 * r, 1, rgb, alpha);
        gfx_fill(x + r, y + h - 1 - j, w - 2 * r, 1, rgb, alpha);
    }
}

void gfx_rrect_border(int x, int y, int w, int h, int r, int t, int border, int fill, int alpha)
{
    gfx_rrect(x, y, w, h, r, border, alpha);
    gfx_rrect(x + t, y + t, w - 2 * t, h - 2 * t, r > t ? r - t : 0, fill, alpha);
}

void gfx_line(float x0, float y0, float x1, float y1, float thick, int rgb, int alpha)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    float nx, ny, h = thick * 0.5f;
    if (len < 0.001f) {
        gfx_circle(x0, y0, h, rgb, alpha);
        return;
    }
    nx = -dy / len * h;
    ny = dx / len * h;
    gfx_quad(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, rgb, alpha);
    gfx_circle(x0, y0, h, rgb, alpha);
    gfx_circle(x1, y1, h, rgb, alpha);
}

void gfx_dashed_rect(int x, int y, int w, int h, int dash, int t, int rgb, int alpha)
{
    int i;
    for (i = 0; i < w; i += dash * 2) {
        int l = (i + dash > w) ? w - i : dash;
        gfx_fill(x + i, y, l, t, rgb, alpha);
        gfx_fill(x + i, y + h - t, l, t, rgb, alpha);
    }
    for (i = 0; i < h; i += dash * 2) {
        int l = (i + dash > h) ? h - i : dash;
        gfx_fill(x, y + i, t, l, rgb, alpha);
        gfx_fill(x + w - t, y + i, t, l, rgb, alpha);
    }
}

/* ------------------------------------------------------------------ */
/* Texto */

static int utf8_next(const char **ps)
{
    const unsigned char *s = (const unsigned char *)*ps;
    int c = *s;
    if (c < 0x80) {
        *ps += 1;
        return c;
    }
    if ((c & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        *ps += 2;
        c = ((c & 0x1F) << 6) | (s[1] & 0x3F);
        return c <= 255 ? c : '?';
    }
    /* sequências maiores: pula e mostra '?' */
    *ps += 1;
    while ((**ps & 0xC0) == 0x80)
        *ps += 1;
    return '?';
}

static void draw_glyph(const Font *f, int code, int x, int y, u32 c, int alpha)
{
    const Glyph *g;
    const unsigned char *p;
    int i, j, stride;
    if (code < FONT_FIRST || code > FONT_LAST)
        return;
    g = &f->glyphs[code - FONT_FIRST];
    if (!g->w)
        return;
    p = f->bits + g->off;
    stride = (g->w + 1) / 2;
    x += g->bx;
    y += g->by;
    for (j = 0; j < g->h; j++) {
        for (i = 0; i < g->w; i++) {
            int v = p[j * stride + (i >> 1)];
            v = (i & 1) ? (v >> 4) : (v & 15);
            if (v)
                blend_px(x + i, y + j, c, v == 15 ? alpha : alpha * v / 15);
        }
    }
}

static int glyph_adv(const Font *f, int code)
{
    if (code < FONT_FIRST || code > FONT_LAST)
        return 0;
    return f->glyphs[code - FONT_FIRST].adv;
}

int gfx_text(const Font *f, int x, int y, const char *s, int rgb, int alpha)
{
    int x0 = x;
    u32 base = PIX(rgb), c = base;
    while (*s) {
        int code = utf8_next(&s);
        if (code >= 1 && code <= 4) {
            c = PIX(gfx_lm_colors[code - 1]);
            continue;
        }
        if (code == 5) {
            c = base;
            continue;
        }
        if (code == '\n')
            break;
        draw_glyph(f, code, x, y, c, alpha);
        x += glyph_adv(f, code);
    }
    return x - x0;
}

int gfx_text_width(const Font *f, const char *s)
{
    int w = 0;
    while (*s) {
        int code = utf8_next(&s);
        if (code == '\n')
            break;
        w += glyph_adv(f, code);
    }
    return w;
}

void gfx_text_al(const Font *f, int x, int y, int align, const char *s, int rgb, int alpha)
{
    int w = gfx_text_width(f, s);
    if (align == ALIGN_CENTER)
        x -= w / 2;
    else if (align == ALIGN_RIGHT)
        x -= w;
    gfx_text(f, x, y, s, rgb, alpha);
}

void gfx_text_shadow(const Font *f, int x, int y, int align, const char *s, int rgb, int shadow)
{
    gfx_text_al(f, x + 2, y + 2, align, s, shadow, 160);
    gfx_text_al(f, x, y, align, s, rgb, 255);
}

int gfx_text_wrap(const Font *f, int x, int y, int w, int align, int line_h,
                  const char *s, int rgb, int alpha, int draw)
{
    char line[256];
    int lines = 0;
    int cur_color = 0; /* cor ativa no começo da linha (0 = padrão) */
    while (*s) {
        /* acumula palavras enquanto couber */
        const char *p = s, *best = 0;
        int wpx = 0;
        while (1) {
            const char *q = p;
            int ww = 0;
            /* espaços */
            while (*q == ' ') {
                ww += glyph_adv(f, ' ');
                q++;
            }
            if (!*q || *q == '\n') {
                best = q;
                break;
            }
            while (*q && *q != ' ' && *q != '\n') {
                int code = utf8_next(&q);
                ww += glyph_adv(f, code);
            }
            if (wpx + ww > w && best) /* já há algo na linha */
                break;
            wpx += ww;
            best = q;
            p = q;
            if (!*q || *q == '\n')
                break;
        }
        {
            int n = (int)(best - s), k = 0;
            if (cur_color)
                line[k++] = (char)cur_color;
            if (n > (int)sizeof(line) - 2 - k)
                n = (int)sizeof(line) - 2 - k;
            memcpy(line + k, s, n);
            line[k + n] = 0;
            /* atualiza a cor ativa para a próxima linha */
            {
                int i;
                for (i = 0; i < n; i++) {
                    if (s[i] >= 1 && s[i] <= 4)
                        cur_color = s[i];
                    else if (s[i] == 5)
                        cur_color = 0;
                }
            }
            if (draw) {
                int lw = gfx_text_width(f, line);
                int lx = x;
                if (align == ALIGN_CENTER)
                    lx = x + (w - lw) / 2;
                else if (align == ALIGN_RIGHT)
                    lx = x + w - lw;
                gfx_text(f, lx, y + lines * line_h, line, rgb, alpha);
            }
            lines++;
            s = best;
            while (*s == ' ')
                s++;
            if (*s == '\n')
                s++;
        }
    }
    return lines * line_h;
}
