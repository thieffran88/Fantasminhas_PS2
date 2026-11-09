/*
 * Sintetizador e sequenciador do Prefeitos do Além.
 *
 * Instrumentos: theremin (seno com vibrato e portamento, a "voz" dos
 * fantasmas), órgão, contrabaixo pizzicato, caixinha de música, sino,
 * pad suave, onda quadrada e ruído. As músicas são escritas como texto
 * ("a5:2 c6:1 -:3" = nota:duração em linhas) e os efeitos como pequenos
 * segmentos com deslizamento de altura.
 */
#include "audio.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Instrumentos */

enum { W_SINE, W_ORGAN, W_TRI, W_MUSICBOX, W_BELL, W_NOISE, W_SQUARE };
enum { I_THEREMIN, I_ORGAN, I_PIZZ, I_MUSICBOX, I_BELL, I_PAD, I_NOISE, I_SQUARE, I_BLIP, NUM_INST };

typedef struct {
    int wave;
    float attack, decay, sustain, release; /* segundos / nível */
    float vib_depth, vib_rate, glide;      /* vibrato (fração de frequência), portamento (s) */
} Inst;

static const Inst insts[NUM_INST] = {
    [I_THEREMIN] = { W_SINE, 0.06f, 0.0f, 1.0f, 0.18f, 0.018f, 5.5f, 0.07f },
    [I_ORGAN] = { W_ORGAN, 0.01f, 0.25f, 0.55f, 0.08f, 0.0f, 0.0f, 0.0f },
    [I_PIZZ] = { W_TRI, 0.003f, 0.30f, 0.0f, 0.05f, 0.0f, 0.0f, 0.0f },
    [I_MUSICBOX] = { W_MUSICBOX, 0.002f, 0.9f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f },
    [I_BELL] = { W_BELL, 0.002f, 1.3f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f },
    [I_PAD] = { W_TRI, 0.35f, 0.0f, 1.0f, 0.45f, 0.004f, 4.0f, 0.0f },
    [I_NOISE] = { W_NOISE, 0.005f, 0.12f, 0.0f, 0.05f, 0.0f, 0.0f, 0.0f },
    [I_SQUARE] = { W_SQUARE, 0.003f, 0.1f, 0.4f, 0.05f, 0.0f, 0.0f, 0.0f },
    [I_BLIP] = { W_SINE, 0.002f, 0.06f, 0.0f, 0.03f, 0.0f, 0.0f, 0.0f },
};

#define TAB_BITS 10
#define TAB_SIZE (1 << TAB_BITS)
static short sine_tab[TAB_SIZE];

