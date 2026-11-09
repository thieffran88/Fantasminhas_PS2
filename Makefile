# Prefeitos do Além - build para PlayStation 2 (ps2sdk + gsKit)
#
# Requer o toolchain do ps2dev (variáveis PS2SDK e GSKIT definidas).
# O jeito mais fácil é usar o container oficial:
#   docker run --rm -v "$PWD":/src -w /src ps2dev/ps2dev:latest sh -c "apk add make && make"
#
#   make        -> prefeitos_do_alem.elf
#   make iso    -> prefeitos_do_alem.iso (precisa de xorriso)

EE_BIN = prefeitos_do_alem.elf
EE_OBJS = src/main_ps2.o src/ui.o src/game.o src/art.o src/gfx.o src/font_data.o src/sprites.o src/sprites_data.o

EE_INCS += -I$(GSKIT)/include -Isrc
EE_LDFLAGS += -L$(GSKIT)/lib
EE_LIBS += -lgskit -ldmakit -lpad -lm
EE_CFLAGS += -O2 -Wall

all: $(EE_BIN)

iso: $(EE_BIN)
	rm -rf build/iso
	mkdir -p build/iso
	cp $(EE_BIN) build/iso/PREFEITO.ELF
	printf 'BOOT2 = cdrom0:\\PREFEITO.ELF;1\r\nVER = 1.00\r\nVMODE = NTSC\r\n' > build/iso/SYSTEM.CNF
	xorriso -as mkisofs -iso-level 1 -V PREFEITOS_DO_ALEM -o prefeitos_do_alem.iso build/iso

clean:
	rm -f $(EE_BIN) $(EE_OBJS) prefeitos_do_alem.iso
	rm -rf build/iso

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
