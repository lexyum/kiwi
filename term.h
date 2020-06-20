#ifndef TEMU_TERM_H
#define TEMU_TERM_H

typedef char glyph_t;

/* memory handling */
void term_resize(int, int);
void term_free();

/* basic editing */
void insert_char(char);
void delete_char(void);
void moveto(int, int);
void newline(void);
void linefeed(void);
void carreturn(void);
void htab(void);
void backspace(void);

/* redisplay */
void redraw(void);
void scrolldown(int);

#endif /* TEMU_TERM_H */