enum { ENV_OFF, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

typedef struct {
    const Inst *in;
    int stage;
    float env, vol;
    float step, target;          /* passo de fase atual e alvo (portamento) */
    float glide_k;               /* coeficiente de portamento por amostra */
    float sweep;                 /* multiplicador por amostra (efeitos) */
    unsigned int phase, phase2;
    float vib_phase, vib_amt;
    int gate;                    /* amostras até soltar a nota (-1 = segura) */
    unsigned int noise, noise_val;
} Voice;

/* ------------------------------------------------------------------ */
/* Músicas */

#define MAX_CH 6

typedef struct {
    int inst;
    float vol;
    const char *notes;
} ChanDef;

typedef struct {
    float row_sec;  /* duração de uma linha (colcheia) */
    int loop;
    int nch;
    ChanDef ch[MAX_CH];
} SongDef;

/* acompanhamentos gerados a partir das cifras em audio_init() */
static char title_bass[512], title_org[3][512];
static char game_bass[512], game_pad[3][512];

/* Valsa assombrada em lá menor (3/4, 6 colcheias por compasso) */
static const char title_melody[] =
    "e5:2 a5:2 c6:2  b5:3 a5:1 g#5:2  a5:4 f5:2  e5:6 "
    "d5:2 f5:2 a5:2  c6:3 b5:1 a5:2  g#5:2 b5:2 e6:2  d6:4 b5:2 "
    "e6:2 d6:2 c6:2  a5:4 c6:2  b5:2 a5:2 f5:2  g#5:4 e5:2 "
    "a4:2 c5:2 e5:2  f5:3 e5:1 d5:2  e5:2 g#5:2 b5:2  a5:6 ";
/* compasso a compasso: baixo e 3 notas do acorde */
static const char *const title_chords[16][4] = {
    { "a2", "a3", "c4", "e4" }, { "a2", "a3", "c4", "e4" }, { "f2", "a3", "c4", "f4" }, { "e2", "g#3", "b3", "e4" },
    { "d2", "a3", "d4", "f4" }, { "a2", "a3", "c4", "e4" }, { "e2", "g#3", "b3", "e4" }, { "e2", "g#3", "b3", "d4" },
    { "a2", "a3", "c4", "e4" }, { "f2", "a3", "c4", "f4" }, { "d2", "a3", "d4", "f4" }, { "e2", "g#3", "b3", "e4" },
    { "a2", "a3", "c4", "e4" }, { "f2", "a3", "c4", "f4" }, { "e2", "g#3", "b3", "e4" }, { "a2", "a3", "c4", "e4" },
};

/* Caixinha de música em ré menor (4/4, 8 colcheias por compasso) */
static const char game_melody[] =
    "a5:1 d6:1 f6:1 e6:1 d6:2 a5:2  a#5:1 d6:1 f6:1 d6:1 c6:2 a#5:2 "
    "g5:1 a#5:1 d6:1 c6:1 a#5:2 a5:2  c#6:2 e6:2 a5:4 "
    "f6:2 e6:1 d6:1 a5:2 d6:2  d6:2 c6:1 a#5:1 f5:2 a#5:2 "
    "e6:2 d6:1 c6:1 g5:2 c6:2  c#6:4 e6:2 -:2 "
    "d6:3 -:1 a5:2 f5:2  a#5:3 -:1 f5:2 d5:2  g5:2 a#5:2 d6:2 c6:2  a5:4 c#6:4 "
    "d6:2 f6:2 e6:2 d6:2  c6:2 a#5:2 a5:2 g5:2  a5:2 c#6:2 e6:2 g6:2  f6:4 d6:4 ";
static const char *const game_chords[16][5] = {
    /* baixo1 baixo2 acorde */
    { "d2", "a2", "a3", "d4", "f4" }, { "a#1", "f2", "a#3", "d4", "f4" }, { "g2", "d3", "a#3", "d4", "g4" },
    { "a2", "e2", "a3", "c#4", "e4" }, { "d2", "a2", "a3", "d4", "f4" }, { "a#1", "f2", "a#3", "d4", "f4" },
    { "c2", "g2", "g3", "c4", "e4" }, { "a2", "e2", "a3", "c#4", "e4" },
    { "d2", "a2", "a3", "d4", "f4" }, { "a#1", "f2", "a#3", "d4", "f4" }, { "g2", "d3", "a#3", "d4", "g4" },
    { "a2", "e2", "a3", "c#4", "e4" }, { "d2", "a2", "a3", "d4", "f4" }, { "c2", "g2", "g3", "c4", "e4" },
    { "a2", "e2", "a3", "c#4", "g4" }, { "d2", "a2", "a3", "d4", "f4" },
};

static SongDef songs[NUM_MUSIC];

static void build_songs(void)
{
    int b, k;
    char *p;
    /* valsa: baixo no 1º tempo, órgão nos tempos 2 e 3 */
    p = title_bass;
    for (b = 0; b < 16; b++)
        p += sprintf(p, "%s:2 -:4 ", title_chords[b][0]);
    for (k = 0; k < 3; k++) {
        p = title_org[k];
        for (b = 0; b < 16; b++)
            p += sprintf(p, "-:2 %s:2 %s:2 ", title_chords[b][k + 1], title_chords[b][k + 1]);
    }
    /* caixinha: baixo em mínimas e pad sustentado */
    p = game_bass;
    for (b = 0; b < 16; b++)
        p += sprintf(p, "%s:4 %s:4 ", game_chords[b][0], game_chords[b][1]);
    for (k = 0; k < 3; k++) {
        p = game_pad[k];
        for (b = 0; b < 16; b++)
            p += sprintf(p, "%s:8 ", game_chords[b][k + 2]);
    }

    songs[MUS_TITLE] = (SongDef){ 60.0f / 112 / 2, 1, 5,
                                  { { I_THEREMIN, 0.30f, title_melody },
                                    { I_PIZZ, 0.42f, title_bass },
                                    { I_ORGAN, 0.075f, title_org[0] },
                                    { I_ORGAN, 0.075f, title_org[1] },
                                    { I_ORGAN, 0.075f, title_org[2] } } };
    songs[MUS_GAME] = (SongDef){ 60.0f / 88 / 2, 1, 5,
                                 { { I_MUSICBOX, 0.30f, game_melody },
                                   { I_PIZZ, 0.30f, game_bass },
                                   { I_PAD, 0.055f, game_pad[0] },
                                   { I_PAD, 0.055f, game_pad[1] },
                                   { I_PAD, 0.055f, game_pad[2] } } };
    songs[MUS_WIN] = (SongDef){ 60.0f / 150 / 2, 0, 5,
                                { { I_BELL, 0.26f, "a5:1 c#6:1 e6:1 a6:3 e6:1 a6:9 " },
                                  { I_THEREMIN, 0.22f, "-:2 e5:2 a5:2 c#6:3 b5:1 a5:6 " },
                                  { I_PIZZ, 0.40f, "a2:2 e2:2 a2:2 e2:2 a2:8 " },
                                  { I_ORGAN, 0.08f, "-:4 c#4:12 " },
                                  { I_ORGAN, 0.08f, "-:4 e4:12 " } } };
    songs[MUS_LOSE] = (SongDef){ 60.0f / 70 / 2, 0, 4,
                                 { { I_THEREMIN, 0.30f, "e5:2 d#5:2 d5:2 c#5:3 c5:5 " },
                                   { I_PIZZ, 0.40f, "a2:4 a2:4 a1:6 " },
                                   { I_ORGAN, 0.07f, "c4:6 c4:8 " },
                                   { I_ORGAN, 0.07f, "e4:6 d#4:8 " } } };
}

/* ------------------------------------------------------------------ */
/* Efeitos: segmentos (instrumento, nota inicial, nota final, ms, volume, atraso ms) */

typedef struct {
    signed char inst, n0, n1;
    short ms;
    float vol;
    short delay;
} Seg;

#define MAX_SEGS 6
static const Seg sfx_defs[NUM_SFX][MAX_SEGS] = {
    [SFX_MOVE] = { { I_BLIP, 81, 83, 45, 0.22f, 0 } },
    [SFX_CONFIRM] = { { I_BELL, 84, 84, 160, 0.20f, 0 }, { I_BELL, 91, 91, 260, 0.18f, 70 } },
    [SFX_BACK] = { { I_BLIP, 79, 72, 110, 0.24f, 0 } },
    [SFX_DRAW] = { { I_NOISE, 100, 80, 150, 0.16f, 0 }, { I_BLIP, 74, 86, 90, 0.10f, 20 } },
    [SFX_PLACE] = { { I_PIZZ, 45, 40, 160, 0.45f, 0 }, { I_NOISE, 90, 70, 40, 0.14f, 0 } },
    [SFX_MAYOR] = { { I_THEREMIN, 64, 76, 520, 0.24f, 0 }, { I_THEREMIN, 76, 69, 420, 0.22f, 520 },
                    { I_BELL, 88, 88, 500, 0.12f, 120 } },
    [SFX_MET] = { { I_MUSICBOX, 88, 88, 220, 0.18f, 0 }, { I_MUSICBOX, 91, 91, 220, 0.18f, 70 },
                  { I_MUSICBOX, 95, 95, 320, 0.18f, 140 } },
    [SFX_WARN] = { { I_SQUARE, 52, 52, 120, 0.09f, 0 }, { I_SQUARE, 51, 51, 200, 0.09f, 150 } },
    [SFX_ERROR] = { { I_SQUARE, 43, 41, 180, 0.12f, 0 } },
    [SFX_PAGE] = { { I_NOISE, 90, 100, 90, 0.12f, 0 } },
    [SFX_START] = { { I_THEREMIN, 57, 69, 380, 0.26f, 0 }, { I_THEREMIN, 69, 81, 450, 0.18f, 300 },
                    { I_NOISE, 80, 100, 400, 0.06f, 0 } },
    [SFX_PAUSE] = { { I_BELL, 76, 76, 200, 0.16f, 0 }, { I_BELL, 71, 71, 260, 0.14f, 90 } },
};

/* ------------------------------------------------------------------ */
/* Estado */

#define SFX_VOICES 6
#define MAX_PENDING 16

typedef struct {
    const char *pos;   /* próxima nota no texto */
    int left;          /* amostras até a próxima nota */
} ChanState;

static Voice music_voice[MAX_CH], sfx_voice[SFX_VOICES];
static ChanState chans[MAX_CH];
static int cur_song = MUS_NONE, row_samples;
static int next_sfx_voice;
static struct {
    Seg seg;
    int wait;
} pending[MAX_PENDING];

/* fila de pedidos (escrita pela thread do jogo, lida pela thread de áudio) */
#define QUEUE 32
static volatile int q_cmd[QUEUE];
static volatile int q_head, q_tail;
static volatile int music_on = 1, sfx_on = 1;
static float music_gain = 1.0f;

/* ------------------------------------------------------------------ */

static float midi_step(float note)
{
    float f = 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);
    return f / AUDIO_RATE * 4294967296.0f;
}

