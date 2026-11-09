#include "lang.h"

int g_lang = LANG_PT;

/* { português, inglês, espanhol } */
static const char *const strings[NUM_STRINGS][NUM_LANGS] = {
    [S_LANG_NAME] = { "Português", "English", "Español" },
    [S_PRESS_START] = { "Pressione START", "Press START", "Pulsa START" },
    [S_MENU_PLAY] = { "Jogar", "Play", "Jugar" },
    [S_MENU_PLAYERS] = { "Jogadores", "Players", "Jugadores" },
    [S_MENU_HINTS] = { "Dicas", "Hints", "Pistas" },
    [S_HINTS_ON] = { "ligadas", "on", "sí" },
    [S_HINTS_OFF] = { "desligadas", "off", "no" },
    [S_MENU_LANGUAGE] = { "Idioma", "Language", "Idioma" },
    [S_MENU_HOWTO] = { "Como jogar", "How to play", "Cómo jugar" },
    [S_GOOD_LUCK_STARTS] = { "Boa sorte! %s começa.", "Good luck! %s goes first.", "¡Suerte! Empieza %s." },
    [S_GOOD_LUCK_SOLO] = { "Boa sorte, fantasminha!", "Good luck, little ghost!", "¡Suerte, fantasmita!" },
    [S_TOWN_FOUNDED] = { "Cidade %d fundada: novo prefeito eleito!", "Town %d founded: a new mayor is elected!",
                         "¡Ciudad %d fundada: nuevo alcalde electo!" },
    [S_TURN_OF] = { "Vez de %s!", "%s's turn!", "¡Turno de %s!" },
    [S_FOUND_IN_ORDER] = { "Funde as cidades em ordem: escolha a próxima vaga.",
                           "Found towns in order: pick the next free spot.",
                           "Funda las ciudades en orden: elige el siguiente hueco." },
    [S_TOWN_FULL_TOAST] = { "Essa cidade já tem 4 cartas.", "That town already has 4 cards.",
                            "Esa ciudad ya tiene 4 cartas." },
    [S_ST_MET] = { "cumprido agora", "met right now", "cumplida ahora" },
    [S_ST_BROKEN] = { "IMPOSSÍVEL", "IMPOSSIBLE", "IMPOSIBLE" },
    [S_ST_OPEN] = { "em aberto", "still open", "pendiente" },
    [S_NEW_MAYOR] = { "Novo prefeito!", "New mayor!", "¡Nuevo alcalde!" },
    [S_NEW_TOWN] = { "Nova cidade", "New town", "Nueva ciudad" },
    [S_SLOT] = { "Vaga", "Empty", "Vacío" },
    [S_CARDS_PLAYED] = { "Cartas jogadas: %d/%d", "Cards played: %d/%d", "Cartas jugadas: %d/%d" },
    [S_TURN_HEADER] = { "Vez de %s (jogador %d de %d)", "%s's turn (player %d of %d)",
                        "Turno de %s (jugador %d de %d)" },
    [S_SOLO] = { "Partida solo", "Solo game", "Partida en solitario" },
    [S_DECK_N] = { "Monte %d", "Deck %d", "Mazo %d" },
    [S_TOURIST] = { "Turista", "Tourist", "Turista" },
    [S_PLAYER_DRAW] = { "%s, compre uma carta", "%s, draw a card", "%s, roba una carta" },
    [S_DRAW] = { "Compre uma carta", "Draw a card", "Roba una carta" },
    [S_TOURIST_REQ] = { "Turista - pedido, se virar prefeito:", "Tourist - request, if it becomes mayor:",
                        "Turista - petición, si es alcalde:" },
    [S_DECK_INFO] = { "Monte: carta surpresa (restam %d).\nEsquerda/direita: monte ou turista.",
                      "Deck: a surprise card (%d left).\nLeft/right: deck or tourist.",
                      "Mazo: carta sorpresa (quedan %d).\nIzquierda/derecha: mazo o turista." },
    [S_BTN_BUY] = { "Comprar", "Draw", "Robar" },
    [S_BTN_RULES] = { "Regras", "Rules", "Reglas" },
    [S_BTN_MENU] = { "START  Menu", "START  Menu", "START  Menú" },
    [S_IN_HAND] = { "Na mão", "In hand", "En mano" },
    [S_FOUND_TOWN] = { "Fundar a Cidade %d: vira PREFEITO!", "Found Town %d: it becomes MAYOR!",
                       "Fundar Ciudad %d: ¡será ALCALDE!" },
    [S_TOWN_CARDS] = { "Cidade %d (%d/4 cartas)", "Town %d (%d/4 cards)", "Ciudad %d (%d/4 cartas)" },
    [S_MAYOR_REQ] = { "Pedido do prefeito: %s", "Mayor's request: %s", "Petición del alcalde: %s" },
    [S_TOWN_COMPLETE] = { "Cidade completa: escolha outra.", "Town is full: pick another one.",
                          "Ciudad completa: elige otra." },
    [S_SLOT_CLOSED] = { "Vaga ainda fechada", "Spot not open yet", "Hueco aún cerrado" },
    [S_FOUND_ORDER_INFO] = { "As cidades são fundadas em ordem.", "Towns are founded in order.",
                             "Las ciudades se fundan en orden." },
    [S_WARN_IMPOSSIBLE] = { "Cuidado: isso quebra o pedido da Cidade %d!",
                            "Careful: this breaks Town %d's request!",
                            "¡Ojo: esto rompe la petición de la Ciudad %d!" },
    [S_HINT_SAFE] = { "Dica: nenhum pedido fica impossível.", "Hint: no request becomes impossible.",
                      "Pista: ninguna petición se vuelve imposible." },
    [S_BTN_PLAY_HERE] = { "Jogar aqui", "Play here", "Jugar aquí" },
    [S_REQ_LABEL] = { "Pedido: %s", "Request: %s", "Petición: %s" },
    [S_RULES_NAV] = { "Esquerda/direita: páginas", "Left/right: pages", "Izquierda/derecha: páginas" },
    [S_BTN_BACK] = { "Voltar", "Back", "Volver" },
    [S_PAUSE] = { "Pausa", "Pause", "Pausa" },
    [S_PAUSE_CONTINUE] = { "Continuar", "Continue", "Continuar" },
    [S_PAUSE_RESTART] = { "Reiniciar partida", "Restart game", "Reiniciar partida" },
    [S_PAUSE_TITLE] = { "Voltar ao título", "Back to title", "Volver al título" },
    [S_SHOW_RESULT] = { "Mostrar resultado", "Show result", "Mostrar resultado" },
    [S_VICTORY] = { "Vitória!", "Victory!", "¡Victoria!" },
    [S_DEFEAT] = { "Derrota...", "Defeat...", "Derrota..." },
    [S_WIN_TEXT] = { "Os 6 prefeitos estão felizes e as cidades do Além brilham na noite!",
                     "All 6 mayors are happy and the towns of the Beyond shine in the night!",
                     "¡Los 6 alcaldes están felices y las ciudades del Más Allá brillan en la noche!" },
    [S_FAIL_TEXT] = { "O pedido da Cidade %d não pode ser cumprido:", "Town %d's request can't be met:",
                      "La petición de la Ciudad %d no se puede cumplir:" },
    [S_PLAY_AGAIN] = { "Jogar de novo", "Play again", "Jugar otra vez" },
    [S_BTN_TITLE] = { "Título", "Title", "Título" },
    [S_SEE_TOWNS] = { "Ver cidades", "See towns", "Ver ciudades" },
    [S_NAME_MAYOR] = { "Prefeito", "Mayor", "Alcalde" },

    [S_RULE_T0] = { "Objetivo", "Goal", "Objetivo" },
    [S_RULE_T1] = { "Seu turno", "Your turn", "Tu turno" },
    [S_RULE_T2] = { "Prefeitos e pedidos", "Mayors and requests", "Alcaldes y peticiones" },
    [S_RULE_T3] = { "Fim de jogo", "Game end", "Fin de la partida" },

    [S_RULE_B0] = {
        "Os fantasminhas do Além querem eleger prefeitos para 6 novas cidades! "
        "Joguem juntos (1 a 6 jogadores, passando o controle) para formar 6 cidades "
        "com 4 cartas cada.\n\n"
        "A primeira carta de cada cidade vira o PREFEITO, e o pedido escrito nela "
        "precisa ser cumprido.\n\n"
        "Quando as 24 cartas forem jogadas, vocês vencem se os 6 prefeitos estiverem satisfeitos.",

        "The little ghosts of the Beyond want to elect mayors for 6 new towns! "
        "Play together (1 to 6 players, passing the controller) to build 6 towns "
        "of 4 cards each.\n\n"
        "The first card of each town becomes its MAYOR, and the request written on it "
        "must be met.\n\n"
        "When all 24 cards are played, you win if all 6 mayors are satisfied.",

        "¡Los fantasmitas del Más Allá quieren elegir alcaldes para 6 nuevas ciudades! "
        "Jugad juntos (de 1 a 6 jugadores, pasando el mando) para formar 6 ciudades "
        "de 4 cartas cada una.\n\n"
        "La primera carta de cada ciudad es su ALCALDE, y la petición escrita en ella "
        "debe cumplirse.\n\n"
        "Cuando se jueguen las 24 cartas, ganáis si los 6 alcaldes están satisfechos.",
    },
    [S_RULE_B1] = {
        "1. Compre a carta do topo do MONTE ou o TURISTA (a carta virada para cima ao lado do monte).\n\n"
        "2. Jogue a carta numa cidade com menos de 4 cartas, ou funde uma cidade nova se ainda "
        "houver menos de 6.\n\n"
        "3. Se você pegou o turista, um novo turista é virado do monte.\n\n"
        "Conversem à vontade sobre a jogada antes de confirmar!",

        "1. Draw the top card of the DECK or the TOURIST (the face-up card next to the deck).\n\n"
        "2. Play the card to a town with fewer than 4 cards, or found a new town if there are "
        "fewer than 6.\n\n"
        "3. If you took the tourist, a new tourist is turned up from the deck.\n\n"
        "Feel free to discuss the move before confirming!",

        "1. Roba la carta de arriba del MAZO o el TURISTA (la carta boca arriba junto al mazo).\n\n"
        "2. Juega la carta en una ciudad con menos de 4 cartas, o funda una ciudad nueva si hay "
        "menos de 6.\n\n"
        "3. Si cogiste el turista, se pone un turista nuevo del mazo.\n\n"
        "¡Hablad todo lo que queráis antes de confirmar la jugada!",
    },
    [S_RULE_B2] = {
        "Cada carta mostra 2 marcos: \x01" "casas\x05, \x02" "árvores\x05, \x03" "lagos\x05 ou \x04" "lojas\x05.\n\n"
        "Ao fundar uma cidade, a carta do topo do monte vai virada para baixo embaixo do prefeito "
        "(estrela) e o pedido fica ATIVO. Pedidos das outras cartas não valem.\n\n"
        "\"Aqui\" = marcos da cidade, incluindo os do prefeito. \"Outro\" = além dos marcos da "
        "carta do prefeito. \"Mais que qualquer outra\" não aceita empate.",

        "Each card shows 2 landmarks: \x01" "houses\x05, \x02" "trees\x05, \x03" "lakes\x05 or \x04" "shops\x05.\n\n"
        "When a town is founded, the top card of the deck goes face down under the mayor "
        "(star) and the request becomes ACTIVE. Requests on other cards don't count.\n\n"
        "\"Here\" = landmarks in that town, including the mayor's. \"Other\" = besides the "
        "mayor card's own landmarks. \"More than any other\" allows no ties.",

        "Cada carta muestra 2 lugares: \x01" "casas\x05, \x02" "árboles\x05, \x03" "lagos\x05 o \x04" "tiendas\x05.\n\n"
        "Al fundar una ciudad, la carta de arriba del mazo va boca abajo bajo el alcalde "
        "(estrella) y la petición queda ACTIVA. Las peticiones de otras cartas no cuentan.\n\n"
        "\"Aquí\" = lugares de la ciudad, incluidos los del alcalde. \"Otro\" = además de los de "
        "la carta del alcalde. \"Más que cualquier otra\" no admite empates.",
    },
    [S_RULE_B3] = {
        "Se o pedido de algum prefeito não puder mais ser cumprido, vocês perdem na hora.\n\n"
        "Com 24 cartas jogadas e os 6 pedidos cumpridos: vitória!\n\n"
        "Selos: estrela = pedido em aberto, verde = cumprido no momento, vermelho = impossível. "
        "Com as dicas ligadas, o jogo avisa antes de uma jogada que tornaria um pedido impossível.",

        "If any mayor's request can no longer be met, you lose immediately.\n\n"
        "With 24 cards played and all 6 requests met: victory!\n\n"
        "Badges: star = request open, green = met right now, red = impossible. "
        "With hints on, the game warns you before a move that would make a request impossible.",

        "Si la petición de algún alcalde ya no se puede cumplir, perdéis al instante.\n\n"
        "Con 24 cartas jugadas y las 6 peticiones cumplidas: ¡victoria!\n\n"
        "Sellos: estrella = pendiente, verde = cumplida ahora, rojo = imposible. "
        "Con las pistas activadas, el juego avisa antes de una jugada que haría imposible una petición.",
    },
};

const char *TR(int id)
{
    const char *s;
    if (id < 0 || id >= NUM_STRINGS)
        return "?";
    s = strings[id][g_lang];
    return s ? s : strings[id][LANG_PT];
}
