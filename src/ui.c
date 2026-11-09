#include "ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "art.h"
#include "gfx.h"
#include "sprites.h"

enum { SCR_TITLE, SCR_RULES, SCR_GAME, SCR_PAUSE, SCR_END };
enum { PH_DRAW, PH_PLACE };

#define TOWN_W 196
#define TOWN_H 142
#define CARD_W 34
#define CARD_H 46
#define HAND_X 172
#define HAND_Y 350
#define ANIM_LEN 14
#define NUM_RULE_PAGES 4

static Game G;
static int screen = SCR_TITLE, rules_return = SCR_TITLE;
static int frame;
static int title_sel, nplayers = 2, hints = 1;
static int rules_page;
static int phase, draw_sel, town_sel, pause_sel;
static unsigned int seed;
static int end_delay, hide_overlay;

static int anim_t = -1, anim_town, anim_slot;
static int happy_town = -1, happy_t; /* prefeito animado depois de receber carta */

/* cada jogador é um dos fantasminhas das folhas de sprites */
static const char *const player_names[6] = { "Zaca", "Nina", "Lili", "Beto", "Dona Nena", "Prefeito" };
static const int player_avatar[6] = { SPR_AVATAR_ZACA, SPR_AVATAR_NINA, SPR_AVATAR_LILI,
                                      SPR_AVATAR_BETO, SPR_AVATAR_NENA, SPR_AVATAR_PREFEITO };
static const int title_chars[6] = { SPR_CHAR_PREFEITO, SPR_CHAR_ZACA, SPR_CHAR_NINA,
                                    SPR_CHAR_LILI, SPR_CHAR_BETO, SPR_CHAR_NENA };
static char toast[128];
static int toast_t;

static u32 bg[SCR_W * SCR_H] __attribute__((aligned(64)));
static u32 cache[SCR_W * SCR_H] __attribute__((aligned(64)));
static int bg_ready, dirty = 1;

const Game *ui_game(void) { return &G; }
int ui_screen(void) { return screen; }
int ui_town_sel(void) { return town_sel; }

static void town_xy(int i, int *x, int *y)
{
    *x = 22 + (i % 3) * (TOWN_W + 4);
    *y = 48 + (i / 3) * (TOWN_H + 4);
}

static void slot_xy(int town, int k, int *x, int *y)
{
    int tx, ty;
    town_xy(town, &tx, &ty);
    *x = tx + 50 + k * (CARD_W + 2);
    *y = ty + 70;
}

static void set_toast(const char *s)
{
    snprintf(toast, sizeof toast, "%s", s);
    toast_t = 120;
}

static int first_valid_town(void)
{
    int i;
    for (i = 0; i <= G.ntowns && i < MAX_TOWNS; i++)
        if (game_can_play(&G, i))
            return i;
    return 0;
}

static void new_game(void)
{
    seed = seed * 1103515245u + 12345u + (unsigned)frame;
    game_init(&G, nplayers, seed);
    phase = PH_DRAW;
    draw_sel = 0;
    town_sel = 0;
    anim_t = -1;
    end_delay = 0;
    hide_overlay = 0;
    toast_t = 0;
    screen = SCR_GAME;
    happy_town = -1;
    {
        char b[64];
        if (G.nplayers > 1)
            snprintf(b, sizeof b, "Boa sorte! %s começa.", player_names[0]);
        else
            snprintf(b, sizeof b, "Boa sorte, fantasminha!");
        set_toast(b);
    }
}

void ui_init(unsigned int s)
{
    seed = s;
    sprites_init();
    art_init();
    screen = SCR_TITLE;
    dirty = 1;
}

/* ------------------------------------------------------------------ */
/* Atualização */

static void open_rules(void)
{
    rules_return = screen;
    screen = SCR_RULES;
    rules_page = 0;
}

static void update_title(unsigned int p)
{
    if (p & B_UP)
        title_sel = (title_sel + 3) % 4;
    if (p & B_DOWN)
        title_sel = (title_sel + 1) % 4;
    if (title_sel == 1 && (p & (B_LEFT | B_RIGHT))) {
        nplayers += (p & B_RIGHT) ? 1 : -1;
        if (nplayers < 1) nplayers = 6;
        if (nplayers > 6) nplayers = 1;
    }
    if (title_sel == 2 && (p & (B_LEFT | B_RIGHT | B_CROSS)))
        hints = !hints;
    if (p & (B_CROSS | B_START)) {
        if (title_sel == 0 || (p & B_START))
            new_game();
        else if (title_sel == 3)
            open_rules();
        else if (title_sel == 1 && (p & B_CROSS))
            nplayers = nplayers % 6 + 1;
    }
}