static void voice_on(Voice *v, int inst, float note, float vol, int gate)
{
    const Inst *in = &insts[inst];
    float st = midi_step(note);
    int legato = (v->in == in && v->stage != ENV_OFF && in->glide > 0);
    v->in = in;
    v->target = st;
    if (!legato) {
        v->step = st;
        v->phase = v->phase2 = 0;
        v->env = 0;
        v->vib_amt = 0;
    }
    v->glide_k = in->glide > 0 ? 1.0f / (in->glide * AUDIO_RATE) : 1.0f;
    v->sweep = 1.0f;
    v->vol = vol;
    v->gate = gate;
    v->stage = ENV_ATTACK;
    if (!v->noise)
        v->noise = 0xACE1u;
}

static void voice_off(Voice *v)
{
    if (v->stage != ENV_OFF)
        v->stage = ENV_RELEASE;
}

static inline int tab(unsigned int ph) { return sine_tab[ph >> (32 - TAB_BITS)]; }

static float voice_sample(Voice *v)
{
    const Inst *in = v->in;
    float s, step;
    if (v->stage == ENV_OFF)
        return 0;
    /* envelope */
    switch (v->stage) {
    case ENV_ATTACK:
        v->env += in->attack > 0 ? 1.0f / (in->attack * AUDIO_RATE) : 1.0f;
        if (v->env >= 1.0f) {
            v->env = 1.0f;
            v->stage = ENV_DECAY;
        }
        break;
    case ENV_DECAY:
        if (in->decay > 0)
            v->env -= (1.0f - in->sustain) / (in->decay * AUDIO_RATE);
        if (v->env <= in->sustain || in->decay <= 0) {
            v->env = in->sustain;
            v->stage = ENV_SUSTAIN;
            if (in->sustain <= 0.0f) {
                v->stage = ENV_OFF;
                return 0;
            }
        }
        break;
    case ENV_RELEASE:
        v->env -= 1.0f / (in->release * AUDIO_RATE);
        if (v->env <= 0) {
            v->env = 0;
            v->stage = ENV_OFF;
            return 0;
        }
        break;
    default: break;
    }
    if (v->gate > 0 && --v->gate == 0)
        voice_off(v);
    /* altura: portamento, varredura e vibrato */
    v->step += (v->target - v->step) * v->glide_k;
    v->target *= v->sweep;
    if (v->sweep != 1.0f)
        v->step *= v->sweep;
    step = v->step;
    if (in->vib_depth > 0) {
        if (v->vib_amt < 1.0f)
            v->vib_amt += 1.0f / (0.25f * AUDIO_RATE);
        v->vib_phase += in->vib_rate / AUDIO_RATE;
        if (v->vib_phase >= 1.0f)
            v->vib_phase -= 1.0f;
        step *= 1.0f + in->vib_depth * v->vib_amt * sine_tab[(int)(v->vib_phase * TAB_SIZE) & (TAB_SIZE - 1)] / 32768.0f;
    }
    v->phase += (unsigned int)step;
    switch (in->wave) {
    case W_ORGAN:
        s = tab(v->phase) * 0.6f + tab(v->phase * 2) * 0.28f + tab(v->phase * 3) * 0.12f;
        break;
    case W_TRI: {
        int p = (int)(v->phase >> 16);
        s = (float)((p < 32768 ? p : 65535 - p) * 2 - 32768);
        break;
    }
    case W_MUSICBOX:
        s = tab(v->phase) * 0.75f + tab(v->phase * 4) * 0.25f * v->env;
        break;
    case W_BELL:
        v->phase2 += (unsigned int)(step * 2.76f);
        s = tab(v->phase) * 0.65f + tab(v->phase2) * 0.35f * v->env;
        break;
    case W_NOISE: {
        unsigned int old = v->phase - (unsigned int)step;
        if (v->phase < old) { /* deu a volta: novo valor de ruído */
            unsigned int x = v->noise;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            v->noise = x;
            v->noise_val = x;
        }
        s = (float)(short)(v->noise_val & 0xFFFF);
        break;
    }
    case W_SQUARE:
        s = (v->phase & 0x80000000u) ? 14000.0f : -14000.0f;
        break;
    default:
        s = (float)tab(v->phase);
        break;
    }
    return s * v->env * v->vol;
}

