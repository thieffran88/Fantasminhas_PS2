/* Testes do motor de regras: rode com `make -f Makefile.host test`. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game.h"

static int fails;
#define CHECK(c, ...)                                     \
    do {                                                  \
        if (!(c)) {                                       \
            fails++;                                      \
            if (fails < 20) {                             \
                printf("FALHA %s:%d: ", __FILE__, __LINE__); \
                printf(__VA_ARGS__);                      \
                printf("\n");                             \
            }                                             \
        }                                                 \
    } while (0)

static int has(const Card *c, int t) { return c->lm[0] == t || c->lm[1] == t; }

static void test_deck(void)
{
    int combo[NUM_LM][NUM_LM] = { { 0 } }, i;
    char buf[200];
    for (i = 0; i < DECK_SIZE; i++) {
        const Card *c = &card_db[i];
        int a = c->lm[0] < c->lm[1] ? c->lm[0] : c->lm[1];
        int b = c->lm[0] < c->lm[1] ? c->lm[1] : c->lm[0];
        combo[a][b]++;
        card_req_text(c, buf, sizeof buf);
        CHECK(strlen(buf) > 8, "carta %d sem texto", i);
        if (c->req == R_NONE)
            CHECK(!has(c, c->a), "carta %d: 'nenhum' com o próprio marco", i);
        if (c->req == R_NONE_NOOTHER)
            CHECK(!has(c, c->a) && has(c, c->b), "carta %d: 'nenhum outro' inconsistente", i);
    }
    for (i = 0; i < NUM_LM; i++) {
        int j;
        for (j = i; j < NUM_LM; j++)
            CHECK(combo[i][j] == 5, "combinação %d/%d tem %d cartas", i, j, combo[i][j]);
    }
}

static int count_cards(const Game *g)
{
    int n = g->deck_count + (g->tourist >= 0) + (g->hand >= 0), i;
    for (i = 0; i < g->ntowns; i++)
        n += g->towns[i].ncards + (g->towns[i].tucked >= 0);
    return n;
}

static int legal_towns(const Game *g, int *out)
{
    int n = 0, i;
    for (i = 0; i <= g->ntowns && i < MAX_TOWNS; i++)
        if (game_can_play(g, i))
            out[n++] = i;
    return n;
}

/* Joga até o fim ignorando derrotas, e confirma que todo pedido marcado como
 * "impossível" realmente não é cumprido no final (detecção correta). */
static void test_broken_soundness(int games)
{
    int gi;
    for (gi = 0; gi < games; gi++) {
        Game g;
        int flagged[MAX_TOWNS] = { 0 }, i;
        game_init(&g, 3, (unsigned)gi * 7919u + 1);
        while (g.played < CARDS_TO_PLAY) {
            int opts[MAX_TOWNS + 1], n;
            if ((game_rand(&g) & 1) && g.tourist >= 0)
                game_draw_tourist(&g);
            else
                game_draw_deck(&g);
            n = legal_towns(&g, opts);
            CHECK(n > 0, "sem jogada legal");
            if (!n)
                break;
            game_play(&g, opts[game_rand(&g) % (unsigned)n]);
            CHECK(count_cards(&g) == DECK_SIZE, "cartas perdidas: %d", count_cards(&g));
            for (i = 0; i < g.ntowns; i++)
                if (g.played < CARDS_TO_PLAY && game_req_status(&g, i) == ST_BROKEN)
                    flagged[i] = 1;
            g.result = GS_PLAYING; /* continua mesmo após derrota */
        }
        CHECK(g.ntowns == MAX_TOWNS, "terminou com %d cidades", g.ntowns);
        for (i = 0; i < g.ntowns; i++) {
            CHECK(g.towns[i].ncards == TOWN_CARDS, "cidade incompleta");
            CHECK(g.towns[i].tucked >= 0, "prefeito sem carta escondida");
            if (flagged[i])
                CHECK(!game_req_final(&g, i), "jogo %d: cidade %d marcada impossível mas cumprida", gi, i);
        }
    }
}

/* Jogador automático simples: evita jogadas que perdem e prefere as que
 * deixam mais pedidos cumpridos. Serve para ver se o jogo é vencível. */
static int score_state(const Game *g)
{
    int i, s = 0;
    if (g->result == GS_LOST)
        return -1000;
    if (g->result == GS_WON)
        return 1000;
    for (i = 0; i < g->ntowns; i++)
        s += game_req_status(g, i) == ST_MET ? 3 : 0;
    return s;
}

static int greedy_game(unsigned seed)
{
    Game g;
    game_init(&g, 1, seed);
    while (g.result == GS_PLAYING) {
        int best = -100000, bt = -1, bsrc = 0, src;
        for (src = 0; src < 2; src++) {
            Game d = g;
            int ok = src ? game_draw_tourist(&d) : game_draw_deck(&d);
            int t;
            if (!ok)
                continue;
            for (t = 0; t <= d.ntowns && t < MAX_TOWNS; t++) {
                Game e = d;
                int sc;
                if (!game_play(&e, t))
                    continue;
                sc = score_state(&e) + (int)(game_rand(&e) % 3);
                if (sc > best) {
                    best = sc;
                    bt = t;
                    bsrc = src;
                }
            }
        }
        if (bt < 0)
            break;
        if (bsrc)
            game_draw_tourist(&g);
        else
            game_draw_deck(&g);
        game_play(&g, bt);
    }
    return g.result == GS_WON;
}

static void test_basic_turn(void)
{
    Game g;
    game_init(&g, 2, 5);
    CHECK(g.deck_count == DECK_SIZE - 1 && g.tourist >= 0, "preparação");
    CHECK(!game_play(&g, 0), "não pode jogar sem comprar");
    CHECK(game_draw_tourist(&g), "compra turista");
    CHECK(!game_draw_deck(&g), "não compra duas vezes");
    CHECK(!game_can_play(&g, 1), "não pula cidades");
    CHECK(game_play(&g, 0), "funda cidade");
    CHECK(g.ntowns == 1 && g.towns[0].tucked >= 0, "prefeito com carta escondida");
    CHECK(g.tourist >= 0 && g.deck_count == DECK_SIZE - 3, "turista reposto (%d)", g.deck_count);
    CHECK(g.player == 1, "passa a vez");
}

int main(void)
{
    int wins = 0, n = 3000, i;
    test_deck();
    test_basic_turn();
    test_broken_soundness(20000);
    for (i = 0; i < n; i++)
        wins += greedy_game((unsigned)i + 99);
    printf("Jogador automático guloso: %d vitórias em %d partidas (%.1f%%)\n", wins, n, 100.0 * wins / n);
    CHECK(wins > 0, "nenhuma vitória: jogo impossível?");
    if (fails) {
        printf("%d falha(s)\n", fails);
        return 1;
    }
    printf("Todos os testes passaram.\n");
    return 0;
}
