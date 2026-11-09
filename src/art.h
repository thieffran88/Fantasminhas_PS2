#ifndef ART_H
#define ART_H

/* Arte procedural (tudo desenhado com formas, sem imagens externas). */

enum { BTN_ICON_CROSS, BTN_ICON_CIRCLE, BTN_ICON_TRIANGLE, BTN_ICON_SQUARE };

#define COL_BUBBLE 0xF4F0FA
#define COL_INK 0x2B2240
#define COL_GOLD 0xFFD45A
#define COL_OK 0x3CCB6A
#define COL_BAD 0xF0475A
#define COL_PANEL 0x241838
#define COL_PANEL_EDGE 0x5B4785
#define COL_CARD 0xFBF8FF
#define COL_CARD_EDGE 0xCFC3E6
#define COL_TEXT 0xF4F0FA
#define COL_DIM 0xB7A8D6

void art_init(void);
void art_background(void);  /* desenha o cenário noturno na tela atual */
void art_landmark(int type, float cx, float cy, float s, int alpha);
void art_ghost(float cx, float cy, float s, int hat, int alpha);
void art_star(float cx, float cy, float r, int rgb, int alpha);
void art_check(float cx, float cy, float s, int rgb);
void art_cross(float cx, float cy, float s, int rgb);
void art_status_badge(float cx, float cy, int status, int t);
void art_card_face(int x, int y, int w, int h, int card, int edge);
void art_card_back(int x, int y, int w, int h, int star);
void art_button(int type, int x, int y);

#endif