static void update_rules(unsigned int p)
{
    if (p & (B_RIGHT | B_CROSS | B_R1)) {
        if (rules_page < NUM_RULE_PAGES - 1)
            rules_page++;
        else if (p & B_CROSS)
            screen = rules_return;
    }
    if (p & (B_LEFT | B_L1))
        if (rules_page > 0)
            rules_page--;
    if (p & (B_CIRCLE | B_TRIANGLE | B_START))
        screen = rules_return;
}

static void move_town_cursor(unsigned int p)
{
    int c = town_sel % 3, r = town_sel / 3;
    if (p & B_LEFT) c = (c + 2) % 3;
    if (p & B_RIGHT) c = (c + 1) % 3;
    if (p & B_UP) r = (r + 1) % 2;
    if (p & B_DOWN) r = (r + 1) % 2;
    town_sel = r * 3 + c;
}

static void update_game(unsigned int p)
{
    if (anim_t >= 0)
        return; /* animação em andamento */
    if (G.result != GS_PLAYING)
        return;
    if (p & B_START) {
        screen = SCR_PAUSE;
        pause_sel = 0;
        return;
    }
    if (p & B_TRIANGLE) {
        open_rules();
        return;
    }
    if (phase == PH_DRAW) {
        if (p & (B_LEFT | B_RIGHT))
            draw_sel = !draw_sel;
        if (draw_sel == 1 && G.tourist < 0)
            draw_sel = 0;
        if (p & B_CROSS) {
            int ok = draw_sel ? game_draw_tourist(&G) : game_draw_deck(&G);
            if (ok) {
                phase = PH_PLACE;
                town_sel = first_valid_town();
            }
        }
    } else {
        if (p & (B_LEFT | B_RIGHT | B_UP | B_DOWN))
            move_town_cursor(p);
        if (p & B_CROSS) {
            int t = town_sel, newtown = (t == G.ntowns);
            int slot = newtown ? 0 : G.towns[t].ncards;
            if (game_play(&G, t)) {
                anim_t = 0;
                anim_town = t;
                anim_slot = slot;
                phase = PH_DRAW;
                draw_sel = 0;
                happy_town = t;
                happy_t = 70;
                if (newtown) {
                    char b[96];
                    snprintf(b, sizeof b, "Cidade %d fundada: novo prefeito eleito!", t + 1);
                    set_toast(b);
                } else if (G.nplayers > 1 && G.result == GS_PLAYING) {
                    char b[64];
                    snprintf(b, sizeof b, "Vez de %s!", player_names[G.player]);
                    set_toast(b);
                }
            } else if (t > G.ntowns) {
                set_toast("Funde as cidades em ordem: escolha a próxima vaga.");
            } else {
                set_toast("Essa cidade já tem 4 cartas.");
            }
        }
    }
}

static void update_pause(unsigned int p)
{
    if (p & B_UP) pause_sel = (pause_sel + 3) % 4;
    if (p & B_DOWN) pause_sel = (pause_sel + 1) % 4;
    if (p & (B_CIRCLE | B_START)) {
        screen = SCR_GAME;
        return;
    }
    if (p & B_CROSS) {
        switch (pause_sel) {
        case 0: screen = SCR_GAME; break;
        case 1: screen = SCR_GAME; open_rules(); break;
        case 2: new_game(); break;
        case 3: screen = SCR_TITLE; break;
        }
    }
}

static void update_end(unsigned int p)
{
    if (p & B_SQUARE)
        hide_overlay = !hide_overlay;
    if (p & (B_CROSS | B_START))
        new_game();
    else if (p & B_CIRCLE)
        screen = SCR_TITLE;
}

