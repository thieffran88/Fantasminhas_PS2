/*
 * Versão de PC (sem janela) usada para testar o jogo e gerar capturas.
 * Uso: prefeitos_host [--seed N] comando...
 *   U D L R X O T Q ST SE L1 R1  -> aperta um botão (Q = quadrado, T = triângulo)
 *   wN                           -> espera N quadros
 *   @arquivo.ppm                 -> salva a tela atual
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "ui.h"

static u32 screen_buf[SCR_W * SCR_H];

static void save_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) {
        perror(path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", SCR_W, SCR_H);
    for (i = 0; i < SCR_W * SCR_H; i++) {
        u32 p = screen_buf[i];
        unsigned char rgb[3] = { (unsigned char)(p & 0xFF), (unsigned char)((p >> 8) & 0xFF),
                                 (unsigned char)((p >> 16) & 0xFF) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

static void step(unsigned int pressed)
{
    ui_update(pressed);
    ui_draw_to(screen_buf);
}

static unsigned int parse_btn(const char *s)
{
    static const struct { const char *n; unsigned int b; } map[] = {
        { "U", B_UP }, { "D", B_DOWN }, { "L", B_LEFT }, { "R", B_RIGHT },
        { "X", B_CROSS }, { "O", B_CIRCLE }, { "T", B_TRIANGLE }, { "Q", B_SQUARE },
        { "ST", B_START }, { "SE", B_SELECT }, { "L1", B_L1 }, { "R1", B_R1 },
    };
    unsigned i;
    for (i = 0; i < sizeof map / sizeof map[0]; i++)
        if (!strcmp(s, map[i].n))
            return map[i].b;
    return 0;
}

int main(int argc, char **argv)
{
    int i = 1;
    unsigned int seed = 42;
    if (argc > 2 && !strcmp(argv[1], "--seed")) {
        seed = (unsigned)strtoul(argv[2], 0, 10);
        i = 3;
    }
    ui_init(seed);
    step(0);
    for (; i < argc; i++) {
        const char *a = argv[i];
        if (a[0] == '@') {
            save_ppm(a + 1);
        } else if (a[0] == 'w') {
            int n = atoi(a + 1);
            while (n-- > 0)
                step(0);
        } else {
            unsigned int b = parse_btn(a);
            if (!b) {
                fprintf(stderr, "comando desconhecido: %s\n", a);
                return 1;
            }
            step(b);
            step(0);
        }
    }
    {
        const Game *g = ui_game();
        printf("tela=%d jogadas=%d cidades=%d resultado=%d monte=%d\n", ui_screen(), g->played, g->ntowns,
               g->result, g->deck_count);
    }
    return 0;
}
