#include "game.h"

#include <stdio.h>
#include <string.h>

#define H CASA
#define T ARVORE
#define L LAGO
#define S LOJA
#define C(x, y, r, pa, pb, pn) { { x, y }, r, pa, pb, pn }

/* Baralho: 10 combinações de marcos x 5 cartas = 50 cartas. */
const Card card_db[DECK_SIZE] = {
    /* casa + casa */
    C(H, H, R_MOST, H, 0, 0),
    C(H, H, R_TOWNS_WITH, T, 0, 4),
    C(H, H, R_EXACT_TYPES, 0, 0, 2),
    C(H, H, R_NONE_NOOTHER, S, H, 0),
    C(H, H, R_SAME, T, L, 0),
    /* árvore + árvore */
    C(T, T, R_MOST, T, 0, 0),
    C(T, T, R_TOWNS_WITH, L, 0, 4),
    C(T, T, R_EXACT_TYPES, 0, 0, 2),
    C(T, T, R_NONE_NOOTHER, H, T, 0),
    C(T, T, R_SAME, H, S, 0),
    /* lago + lago */
    C(L, L, R_MOST, L, 0, 0),
    C(L, L, R_TOWNS_WITH, S, 0, 4),
    C(L, L, R_EXACT_TYPES, 0, 0, 2),
    C(L, L, R_NONE_NOOTHER, T, L, 0),
    C(L, L, R_ATLEAST2, H, S, 0),
    /* loja + loja */
    C(S, S, R_MOST, S, 0, 0),
    C(S, S, R_TOWNS_WITH, H, 0, 4),
    C(S, S, R_EXACT_TYPES, 0, 0, 2),
    C(S, S, R_NONE_NOOTHER, L, S, 0),
    C(S, S, R_ATLEAST2, T, L, 0),
    /* casa + árvore */
    C(H, T, R_NONE, S, 0, 0),
    C(H, T, R_TOWNS_BOTH, H, T, 3),
    C(H, T, R_TOWNS_WITH, L, 0, 5),
    C(H, T, R_EXACT_TYPES, 0, 0, 3),
    C(H, T, R_ALL_TOWNS, S, 0, 0),
    /* casa + lago */
    C(H, L, R_NONE, T, 0, 0),
    C(H, L, R_TOWNS_BOTH, H, L, 3),
    C(H, L, R_TOWNS_WITH, S, 0, 5),
    C(H, L, R_MORE_THAN, H, L, 0),
    C(H, L, R_ALL_TOWNS, T, 0, 0),
    /* casa + loja */
    C(H, S, R_NONE, L, 0, 0),
    C(H, S, R_TOWNS_BOTH, H, S, 3),
    C(H, S, R_TOWNS_WITH, T, 0, 5),
    C(H, S, R_EXACT_COUNT, H, 0, 4),
    C(H, S, R_ALL_TOWNS, L, 0, 0),
    /* árvore + lago */
    C(T, L, R_NONE, H, 0, 0),
    C(T, L, R_TOWNS_BOTH, T, L, 3),
    C(T, L, R_TOWNS_WITH, H, 0, 5),
    C(T, L, R_EXACT_COUNT, T, 0, 4),
    C(T, L, R_MORE_THAN, L, T, 0),
    /* árvore + loja */
    C(T, S, R_EXACT_TYPES, 0, 0, 3),
    C(T, S, R_TOWNS_BOTH, T, S, 3),
    C(T, S, R_ATLEAST2, S, H, 0),
    C(T, S, R_EXACT_COUNT, S, 0, 4),
    C(T, S, R_ALL_TOWNS, H, 0, 0),
    /* lago + loja */
    C(L, S, R_EXACT_TYPES, 0, 0, 3),
    C(L, S, R_TOWNS_BOTH, L, S, 3),
    C(L, S, R_MORE_THAN, S, T, 0),
    C(L, S, R_EXACT_COUNT, L, 0, 4),
    C(L, S, R_SAME, L, S, 0),
};

const char *const lm_name_sg[NUM_LM] = { "casa", "árvore", "lago", "loja" };
const char *const lm_name_pl[NUM_LM] = { "casas", "árvores", "lagos", "lojas" };
static const int lm_masc[NUM_LM] = { 0, 0, 1, 0 };