void ui_update(unsigned int p)
{
    int prev_screen = screen;
    frame++;
    if (p)
        dirty = 1;
    switch (screen) {
    case SCR_TITLE: update_title(p); break;
    case SCR_RULES: update_rules(p); break;
    case SCR_GAME: update_game(p); break;
    case SCR_PAUSE: update_pause(p); break;
    case SCR_END: update_end(p); break;
    }
    if (screen != prev_screen)
        dirty = 1;
    if (anim_t >= 0) {
        anim_t++;
        if (anim_t > ANIM_LEN) {
            anim_t = -1;
            dirty = 1;
        }
    }
    if (screen == SCR_GAME && anim_t < 0 && G.result != GS_PLAYING) {
        if (++end_delay > 40) {
            screen = SCR_END;
            dirty = 1;
        }
    }
    if (toast_t > 0)
        toast_t--;
    if (happy_t > 0 && --happy_t == 0)
        happy_town = -1;
}

/* ------------------------------------------------------------------ */
/* Desenho */

static void hint_bar(int x, int y, const int *icons, const char *const *labels, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (icons[i] >= 0) {
            art_button(icons[i], x + 9, y + 9);
            x += 22;
        }
        x += gfx_text(&font_small, x, y + 1, labels[i], COL_DIM, 255) + 16;
    }
}

static const char *status_word(int st)
{
    switch (st) {
    case ST_MET: return "cumprido agora";
    case ST_BROKEN: return "IMPOSSÍVEL";
    default: return "em aberto";
    }
}

static void draw_counts(int x, int y, const int c[NUM_LM])
{
    int k;
    char b[8];
    for (k = 0; k < NUM_LM; k++) {
        int gx = x + k * 46;
        art_landmark(k, (float)gx + 11, (float)y + 8, 16, 255);
        snprintf(b, sizeof b, "%d", c[k]);
        gfx_text(&font_small, gx + 24, y, b, c[k] ? COL_TEXT : 0x7D6E9C, 255);
    }
}

static void draw_town(int i, int selected, int preview)
{
    int x, y, k;
    char req[160];
    town_xy(i, &x, &y);
    if (i < G.ntowns) {
        const Town *t = &G.towns[i];
        int c[NUM_LM], th, st = game_req_status(&G, i);
        int edge = selected ? COL_GOLD : (st == ST_BROKEN ? COL_BAD : COL_PANEL_EDGE);
        gfx_rrect(x + 3, y + 4, TOWN_W, TOWN_H, 12, 0x000000, 90);
        gfx_rrect_border(x, y, TOWN_W, TOWN_H, 12, selected ? 3 : 2, edge, COL_PANEL, 235);
        /* balão com o pedido */
        gfx_tri((float)x + 20, (float)y + 60, (float)x + 38, (float)y + 60, (float)x + 22, (float)y + 76,
                COL_OUTLINE, 255);
        gfx_rrect_border(x + 6, y + 6, TOWN_W - 12, 58, 10, 2, COL_OUTLINE,
                         st == ST_BROKEN ? 0xFFD9DE : COL_BUBBLE, 255);
        gfx_tri((float)x + 23, (float)y + 62, (float)x + 35, (float)y + 62, (float)x + 23.5f, (float)y + 72,
                st == ST_BROKEN ? 0xFFD9DE : COL_BUBBLE, 255);
        card_req_text(&card_db[t->cards[0]], req, sizeof req);
        th = gfx_text_wrap(&font_small, x + 24, 0, TOWN_W - 48, ALIGN_CENTER, 16, req, COL_INK, 255, 0);
        gfx_text_wrap(&font_small, x + 24, y + 6 + (58 - th) / 2, TOWN_W - 48, ALIGN_CENTER, 16, req,
                      COL_INK, 255, 1);
        {
            char nm[8];
            snprintf(nm, sizeof nm, "%d", i + 1);
            gfx_circle((float)x + 10, (float)y + 10, 10, 0x4A3277, 255);
            gfx_text_al(&font_small, x + 10, y + 2, ALIGN_CENTER, nm, COL_GOLD, 255);
        }
        art_status_badge((float)(x + TOWN_W - 10), (float)(y + 10), st, 0);
        /* cartas */
        for (k = 0; k < TOWN_CARDS; k++) {
            int sx, sy;
            slot_xy(i, k, &sx, &sy);
            if (k == 0 && t->tucked >= 0 && !(anim_t >= 0 && anim_town == i)) {
                art_card_back(sx, sy + 7, CARD_W, CARD_H, 0);
                art_star((float)sx + CARD_W / 2.0f, (float)sy + CARD_H + 3, 3.5f, COL_GOLD, 255);
            }
            if (k < t->ncards && !(anim_t >= 0 && anim_town == i && anim_slot == k)) {
                art_card_face(sx, sy, CARD_W, CARD_H, t->cards[k], k == 0 ? COL_GOLD : COL_CARD_EDGE);
            } else if (k >= t->ncards) {
                if (preview && k == t->ncards && G.hand >= 0)
                    art_card_face(sx, sy, CARD_W, CARD_H, G.hand, COL_GOLD);
                else
                    gfx_dashed_rect(sx, sy, CARD_W, CARD_H, 4, 1, 0x7D6E9C, 200);
            }
        }
        town_counts(&G, i, c);
        draw_counts(x + 12, y + TOWN_H - 20, c);
    } else {
        int next = (i == G.ntowns);
        gfx_rrect(x, y, TOWN_W, TOWN_H, 12, COL_PANEL, next ? 200 : 160);
        gfx_dashed_rect(x + 4, y + 4, TOWN_W - 8, TOWN_H - 8, 7, 2, selected ? COL_GOLD : 0x6A5A8E,
                        selected ? 255 : 160);
        if (next && preview && G.hand >= 0) {
            int sx, sy;
            slot_xy(i, 0, &sx, &sy);
            art_card_face(sx, sy, CARD_W, CARD_H, G.hand, COL_GOLD);
            gfx_text_al(&font_body, x + TOWN_W / 2, y + 24, ALIGN_CENTER, "Novo prefeito!", COL_GOLD, 255);
        } else {
            gfx_text_al(&font_body, x + TOWN_W / 2, y + TOWN_H / 2 - 10, ALIGN_CENTER,
                        next ? "Nova cidade" : "Vaga", next ? COL_TEXT : 0x6A5A8E, 255);
        }
    }
}

