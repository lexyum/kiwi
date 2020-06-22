#ifndef TEMU_PTY_H
#define TEMU_PTY_H

int pty_init(char *argc, char *argv[]);

void pty_read(void);

void pty_write(char *buf, size_t count);

void pty_resize(unsigned short rows,
		unsigned short cols,
		unsigned short xpixels,
		unsigned short ypixels);

#endif /* TEMU_PTY_H */
