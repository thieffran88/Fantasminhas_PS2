#ifndef FONT_H
#define FONT_H

typedef struct {
    int off;               /* deslocamento no bitmap (bytes) */
    unsigned char w, h;    /* tamanho do glifo */
    signed char bx, by;    /* posição relativa ao topo da linha */
    unsigned char adv;     /* avanço horizontal */
} Glyph;

typedef struct {
    int size, ascent, descent;
    const Glyph *glyphs;          /* códigos 32..255 */
    const unsigned char *bits;    /* alfa 4 bits, 2 px por byte */
} Font;

extern const Font font_small, font_body, font_big, font_title;

#define FONT_FIRST 32
#define FONT_LAST 255

#endif