static void draw_header(void)
{
    char b[64];
    art_title_text(22 + gfx_text_width(&font_big, "Prefeitos do Além") / 2, 9, "Prefeitos do Além", COL_GOLD);
    snprintf(b, sizeof b, "Cartas jogadas: %d/%d", G.played, CARDS_TO_PLAY);
    gfx_text_al(&font_body, 618, 8, ALIGN_RIGHT, b, COL_TEXT, 255);
    if (G.nplayers > 1)
        snprintf(b, sizeof b, "Vez de %s (jogador %d de %d)", player_names[G.player], G.player + 1, G.nplayers);
    else
        snprintf(b, sizeof b, "Partida solo");
    gfx_text_al(&font_small, 618, 29, ALIGN_RIGHT, b, COL_DIM, 255);
}

static void draw_deck_area(void)
{
    char b[32];
    int sel_deck = (screen == SCR_GAME && phase == PH_DRAW && draw_sel == 0);
    int sel_tour = (screen == SCR_GAME && phase == PH_DRAW && draw_sel == 1);
    /* monte */
    if (sel_deck)
        gfx_rrect(18, 342, 66, 98, 10, COL_GOLD, 255);
    if (G.deck_count > 0)
        art_card_back(24, 348, 54, 72, 0);
    else
        gfx_dashed_rect(24, 348, 54, 72, 5, 2, 0x6A5A8E, 255);
    snprintf(b, sizeof b, "Monte %d", G.deck_count);
    gfx_text_al(&font_small, 51, 422, ALIGN_CENTER, b, sel_deck ? COL_INK : COL_DIM, 255);
    /* turista */
    if (sel_tour)
        gfx_rrect(90, 342, 66, 98, 10, COL_GOLD, 255);
    if (G.tourist >= 0)
        art_card_face(96, 348, 54, 72, G.tourist, COL_CARD_EDGE);
    else
        gfx_dashed_rect(96, 348, 54, 72, 5, 2, 0x6A5A8E, 255);
    gfx_text_al(&font_small, 123, 422, ALIGN_CENTER, "Turista", sel_tour ? COL_INK : COL_DIM, 255);
}

/* simula a jogada para avisar se algum pedido ficaria impossível */
static int simulate_risk(int town)
{
    Game sim = G;
    if (!game_play(&sim, town))
        return -2;
    if (sim.result == GS_LOST)
        return sim.fail_town;
    return -1;
}