/* ------------------------------------------------------------------ */
/* Sequenciador */

static int parse_note(const char **ps, int *len)
{
    static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 }; /* a b c d e f g */
    const char *s = *ps;
    int note = -1;
    while (*s == ' ')
        s++;
    if (!*s) {
        *ps = s;
        return -2; /* fim */
    }
    if (*s == '-') {
        s++;
    } else {
        int n = base[*s - 'a'];
        s++;
        if (*s == '#') {
            n++;
            s++;
        } else if (*s == 'b' && s[1] >= '0' && s[1] <= '9') {
            n--;
            s++;
        }
        note = n + 12 * (*s - '0' + 1);
        s++;
    }
    *len = 1;
    if (*s == ':') {
        s++;
        *len = 0;
        while (*s >= '0' && *s <= '9')
            *len = *len * 10 + (*s++ - '0');
    }
    *ps = s;
    return note;
}

static void start_song(int song)
{
    int c;
    for (c = 0; c < MAX_CH; c++) {
        voice_off(&music_voice[c]);
        chans[c].pos = 0;
        chans[c].left = 0;
    }
    cur_song = song;
    if (song == MUS_NONE)
        return;
    row_samples = (int)(songs[song].row_sec * AUDIO_RATE);
    for (c = 0; c < songs[song].nch; c++)
        chans[c].pos = songs[song].ch[c].notes;
}

