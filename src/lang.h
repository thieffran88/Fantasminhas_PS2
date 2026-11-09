#ifndef LANG_H
#define LANG_H

/* Idiomas do jogo: português (principal), inglês e espanhol. */
enum { LANG_PT, LANG_EN, LANG_ES, NUM_LANGS };

extern int g_lang;

enum {
    S_LANG_NAME,
    S_PRESS_START,
    S_MENU_PLAY,
    S_MENU_PLAYERS,
    S_MENU_HINTS,
    S_HINTS_ON,
    S_HINTS_OFF,
    S_MENU_LANGUAGE,
    S_MENU_MUSIC,
    S_MENU_SFX,
    S_YES,
    S_NO,
    S_MENU_HOWTO,
    S_GOOD_LUCK_STARTS,   /* %s */
    S_GOOD_LUCK_SOLO,
    S_TOWN_FOUNDED,       /* %d */
    S_TURN_OF,            /* %s */
    S_FOUND_IN_ORDER,
    S_TOWN_FULL_TOAST,
    S_ST_MET,
    S_ST_BROKEN,
    S_ST_OPEN,
    S_NEW_MAYOR,
    S_NEW_TOWN,
    S_SLOT,
    S_CARDS_PLAYED,       /* %d %d */
    S_TURN_HEADER,        /* %s %d %d */
    S_SOLO,
    S_DECK_N,             /* %d */
    S_TOURIST,
    S_PLAYER_DRAW,        /* %s */
    S_DRAW,
    S_TOURIST_REQ,
    S_DECK_INFO,          /* %d */
    S_BTN_BUY,
    S_BTN_RULES,
    S_BTN_MENU,
    S_IN_HAND,
    S_FOUND_TOWN,         /* %d */
    S_TOWN_CARDS,         /* %d %d */
    S_MAYOR_REQ,          /* %s */
    S_TOWN_COMPLETE,
    S_SLOT_CLOSED,
    S_FOUND_ORDER_INFO,
    S_WARN_IMPOSSIBLE,    /* %d */
    S_HINT_SAFE,
    S_BTN_PLAY_HERE,
    S_REQ_LABEL,          /* %s */
    S_RULES_NAV,
    S_BTN_BACK,
    S_PAUSE,
    S_PAUSE_CONTINUE,
    S_PAUSE_RESTART,
    S_PAUSE_TITLE,
    S_SHOW_RESULT,
    S_VICTORY,
    S_DEFEAT,
    S_WIN_TEXT,
    S_FAIL_TEXT,          /* %d */
    S_PLAY_AGAIN,
    S_BTN_TITLE,
    S_SEE_TOWNS,
    S_NAME_MAYOR,
    S_RULE_T0, S_RULE_T1, S_RULE_T2, S_RULE_T3,
    S_RULE_B0, S_RULE_B1, S_RULE_B2, S_RULE_B3,
    NUM_STRINGS
};

const char *TR(int id);

#endif