static void req_bubble(int x, int y, int w, const char *req)
{
    int th = gfx_text_wrap(&font_small, x + 8, 0, w - 16, ALIGN_LEFT, 16, req, COL_INK, 255, 0);
    if (th < 16)
        th = 16;
    gfx_rrect_border(x, y, w, th + 8, 8, 2, COL_OUTLINE, COL_BUBBLE, 255);
    gfx_text_wrap(&font_small, x + 8, y + 4, w - 16, ALIGN_LEFT, 16, req, COL_INK, 255, 1);
}

static void draw_info_panel(void)
{
    char b[200], req[160];
    int x = 166, y = 340, w = 452;
    gfx_rrect_border(x, y, w, 104, 10, 2, COL_PANEL_EDGE, COL_PANEL, 235);
    if (phase == PH_DRAW) {
        spr_draw_base(player_avatar[G.nplayers > 1 ? G.player : 5], x + 40, y + 100, 255);
        x += 70;
        w -= 70;
        if (G.nplayers > 1)
            snprintf(b, sizeof b, "%s, compre uma carta", player_names[G.player]);
        else
            snprintf(b, sizeof b, "Compre uma carta");
        gfx_text(&font_body, x + 12, y + 6, b, COL_GOLD, 255);
        if (draw_sel == 1 && G.tourist >= 0) {
            gfx_text(&font_small, x + 12, y + 30, "Turista - pedido, se virar prefeito:", COL_TEXT, 255);
            card_req_text(&card_db[G.tourist], req, sizeof req);
            req_bubble(x + 10, y + 48, w - 20, req);
        } else {
            snprintf(b, sizeof b, "Monte: carta surpresa (restam %d).\nEsquerda/direita: monte ou turista.",
                     G.deck_count);
            gfx_text_wrap(&font_small, x + 12, y + 32, w - 24, ALIGN_LEFT, 17, b, COL_TEXT, 255, 1);
        }
        {
            static const int ic[] = { BTN_ICON_CROSS, BTN_ICON_TRIANGLE, -1 };
            static const char *const lb[] = { "Comprar", "Regras", "START  Menu" };
            if (!(draw_sel == 1 && G.tourist >= 0))
                hint_bar(x + 12, y + 80, ic, lb, 3);
        }
    } else {
        int t = town_sel, line = y + 30;
        art_card_face(HAND_X, HAND_Y, 50, 72, G.hand, COL_GOLD);
        gfx_text_al(&font_small, HAND_X + 25, y + 84, ALIGN_CENTER, "Na mão", COL_DIM, 255);
        x += 62;
        w -= 62;
        if (t == G.ntowns) {
            snprintf(b, sizeof b, "Fundar a Cidade %d: vira PREFEITO!", t + 1);
            gfx_text(&font_body, x + 4, y + 6, b, COL_GOLD, 255);
            card_req_text(&card_db[G.hand], req, sizeof req);
            req_bubble(x + 2, y + 30, w - 12, req);
            line = y + 66;
        } else if (t < G.ntowns) {
            int st = game_req_status(&G, t);
            snprintf(b, sizeof b, "Cidade %d (%d/4 cartas)", t + 1, G.towns[t].ncards);
            gfx_text(&font_body, x + 4, y + 6, b, G.towns[t].ncards < 4 ? COL_GOLD : COL_DIM, 255);
            snprintf(b, sizeof b, "Pedido do prefeito: %s", status_word(st));
            gfx_text(&font_small, x + 4, line, b,
                     st == ST_MET ? COL_OK : (st == ST_BROKEN ? COL_BAD : COL_TEXT), 255);
            line += 18;
            if (G.towns[t].ncards >= 4) {
                gfx_text(&font_small, x + 4, line, "Cidade completa: escolha outra.", COL_DIM, 255);
                line += 18;
            }
        } else {
            gfx_text(&font_body, x + 4, y + 6, "Vaga ainda fechada", COL_DIM, 255);
            gfx_text(&font_small, x + 4, line, "As cidades são fundadas em ordem.", COL_TEXT, 255);
            line += 18;
        }
        if (hints && game_can_play(&G, t)) {
            int r = simulate_risk(t);
            if (r >= 0) {
                snprintf(b, sizeof b, "Cuidado: o pedido da Cidade %d ficaria impossível!", r + 1);
                spr_draw_center(SPR_FX_EXCLAMACAO, x + 10, line + 8, 255);
                gfx_text(&font_small, x + 24, line, b, COL_BAD, 255);
            } else {
                spr_draw_center(SPR_FX_LAMPADA, x + 10, line + 7, 255);
                gfx_text(&font_small, x + 24, line, "Dica: nenhum pedido fica impossível.", COL_OK, 255);
            }
        }
        {
            static const int ic[] = { BTN_ICON_CROSS, BTN_ICON_TRIANGLE, -1 };
            static const char *const lb[] = { "Jogar aqui", "Regras", "START  Menu" };
            if (t != G.ntowns)
                hint_bar(x + 4, y + 80, ic, lb, 3);
        }
    }
}