static void seq_tick(void)
{
    const SongDef *sd;
    int c, ended = 1;
    if (cur_song == MUS_NONE)
        return;
    sd = &songs[cur_song];
    for (c = 0; c < sd->nch; c++) {
        ChanState *ch = &chans[c];
        if (!ch->pos)
            continue;
        if (ch->left > 0) {
            ch->left--;
            ended = 0;
            continue;
        }
        {
            int len, note = parse_note(&ch->pos, &len);
            if (note == -2) {
                ch->pos = 0;
                continue;
            }
            ended = 0;
            ch->left = len * row_samples - 1;
            if (note >= 0) {
                /* teatro de fantasmas: theremin liga as notas, os outros soltam antes */
                int gate = sd->ch[c].inst == I_THEREMIN ? len * row_samples - row_samples / 8
                                                        : len * row_samples * 7 / 8;
                voice_on(&music_voice[c], sd->ch[c].inst, (float)note, sd->ch[c].vol, gate);
            }
        }
    }
    if (ended) {
        if (sd->loop)
            start_song(cur_song);
        else
            cur_song = MUS_NONE;
    }
}

static void start_seg(const Seg *sg)
{
    Voice *v = &sfx_voice[next_sfx_voice];
    int n = (int)(sg->ms * (AUDIO_RATE / 1000.0f));
    next_sfx_voice = (next_sfx_voice + 1) % SFX_VOICES;
    v->in = 0; /* força ataque novo */
    voice_on(v, sg->inst, (float)sg->n0, sg->vol, n);
    if (sg->n1 != sg->n0 && n > 0)
        v->sweep = powf(2.0f, (float)(sg->n1 - sg->n0) / 12.0f / (float)n);
    v->glide_k = 1.0f;
}

