#ifndef TEMU_TERM_H
#define TEMU_TERM_H

typedef char glyph_t;

/* memory handling */
void term_resize(int rows, int cols);
void term_free();

/* basic editing */
void insert_char(char c);
void delete_char(void);
void moveto(int, int);
void newline(void);
void linefeed(void);
void carreturn(void);
void htab(void);

#endif /* TEMU_TERM_H */