static void draw_game_static(void)
{
    int i;
    draw_header();
    for (i = 0; i < MAX_TOWNS; i++) {
        int sel = (screen == SCR_GAME && phase == PH_PLACE && i == town_sel);
        draw_town(i, sel, sel);
    }
    draw_deck_area();
    if (G.result == GS_PLAYING)
        draw_info_panel();
}

static int mayor_sprite(int town)
{
    int st = game_req_status(&G, town);
    if (G.result == GS_WON)
        return SPR_EXPR_RINDO;
    if (st == ST_BROKEN)
        return SPR_EXPR_TRISTE;
    if (town == happy_town)
        return SPR_EXPR_ANIMADO;
    if (st == ST_MET)
        return SPR_EXPR_FELIZ;
    if (screen == SCR_GAME && phase == PH_PLACE && town == town_sel)
        return SPR_EXPR_PENSATIVO;
    return SPR_EXPR_NORMAL;
}

static void draw_town_ghosts(void)
{
    int i;
    for (i = 0; i < G.ntowns; i++) {
        int x, y;
        float bob = sinf((float)(frame + i * 17) * 0.08f) * 2.0f;
        town_xy(i, &x, &y);
        spr_draw_base(mayor_sprite(i), x + 26, y + 116 + (int)bob, 255);
    }
}

static void draw_anim_card(void)
{
    int sx, sy;
    float t;
    float x, y;
    if (anim_t < 0)
        return;
    slot_xy(anim_town, anim_slot, &sx, &sy);
    t = (float)anim_t / ANIM_LEN;
    t = t * t * (3 - 2 * t);
    x = HAND_X + (sx - HAND_X) * t;
    y = HAND_Y + (sy - HAND_Y) * t - sinf(t * 3.14159f) * 40;
    art_card_face((int)x, (int)y, CARD_W + (int)(16 * (1 - t)), CARD_H + (int)(26 * (1 - t)),
                  G.towns[anim_town].cards[anim_slot], COL_GOLD);
}

static void draw_toast(void)
{
    int w, a;
    if (toast_t <= 0)
        return;
    a = toast_t > 20 ? 255 : toast_t * 12;
    w = gfx_text_width(&font_body, toast) + 32;
    gfx_rrect(320 - w / 2, 196, w, 34, 17, 0x120A26, a * 220 / 255);
    gfx_text_al(&font_body, 320, 203, ALIGN_CENTER, toast, COL_GOLD, a);
}

static void draw_title_static(void)
{
    static const char *const items[] = { "Jogar", "Jogadores", "Dicas", "Como jogar" };
    int i;
    spr_draw(SPR_LOGO_BIG, 320 - spr(SPR_LOGO_BIG)->w / 2, 4, 255);
    for (i = 0; i < 4; i++) {
        char b[48];
        if (i == 1)
            snprintf(b, sizeof b, "Jogadores:  < %d >", nplayers);
        else if (i == 2)
            snprintf(b, sizeof b, "Dicas:  %s", hints ? "ligadas" : "desligadas");
        else
            snprintf(b, sizeof b, "%s", items[i]);
        art_menu_button(320, 228 + i * 46, 250, 40, b, i == title_sel);
    }
}

static void draw_title_dynamic(void)
{
    /* personagens dos dois lados, flutuando */
    static const int cx[6] = { 88, 552, 66, 150, 494, 578 };
    static const int base[6] = { 196, 196, 336, 438, 438, 336 };
    int i;
    for (i = 0; i < 6; i++) {
        float bob = sinf((float)frame * 0.06f + i * 1.3f) * 5;
        spr_draw_base(title_chars[i], cx[i], base[i] + (int)bob, 255);
    }
    if ((frame / 40) % 3 == 0)
        spr_draw_center(SPR_FX_BRILHO, 470, 40, 255);
}

static const char *const rule_titles[NUM_RULE_PAGES] = {
    "Objetivo", "Seu turno", "Prefeitos e pedidos", "Fim de jogo",
};

