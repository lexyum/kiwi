#ifndef TEMU_TERM_H
#define TEMU_TERM_H

typedef char glyph_t;

/* memory handling */
void term_resize(int rows, int cols);
void term_free();

#endif /* TEMU_TERM_H */
