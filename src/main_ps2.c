/*
 * Prefeitos do Além - ponto de entrada do PlayStation 2.
 *
 * O jogo desenha cada quadro por software (gfx.c) num buffer de 640x448
 * na RAM do EE; aqui esse buffer é enviado ao GS como textura e esticado
 * para a tela inteira (NTSC 640x448 ou PAL 640x512). O controle é lido
 * com a libpad (módulos SIO2MAN/PADMAN da ROM do console).
 */
#include <kernel.h>
#include <tamtypes.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <libpad.h>
#include <audsrv.h>
#include <sbv_patches.h>
#include <malloc.h>
#include <string.h>
#include <time.h>

#include <dmaKit.h>
#include <gsKit.h>

#include "audio.h"
#include "gfx.h"
#include "ui.h"

/* módulo IRX do audsrv embutido no ELF (gerado com bin2c no Makefile) */
extern unsigned char audsrv_irx[];
extern unsigned int size_audsrv_irx;
extern void *_gp;

static char pad_buf[256] __attribute__((aligned(64)));
static int pad_ok;

static void reset_iop(void)
{
    SifInitRpc(0);
    while (!SifIopReset("", 0)) {
    }
    while (!SifIopSync()) {
    }
    SifInitRpc(0);
}

/* ------------------------------------------------------------------ */
/* Som: uma thread de prioridade maior que a do jogo mixa e envia blocos ao
 * audsrv. audsrv_wait_audio() bloqueia até haver espaço, então a thread do
 * jogo segue rodando e o som não engasga quando a tela demora a desenhar. */

#define AUDIO_CHUNK 512
static short audio_buf[AUDIO_CHUNK] __attribute__((aligned(64)));
static unsigned char audio_stack[0x8000] __attribute__((aligned(16)));
static int audio_ok;

static void audio_thread(void *arg)
{
    (void)arg;
    while (1) {
        audio_render(audio_buf, AUDIO_CHUNK);
        audsrv_wait_audio(sizeof audio_buf);
        audsrv_play_audio((char *)audio_buf, sizeof audio_buf);
    }
}

static void init_audio(void)
{
    struct audsrv_fmt_t fmt;
    ee_thread_t th;
    int ret, tid;

    if (SifLoadModule("rom0:LIBSD", 0, NULL) < 0)
        return;
    if (SifExecModuleBuffer(audsrv_irx, size_audsrv_irx, 0, NULL, &ret) < 0 || ret < 0)
        return;
    if (audsrv_init() != 0)
        return;
    fmt.freq = AUDIO_RATE;
    fmt.bits = 16;
    fmt.channels = 1;
    if (audsrv_set_format(&fmt) != 0)
        return;
    audsrv_set_volume(MAX_VOLUME);

    /* a thread do jogo fica com prioridade menor que a do som */
    ChangeThreadPriority(GetThreadId(), 0x40);
    memset(&th, 0, sizeof th);
    th.func = (void *)audio_thread;
    th.stack = audio_stack;
    th.stack_size = sizeof audio_stack;
    th.gp_reg = &_gp;
    th.initial_priority = 0x30;
    tid = CreateThread(&th);
    if (tid < 0)
        return;
    StartThread(tid, NULL);
    audio_ok = 1;
}

static void init_pad(void)
{
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    padInit(0);
    pad_ok = padPortOpen(0, 0, pad_buf) != 0;
}

/* Lê o controle 1 e devolve os botões recém-apertados (com auto-repetição
 * no direcional, para andar rápido pelos menus segurando o botão). */
static unsigned int read_pad(void)
{
    static unsigned int prev;
    static int hold;
    struct padButtonStatus st;
    unsigned int cur = 0, pressed;
    int state;

    if (!pad_ok)
        return 0;
    state = padGetState(0, 0);
    if ((state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) && padRead(0, 0, &st) != 0) {
        unsigned int d = 0xffff ^ st.btns;
        if (d & PAD_UP) cur |= B_UP;
        if (d & PAD_DOWN) cur |= B_DOWN;
        if (d & PAD_LEFT) cur |= B_LEFT;
        if (d & PAD_RIGHT) cur |= B_RIGHT;
        if (d & PAD_CROSS) cur |= B_CROSS;
        if (d & PAD_CIRCLE) cur |= B_CIRCLE;
        if (d & PAD_TRIANGLE) cur |= B_TRIANGLE;
        if (d & PAD_SQUARE) cur |= B_SQUARE;
        if (d & PAD_START) cur |= B_START;
        if (d & PAD_SELECT) cur |= B_SELECT;
        if (d & PAD_L1) cur |= B_L1;
        if (d & PAD_R1) cur |= B_R1;
    }
    pressed = cur & ~prev;
    if (cur & (B_UP | B_DOWN | B_LEFT | B_RIGHT)) {
        if (++hold > 20 && (hold % 6) == 0)
            pressed |= cur & (B_UP | B_DOWN | B_LEFT | B_RIGHT);
    } else {
        hold = 0;
    }
    prev = cur;
    return pressed;
}

int main(int argc, char *argv[])
{
    GSGLOBAL *gs;
    GSTEXTURE tex;
    (void)argc;
    (void)argv;

    reset_iop();
    sbv_patch_enable_lmb(); /* permite carregar IRX a partir da memória */
    init_pad();

    gs = gsKit_init_global();
    gs->PSM = GS_PSM_CT24;
    gs->PSMZ = GS_PSMZ_16S;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->DoubleBuffering = GS_SETTING_ON;
    gs->PrimAlphaEnable = GS_SETTING_OFF;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8,
                1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);

    memset(&tex, 0, sizeof tex);
    tex.Width = SCR_W;
    tex.Height = SCR_H;
    tex.PSM = GS_PSM_CT32;
    tex.Filter = GS_FILTER_LINEAR;
    tex.Mem = memalign(128, gsKit_texture_size_ee(SCR_W, SCR_H, GS_PSM_CT32));

    ui_init((unsigned int)clock() ^ 0x5EED1234u); /* também inicia o mixer */
    init_audio();

    while (1) {
        ui_update(read_pad());
        ui_draw_to((unsigned int *)tex.Mem);
        FlushCache(0); /* garante que a textura saiu do cache antes do DMA */

        gsKit_TexManager_invalidate(gs, &tex);
        gsKit_clear(gs, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00));
        gsKit_TexManager_bind(gs, &tex);
        gsKit_prim_sprite_texture(gs, &tex, 0.0f, 0.0f, 0.0f, 0.0f, (float)gs->Width, (float)gs->Height,
                                  (float)SCR_W, (float)SCR_H, 1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
        gsKit_TexManager_nextFrame(gs);
    }
    return 0;
}