static const char *const rule_text[NUM_RULE_PAGES] = {
    "Os fantasminhas do Além querem eleger prefeitos para 6 novas cidades! "
    "Joguem juntos (1 a 6 jogadores, passando o controle) para formar 6 cidades "
    "com 4 cartas cada.\n\n"
    "A primeira carta de cada cidade vira o PREFEITO, e o pedido escrito nela "
    "precisa ser cumprido.\n\n"
    "Quando as 24 cartas forem jogadas, vocês vencem se os 6 prefeitos estiverem satisfeitos.",

    "1. Compre a carta do topo do MONTE ou o TURISTA (a carta virada para cima ao lado do monte).\n\n"
    "2. Jogue a carta numa cidade com menos de 4 cartas, ou funde uma cidade nova se ainda "
    "houver menos de 6.\n\n"
    "3. Se você pegou o turista, um novo turista é virado do monte.\n\n"
    "Conversem à vontade sobre a jogada antes de confirmar!",

    "Cada carta mostra 2 marcos: \x01" "casas\x05, \x02" "árvores\x05, \x03" "lagos\x05 ou \x04" "lojas\x05.\n\n"
    "Ao fundar uma cidade, a carta do topo do monte vai virada para baixo embaixo do prefeito "
    "(estrela) e o pedido fica ATIVO. Pedidos das outras cartas não valem.\n\n"
    "\"Aqui\" = marcos da cidade, incluindo os do prefeito. \"Outro\" = além dos marcos da "
    "carta do prefeito. \"Mais que qualquer outra\" não aceita empate.",

    "Se o pedido de algum prefeito não puder mais ser cumprido, vocês perdem na hora.\n\n"
    "Com 24 cartas jogadas e os 6 pedidos cumpridos: vitória!\n\n"
    "Selos: estrela = pedido em aberto, verde = cumprido no momento, vermelho = impossível. "
    "Com as dicas ligadas, o jogo avisa antes de uma jogada que tornaria um pedido impossível.",
};

static void draw_rules_static(void)
{
    char b[32];
    int y;
    gfx_rrect_border(40, 24, 560, 400, 16, 2, COL_PANEL_EDGE, COL_PANEL, 240);
    art_title_text(320, 38, rule_titles[rules_page], COL_GOLD);
    /* fundo claro para as cores dos marcos aparecerem bem */
    gfx_rrect_border(64, 82, 512, 250, 12, 2, COL_OUTLINE, COL_BUBBLE, 255);
    gfx_text_wrap(&font_body, 84, 94, 472, ALIGN_LEFT, 21, rule_text[rules_page], COL_INK, 255, 1);
    y = 344;
    if (rules_page == 2) {
        int k;
        for (k = 0; k < NUM_LM; k++)
            art_landmark(k, 200.0f + k * 80, (float)y + 22, 44, 255);
    } else if (rules_page == 3) {
        art_status_badge(220, (float)y + 20, ST_OPEN, 0);
        art_status_badge(320, (float)y + 20, ST_MET, 0);
        art_status_badge(420, (float)y + 20, ST_BROKEN, 0);
    } else if (rules_page == 1) {
        art_card_back(250, y - 6, 40, 54, 0);
        art_card_face(300, y - 6, 40, 54, 21, COL_CARD_EDGE);
        art_card_face(350, y - 6, 40, 54, 14, COL_GOLD);
    } else {
        int k;
        for (k = 0; k < 6; k++)
            spr_draw_base(player_avatar[k], 170 + k * 60, y + 50, 255);
    }
    snprintf(b, sizeof b, "%d / %d", rules_page + 1, NUM_RULE_PAGES);
    gfx_text_al(&font_small, 578, 398, ALIGN_RIGHT, b, COL_DIM, 255);
    {
        static const int ic[] = { -1, BTN_ICON_CIRCLE };
        static const char *const lb[] = { "Esquerda/direita: páginas", "Voltar" };
        hint_bar(60, 396, ic, lb, 2);
    }
}

static void draw_pause(void)
{
    static const char *const items[] = { "Continuar", "Como jogar", "Reiniciar partida", "Voltar ao título" };
    int i;
    gfx_fill(0, 0, SCR_W, SCR_H, 0x000000, 140);
    gfx_rrect_border(170, 70, 300, 290, 18, 3, COL_OUTLINE, COL_PANEL, 250);
    art_title_text(320, 86, "Pausa", COL_GOLD);
    for (i = 0; i < 4; i++)
        art_menu_button(320, 136 + i * 52, 260, 40, items[i], i == pause_sel);
}