static void handle_queue(void)
{
    while (q_tail != q_head) {
        int cmd = q_cmd[q_tail];
        q_tail = (q_tail + 1) % QUEUE;
        if (cmd >= 1000) {
            int song = cmd - 1000;
            if (song != cur_song)
                start_song(song);
        } else if (cmd >= 0 && cmd < NUM_SFX) {
            int k, j;
            for (k = 0; k < MAX_SEGS; k++) {
                const Seg *sg = &sfx_defs[cmd][k];
                if (sg->ms <= 0)
                    break;
                if (sg->delay <= 0) {
                    start_seg(sg);
                    continue;
                }
                for (j = 0; j < MAX_PENDING; j++) {
                    if (pending[j].wait <= 0) {
                        pending[j].seg = *sg;
                        pending[j].wait = (int)(sg->delay * (AUDIO_RATE / 1000.0f));
                        break;
                    }
                }
            }
        }
    }
}

static void push(int cmd)
{
    int next = (q_head + 1) % QUEUE;
    if (next == q_tail)
        return; /* fila cheia: ignora */
    q_cmd[q_head] = cmd;
    q_head = next;
}

/* ------------------------------------------------------------------ */

void audio_init(void)
{
    int i;
    for (i = 0; i < TAB_SIZE; i++)
        sine_tab[i] = (short)(sinf(6.2831853f * i / TAB_SIZE) * 32767.0f);
    build_songs();
    memset(music_voice, 0, sizeof music_voice);
    memset(sfx_voice, 0, sizeof sfx_voice);
    cur_song = MUS_NONE;
}

void audio_music(int song) { push(1000 + song); }
void audio_sfx(int id)
{
    if (sfx_on)
        push(id);
}
void audio_set_enabled(int m, int s)
{
    music_on = m;
    sfx_on = s;
}

void audio_render(short *out, int n)
{
    int i, c, j;
    handle_queue();
    for (i = 0; i < n; i++) {
        float mix = 0, mus = 0;
        seq_tick();
        for (j = 0; j < MAX_PENDING; j++) {
            if (pending[j].wait > 0 && --pending[j].wait == 0)
                start_seg(&pending[j].seg);
        }
        for (c = 0; c < MAX_CH; c++)
            mus += voice_sample(&music_voice[c]);
        /* liga/desliga a música suavemente */
        music_gain += ((music_on ? 1.0f : 0.0f) - music_gain) * 0.0005f;
        mix = mus * music_gain;
        for (c = 0; c < SFX_VOICES; c++)
            mix += voice_sample(&sfx_voice[c]);
        if (mix > 32767.0f)
            mix = 32767.0f;
        if (mix < -32768.0f)
            mix = -32768.0f;
        out[i] = (short)mix;
    }
}
