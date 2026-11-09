#ifndef AUDIO_H
#define AUDIO_H

/*
 * Música e efeitos sonoros sintetizados em tempo real (nenhum arquivo de áudio).
 * O mixer gera PCM 16 bits mono; a plataforma só precisa chamar audio_render().
 * audio_music()/audio_sfx() podem ser chamados de outra thread (fila sem trava).
 */

#define AUDIO_RATE 22050

enum {
    MUS_NONE,
    MUS_TITLE,    /* valsa assombrada (lá menor) */
    MUS_GAME,     /* caixinha de música (ré menor) */
    MUS_WIN,      /* fanfarra de vitória (toca uma vez) */
    MUS_LOSE,     /* lamento de derrota (toca uma vez) */
    NUM_MUSIC
};

enum {
    SFX_MOVE,     /* cursor */
    SFX_CONFIRM,
    SFX_BACK,
    SFX_DRAW,     /* comprar carta */
    SFX_PLACE,    /* jogar carta */
    SFX_MAYOR,    /* novo prefeito: "uuuuu" de fantasma */
    SFX_MET,      /* pedido cumprido */
    SFX_WARN,     /* jogada perigosa */
    SFX_ERROR,
    SFX_PAGE,
    SFX_START,    /* sair da abertura */
    SFX_PAUSE,
    NUM_SFX
};

void audio_init(void);
void audio_render(short *out, int nsamples);
void audio_music(int song);           /* troca a música (não reinicia se já toca) */
void audio_sfx(int id);
void audio_set_enabled(int music_on, int sfx_on);

#endif
