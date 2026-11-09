#ifndef GFX_H
#define GFX_H

#include "font.h"

/* Renderizador por software. O quadro é desenhado na RAM e enviado
 * inteiro como textura para o GS do PS2 (ou salvo em PNG no PC). */

#define SCR_W 640
#define SCR_H 448

typedef unsigned int u32;

/* formato dos pixels = GS_PSM_CT32 do PS2: 0xAABBGGRR (little endian) */
#define PIX(rgb) ((((rgb) >> 16) & 0xFF) | ((rgb) & 0xFF00) | (((rgb) & 0xFF) << 16) | 0x80000000u)

enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

void gfx_target(u32 *pixels);
u32 *gfx_pixels(void);
void gfx_copy(const u32 *src);  /* copia uma tela inteira */

void gfx_fill(int x, int y, int w, int h, int rgb, int alpha);
void gfx_vgrad(int x, int y, int w, int h, int rgb_top, int rgb_bottom);
void gfx_circle(float cx, float cy, float r, int rgb, int alpha);
void gfx_ellipse(float cx, float cy, float rx, float ry, int rgb, int alpha);
void gfx_tri(float x0, float y0, float x1, float y1, float x2, float y2, int rgb, int alpha);
void gfx_quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, int rgb, int alpha);
void gfx_rrect(int x, int y, int w, int h, int r, int rgb, int alpha);
void gfx_rrect_border(int x, int y, int w, int h, int r, int t, int border, int fill, int alpha);
void gfx_line(float x0, float y0, float x1, float y1, float thick, int rgb, int alpha);
void gfx_dashed_rect(int x, int y, int w, int h, int dash, int t, int rgb, int alpha);

/* Texto (UTF-8). Bytes 0x01..0x04 trocam a cor para gfx_lm_colors[n-1];
 * 0x05 volta para a cor original. '\n' quebra a linha. */
extern int gfx_lm_colors[4];
int gfx_text(const Font *f, int x, int y, const char *s, int rgb, int alpha);
int gfx_text_width(const Font *f, const char *s);
void gfx_text_al(const Font *f, int x, int y, int align, const char *s, int rgb, int alpha);
/* Desenha com quebra automática de linhas. Retorna a altura total. */
int gfx_text_wrap(const Font *f, int x, int y, int w, int align, int line_h,
                  const char *s, int rgb, int alpha, int draw);
void gfx_text_shadow(const Font *f, int x, int y, int align, const char *s, int rgb, int shadow);

#endif