static void draw_end_overlay(void)
{
    char b[200], req[160];
    int won = (G.result == GS_WON);
    if (hide_overlay) {
        static const int ic[] = { BTN_ICON_SQUARE };
        static const char *const lb[] = { "Mostrar resultado" };
        gfx_rrect_border(166, 340, 452, 104, 10, 2, COL_PANEL_EDGE, COL_PANEL, 235);
        hint_bar(180, 382, ic, lb, 1);
        return;
    }
    gfx_fill(0, 0, SCR_W, SCR_H, 0x000000, 120);
    gfx_rrect_border(90, 96, 460, 250, 18, 3, won ? COL_GOLD : COL_BAD, COL_PANEL, 250);
    gfx_text_shadow(&font_title, 320, 108, ALIGN_CENTER, won ? "Vitória!" : "Derrota...", won ? COL_GOLD : COL_BAD,
                    COL_OUTLINE);
    if (won) {
        gfx_text_wrap(&font_body, 120, 170, 400, ALIGN_CENTER, 24,
                      "Os 6 prefeitos estão felizes e as cidades do Além brilham na noite!", COL_TEXT, 255, 1);
    } else if (G.fail_town >= 0) {
        card_req_text(&card_db[G.towns[G.fail_town].cards[0]], req, sizeof req);
        snprintf(b, sizeof b, "O pedido da Cidade %d não pode ser cumprido:", G.fail_town + 1);
        gfx_text_wrap(&font_body, 120, 166, 400, ALIGN_CENTER, 22, b, COL_TEXT, 255, 1);
        gfx_rrect_border(120, 216, 400, 50, 10, 2, COL_OUTLINE, COL_BUBBLE, 255);
        gfx_text_wrap(&font_small, 130, 224, 380, ALIGN_CENTER, 17, req, COL_INK, 255, 1);
    }
    {
        static const int ic[] = { BTN_ICON_CROSS, BTN_ICON_CIRCLE, BTN_ICON_SQUARE };
        static const char *const lb[] = { "Jogar de novo", "Título", "Ver cidades" };
        hint_bar(132, 306, ic, lb, 3);
    }
}

static void draw_end_dynamic(void)
{
    if (hide_overlay)
        return;
    if (G.result == GS_WON) {
        int f = (frame / 6) % 6;
        spr_draw_base(SPR_FESTA0 + f, 150, 166, 255);
        spr_draw_base(SPR_FESTA0 + (f + 3) % 6, 490, 166, 255);
        if ((frame / 12) % 2)
            spr_draw_center(SPR_FX_ESTRELAS, 200, 106, 255);
        else
            spr_draw_center(SPR_FX_BRILHO, 440, 106, 255);
    } else {
        float b = sinf((float)frame * 0.05f) * 3;
        spr_draw_base(SPR_EXPR_BIG_TRISTE, 150, 162 + (int)b, 255);
        spr_draw_base(SPR_EXPR_BIG_PENSATIVO, 490, 162 - (int)b, 255);
    }
}

static void render_static(void)
{
    if (!bg_ready) {
        gfx_target(bg);
        art_background();
        bg_ready = 1;
    }
    gfx_target(cache);
    gfx_copy(bg);
    switch (screen) {
    case SCR_TITLE: draw_title_static(); break;
    case SCR_RULES: draw_rules_static(); break;
    case SCR_GAME:
    case SCR_PAUSE:
    case SCR_END: draw_game_static(); break;
    }
}

void ui_draw_to(unsigned int *out)
{
    if (dirty || anim_t >= 0) {
        render_static();
        dirty = 0;
    }
    gfx_target(out);
    gfx_copy(cache);
    switch (screen) {
    case SCR_TITLE: draw_title_dynamic(); break;
    case SCR_GAME:
    case SCR_PAUSE:
    case SCR_END:
        draw_town_ghosts();
        draw_anim_card();
        draw_toast();
        if (screen == SCR_PAUSE)
            draw_pause();
        if (screen == SCR_END) {
            draw_end_overlay();
            draw_end_dynamic();
        }
        break;
    default: break;
    }
}
