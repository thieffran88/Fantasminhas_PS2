#ifndef GAME_H
#define GAME_H

/*
 * Prefeitos do Além - motor de regras (C puro, sem dependência de plataforma).
 *
 * Regras (resumo):
 *  - Jogo cooperativo para 1 a 6 jogadores, com 50 cartas de cidade.
 *  - Objetivo: formar 6 cidades com 4 cartas cada (24 cartas jogadas).
 *    A primeira carta de cada cidade é o seu PREFEITO.
 *  - Preparação: embaralhar; virar 1 carta ao lado do monte: o TURISTA.
 *  - No turno: 1) compre a carta do topo do monte OU o turista;
 *              2) jogue-a numa cidade com menos de 4 cartas (ou funde uma
 *                 cidade nova, se houver menos de 6);
 *              3) se pegou o turista, vire um novo turista do monte.
 *  - A primeira carta de uma cidade vira prefeito: a carta do topo do monte
 *    é colocada virada para baixo embaixo dela (o pedido fica ATIVO).
 *  - Os pedidos das cartas que não são prefeito não têm efeito.
 *  - Se algum pedido de prefeito não puder mais ser cumprido: derrota.
 *  - Com 24 cartas jogadas e os 6 pedidos cumpridos: vitória!
 */

enum { CASA = 0, ARVORE = 1, LAGO = 2, LOJA = 3, NUM_LM = 4 };

typedef enum {
    R_ATLEAST2,       /* Pelo menos 2 A e 2 B aqui. */
    R_NONE,           /* Nenhum A aqui. */
    R_NONE_NOOTHER,   /* Nenhum A e nenhum outro B aqui. */
    R_EXACT_TYPES,    /* Exatamente N tipos de marcos aqui. */
    R_EXACT_COUNT,    /* Exatamente N A aqui. */
    R_MOST,           /* Mais A do que qualquer outra cidade (sem empate). */
    R_TOWNS_WITH,     /* Exatamente N cidades com A. */
    R_TOWNS_BOTH,     /* Exatamente N cidades com A e B. */
    R_MORE_THAN,      /* Mais A do que B aqui. */
    R_SAME,           /* Mesmo número de A e B aqui. */
    R_ALL_TOWNS       /* Todas as cidades têm A. */
} ReqType;

typedef struct {
    unsigned char lm[2];   /* os dois marcos impressos na carta */
    unsigned char req;     /* ReqType */
    signed char a, b, n;   /* parâmetros do pedido */
} Card;

#define DECK_SIZE 50
#define MAX_TOWNS 6
#define TOWN_CARDS 4
#define CARDS_TO_PLAY (MAX_TOWNS * TOWN_CARDS)

typedef enum { ST_OPEN, ST_MET, ST_BROKEN } ReqStatus;

typedef enum { GS_PLAYING, GS_WON, GS_LOST } GameResult;

typedef struct {
    int ncards;
    int cards[TOWN_CARDS];  /* índices em card_db; cards[0] é o prefeito */
    int tucked;             /* carta escondida embaixo do prefeito (-1 se não há) */
} Town;

typedef struct {
    int deck[DECK_SIZE];
    int deck_count;         /* topo = deck[deck_count-1] */
    int tourist;            /* -1 se não há turista */
    Town towns[MAX_TOWNS];
    int ntowns;
    int played;
    int hand;               /* carta comprada, aguardando jogada (-1 = nenhuma) */
    int hand_from_tourist;
    int nplayers, player;
    int result;             /* GameResult */
    int fail_town;          /* cidade cujo pedido falhou (-1) */
    unsigned int rng;
} Game;

extern const Card card_db[DECK_SIZE];

unsigned int game_rand(Game *g);
void game_init(Game *g, int nplayers, unsigned int seed);
int game_draw_deck(Game *g);       /* 1 se ok */
int game_draw_tourist(Game *g);    /* 1 se ok */
int game_can_play(const Game *g, int town); /* town == ntowns => cidade nova */
int game_play(Game *g, int town);  /* 1 se ok; atualiza result */

void town_counts(const Game *g, int town, int counts[NUM_LM]);
int town_types(const int counts[NUM_LM]);
ReqStatus game_req_status(const Game *g, int town);
int game_req_final(const Game *g, int town);  /* avalia como se o jogo tivesse terminado */
void game_check(Game *g);

/* Gera o texto do pedido. Marcos vêm marcados com bytes de cor
 * (TXT_LM0 + tipo ... TXT_RESET), interpretados pelo renderizador. */
#define TXT_LM0 '\x01'
#define TXT_RESET '\x05'
void card_req_text(const Card *c, char *out, int outsz);

#endif