/* ------------------------------------------------------------------ */

unsigned int game_rand(Game *g)
{
    unsigned int x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

void game_init(Game *g, int nplayers, unsigned int seed)
{
    int i;
    memset(g, 0, sizeof(*g));
    g->rng = seed ? seed : 0x1234567u;
    for (i = 0; i < 8; i++)
        game_rand(g);
    for (i = 0; i < DECK_SIZE; i++)
        g->deck[i] = i;
    for (i = DECK_SIZE - 1; i > 0; i--) {
        int j = (int)(game_rand(g) % (unsigned)(i + 1));
        int t = g->deck[i];
        g->deck[i] = g->deck[j];
        g->deck[j] = t;
    }
    g->deck_count = DECK_SIZE;
    g->tourist = g->deck[--g->deck_count];
    for (i = 0; i < MAX_TOWNS; i++)
        g->towns[i].tucked = -1;
    g->hand = -1;
    g->nplayers = nplayers < 1 ? 1 : (nplayers > 6 ? 6 : nplayers);
    g->player = 0;
    g->result = GS_PLAYING;
    g->fail_town = -1;
}

int game_draw_deck(Game *g)
{
    if (g->result != GS_PLAYING || g->hand >= 0 || g->deck_count <= 0)
        return 0;
    g->hand = g->deck[--g->deck_count];
    g->hand_from_tourist = 0;
    return 1;
}

int game_draw_tourist(Game *g)
{
    if (g->result != GS_PLAYING || g->hand >= 0 || g->tourist < 0)
        return 0;
    g->hand = g->tourist;
    g->tourist = -1;
    g->hand_from_tourist = 1;
    return 1;
}

int game_can_play(const Game *g, int town)
{
    if (g->result != GS_PLAYING || g->hand < 0)
        return 0;
    if (town >= 0 && town < g->ntowns)
        return g->towns[town].ncards < TOWN_CARDS;
    return town == g->ntowns && g->ntowns < MAX_TOWNS;
}

int game_play(Game *g, int town)
{
    Town *t;
    if (!game_can_play(g, town))
        return 0;
    if (town == g->ntowns) {
        g->ntowns++;
        t = &g->towns[town];
        t->ncards = 0;
        t->tucked = -1;
        t->cards[t->ncards++] = g->hand;
        /* novo prefeito: a carta do topo vai virada para baixo embaixo dele */
        if (g->deck_count > 0)
            t->tucked = g->deck[--g->deck_count];
    } else {
        t = &g->towns[town];
        t->cards[t->ncards++] = g->hand;
    }
    g->hand = -1;
    g->played++;
    if (g->hand_from_tourist && g->deck_count > 0)
        g->tourist = g->deck[--g->deck_count];
    g->hand_from_tourist = 0;
    game_check(g);
    if (g->result == GS_PLAYING)
        g->player = (g->player + 1) % g->nplayers;
    return 1;
}

/* ------------------------------------------------------------------ */

void town_counts(const Game *g, int town, int counts[NUM_LM])
{
    int i;
    const Town *t = &g->towns[town];
    for (i = 0; i < NUM_LM; i++)
        counts[i] = 0;
    for (i = 0; i < t->ncards; i++) {
        const Card *c = &card_db[t->cards[i]];
        counts[c->lm[0]]++;
        counts[c->lm[1]]++;
    }
}

int town_types(const int counts[NUM_LM])
{
    int i, n = 0;
    for (i = 0; i < NUM_LM; i++)
        n += counts[i] > 0;
    return n;
}

static int towns_with(const Game *g, int a, int b, int *possible)
{
    int i, have = 0, pot = 0;
    for (i = 0; i < g->ntowns; i++) {
        int c[NUM_LM], ok;
        town_counts(g, i, c);
        ok = c[a] > 0 && (b < 0 || c[b] > 0);
        if (ok)
            have++;
        else if (g->towns[i].ncards < TOWN_CARDS)
            pot++;
    }
    if (possible)
        *possible = have + pot + (MAX_TOWNS - g->ntowns);
    return have;
}

static int own_count(const Card *c, int type)
{
    return (c->lm[0] == type) + (c->lm[1] == type);
}

int game_req_final(const Game *g, int town)
{
    const Town *t = &g->towns[town];
    const Card *m = &card_db[t->cards[0]];
    int c[NUM_LM], i;
    town_counts(g, town, c);
    switch (m->req) {
    case R_ATLEAST2: return c[m->a] >= 2 && c[m->b] >= 2;
    case R_NONE: return c[m->a] == 0;
    case R_NONE_NOOTHER: return c[m->a] == 0 && c[m->b] <= own_count(m, m->b);
    case R_EXACT_TYPES: return town_types(c) == m->n;
    case R_EXACT_COUNT: return c[m->a] == m->n;
    case R_MOST:
        for (i = 0; i < g->ntowns; i++) {
            int o[NUM_LM];
            if (i == town)
                continue;
            town_counts(g, i, o);
            if (o[m->a] >= c[m->a])
                return 0;
        }
        return 1;
    case R_TOWNS_WITH: return towns_with(g, m->a, -1, 0) == m->n;
    case R_TOWNS_BOTH: return towns_with(g, m->a, m->b, 0) == m->n;
    case R_MORE_THAN: return c[m->a] > c[m->b];
    case R_SAME: return c[m->a] == c[m->b];
    case R_ALL_TOWNS:
        if (g->ntowns < MAX_TOWNS)
            return 0;
        return towns_with(g, m->a, -1, 0) == g->ntowns;
    }
    return 0;
}

/* Verdadeiro somente se for IMPOSSÍVEL cumprir o pedido daqui para frente
 * (cidades só ganham cartas, e cada carta traz no máximo 2 marcos). */
static int req_broken(const Game *g, int town)
{
    const Town *t = &g->towns[town];
    const Card *m = &card_db[t->cards[0]];
    int c[NUM_LM], i, pot, have;
    int room = 2 * (TOWN_CARDS - t->ncards); /* marcos que ainda podem chegar */
    town_counts(g, town, c);
    switch (m->req) {
    case R_ATLEAST2: {
        int da = c[m->a] < 2 ? 2 - c[m->a] : 0;
        int db = c[m->b] < 2 ? 2 - c[m->b] : 0;
        return da + db > room;
    }
    case R_NONE: return c[m->a] > 0;
    case R_NONE_NOOTHER: return c[m->a] > 0 || c[m->b] > own_count(m, m->b);
    case R_EXACT_TYPES: {
        int ty = town_types(c);
        return ty > m->n || ty + room < m->n;
    }
    case R_EXACT_COUNT: return c[m->a] > m->n || c[m->a] + room < m->n;
    case R_MOST:
        for (i = 0; i < g->ntowns; i++) {
            int o[NUM_LM];
            if (i == town || g->towns[i].ncards < TOWN_CARDS)
                continue;
            town_counts(g, i, o);
            if (o[m->a] >= c[m->a] + room)
                return 1;
        }
        return 0;
    case R_TOWNS_WITH:
        have = towns_with(g, m->a, -1, &pot);
        return have > m->n || pot < m->n;
    case R_TOWNS_BOTH:
        have = towns_with(g, m->a, m->b, &pot);
        return have > m->n || pot < m->n;
    case R_MORE_THAN: return c[m->b] >= c[m->a] + room;
    case R_SAME: {
        int d = c[m->a] - c[m->b];
        if (d < 0)
            d = -d;
        return d > room;
    }
    case R_ALL_TOWNS:
        for (i = 0; i < g->ntowns; i++) {
            int o[NUM_LM];
            if (g->towns[i].ncards < TOWN_CARDS)
                continue;
            town_counts(g, i, o);
            if (o[m->a] == 0)
                return 1;
        }
        return 0;
    }
    return 0;
}

ReqStatus game_req_status(const Game *g, int town)
{
    if (town < 0 || town >= g->ntowns)
        return ST_OPEN;
    if (g->played >= CARDS_TO_PLAY)
        return game_req_final(g, town) ? ST_MET : ST_BROKEN;
    if (req_broken(g, town))
        return ST_BROKEN;
    return game_req_final(g, town) ? ST_MET : ST_OPEN;
}

void game_check(Game *g)
{
    int i;
    if (g->result != GS_PLAYING)
        return;
    for (i = 0; i < g->ntowns; i++) {
        if (game_req_status(g, i) == ST_BROKEN) {
            g->result = GS_LOST;
            g->fail_town = i;
            return;
        }
    }
    if (g->played >= CARDS_TO_PLAY)
        g->result = GS_WON;
}

/* ------------------------------------------------------------------ */

typedef struct {
    char *p;
    int left;
} Sb;

static void sb_put(Sb *s, const char *txt)
{
    while (*txt && s->left > 1) {
        *s->p++ = *txt++;
        s->left--;
    }
    *s->p = 0;
}

static void sb_lm(Sb *s, int type, int plural)
{
    char tag[2] = { (char)(TXT_LM0 + type), 0 };
    char end[2] = { TXT_RESET, 0 };
    sb_put(s, tag);
    sb_put(s, plural ? lm_name_pl[type] : lm_name_sg[type]);
    sb_put(s, end);
}

static void sb_num(Sb *s, int n)
{
    char buf[12];
    snprintf(buf, sizeof buf, "%d", n);
    sb_put(s, buf);
}

void card_req_text(const Card *c, char *out, int outsz)
{
    Sb s;
    s.p = out;
    s.left = outsz;
    out[0] = 0;
    switch (c->req) {
    case R_ATLEAST2:
        sb_put(&s, "Pelo menos 2 ");
        sb_lm(&s, c->a, 1);
        sb_put(&s, " e 2 ");
        sb_lm(&s, c->b, 1);
        sb_put(&s, " aqui.");
        break;
    case R_NONE:
        sb_put(&s, lm_masc[c->a] ? "Nenhum " : "Nenhuma ");
        sb_lm(&s, c->a, 0);
        sb_put(&s, " aqui.");
        break;
    case R_NONE_NOOTHER:
        sb_put(&s, lm_masc[c->a] ? "Nenhum " : "Nenhuma ");
        sb_lm(&s, c->a, 0);
        sb_put(&s, lm_masc[c->b] ? " e nenhum outro " : " e nenhuma outra ");
        sb_lm(&s, c->b, 0);
        sb_put(&s, " aqui.");
        break;
    case R_EXACT_TYPES:
        sb_put(&s, "Exatamente ");
        sb_num(&s, c->n);
        sb_put(&s, " tipos de marcos aqui.");
        break;
    case R_EXACT_COUNT:
        sb_put(&s, "Exatamente ");
        sb_num(&s, c->n);
        sb_put(&s, " ");
        sb_lm(&s, c->a, 1);
        sb_put(&s, " aqui.");
        break;
    case R_MOST:
        sb_put(&s, "Mais ");
        sb_lm(&s, c->a, 1);
        sb_put(&s, " que qualquer outra cidade.");
        break;
    case R_TOWNS_WITH:
        sb_put(&s, "Exatamente ");
        sb_num(&s, c->n);
        sb_put(&s, " cidades com ");
        sb_lm(&s, c->a, 1);
        sb_put(&s, ".");
        break;
    case R_TOWNS_BOTH:
        sb_put(&s, "Exatamente ");
        sb_num(&s, c->n);
        sb_put(&s, " cidades com ");
        sb_lm(&s, c->a, 1);
        sb_put(&s, " e ");
        sb_lm(&s, c->b, 1);
        sb_put(&s, ".");
        break;
    case R_MORE_THAN:
        sb_put(&s, "Mais ");
        sb_lm(&s, c->a, 1);
        sb_put(&s, " do que ");
        sb_lm(&s, c->b, 1);
        sb_put(&s, " aqui.");
        break;
    case R_SAME:
        sb_put(&s, "Mesmo número de ");
        sb_lm(&s, c->a, 1);
        sb_put(&s, " e ");
        sb_lm(&s, c->b, 1);
        sb_put(&s, " aqui.");
        break;
    case R_ALL_TOWNS:
        sb_put(&s, "Todas as cidades têm ");
        sb_lm(&s, c->a, 1);
        sb_put(&s, ".");
        break;
    }
}
