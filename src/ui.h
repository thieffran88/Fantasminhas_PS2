#ifndef UI_H
#define UI_H

#include "game.h"

/* Botões (independentes de plataforma) */
#define B_UP (1 << 0)
#define B_DOWN (1 << 1)
#define B_LEFT (1 << 2)
#define B_RIGHT (1 << 3)
#define B_CROSS (1 << 4)
#define B_CIRCLE (1 << 5)
#define B_TRIANGLE (1 << 6)
#define B_SQUARE (1 << 7)
#define B_START (1 << 8)
#define B_SELECT (1 << 9)
#define B_L1 (1 << 10)
#define B_R1 (1 << 11)

void ui_init(unsigned int seed);
/* pressed = botões que acabaram de ser apertados neste quadro */
void ui_update(unsigned int pressed);

/* desenha o quadro atual no buffer (SCR_W x SCR_H, formato GS CT32) */
void ui_draw_to(unsigned int *out);

/* para testes automáticos */
const Game *ui_game(void);
int ui_screen(void);

#endif
