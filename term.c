#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "term.h"
/*
 * TODO: Currently we store the physical screen and alternate screen together
n *       and update both when the window size changes. Maybe better to 
 *       initialise/resize each screen only when they become active,
 *       not bother resizing an inactive screen?
 */
static struct {
	int rows;
	int cols;
	int xcur;
	int ycur;
	glyph_t **screen;
	glyph_t **alt;
} term;

/* Return-tested dynamic memory allocation. Use library prototypes */
static void *malloc_test(size_t n)
{
	void *tmp = malloc(n);
	if (!tmp) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	return tmp;
}

static void *calloc_test(size_t nmemb, size_t n)
{
	void *tmp = calloc(nmemb, n);
	if (!tmp) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	return tmp;
}
	
static void *realloc_test(void *p, size_t n)
{
	void *tmp = realloc(p, n);
	if (!tmp) {
		perror("realloc");
		exit(EXIT_FAILURE);
	}
	p = tmp;
	return p;
}

void term_resize(int rows, int cols)
{
	int i;
	// shift screen to adjust cursor vertical position
	for (i = 0; i <= term.ycur - cols; ++i) {
		free(term.screen[i]);
		free(term.alt[i]);
	}
	if (i > 0) {
		memmove(term.screen, term.screen + i, rows * sizeof(*term.screen));
		memmove(term.alt, term.alt + i, rows * sizeof(*term.alt));
		term.ycur = cols - 1;
	}
	// correct number of rows
	for (i += rows; i < term.rows; ++i) {
		free(term.screen[i]);
		free(term.alt[i]);
	}

	term.screen = realloc_test(term.screen, rows * sizeof(*term.screen));
	term.alt    = realloc_test(term.alt, rows * sizeof(*term.alt));

	// Insert line breaks for cursor if needed by shifting screen 
	while (term.xcur >= cols) {
		glyph_t *saved = term.screen[0];
		
		memmove(term.screen, term.screen + 1, term.ycur * sizeof(*term.screen));
		term.screen[term.ycur] = saved;
		memcpy(term.screen[term.ycur], term.screen[term.ycur - 1] + cols,
		       (term.cols - cols) * sizeof(**term.screen));
		
		term.xcur -= cols;
		
		memset(term.screen[term.ycur] + term.xcur, ' ',
		       (term.cols - term.xcur) * sizeof(**term.screen));
	}
		
	// resize each existing row, zeroing if necessary
	for (int i = 0; i < term.rows && i < rows; ++i) {
		term.screen[i] = realloc_test(term.screen[i], cols * sizeof(*term.screen[i]));
		term.alt[i]    = realloc_test(term.alt[i], cols * sizeof(*term.alt[i]));
		if (cols > term.cols) {
			memset(term.screen[i], ' ', (cols - term.cols) * sizeof(*term.screen[i]));
			memset(term.alt[i], ' ', (cols - term.cols) * sizeof(*term.screen[i]));
		}

	}

	// allocate new rows
	for (int i = term.rows; i < rows; ++i) {
		term.screen[i] = calloc_test(cols, sizeof(*term.screen[i]));
		term.alt[i] = calloc_test(cols, sizeof(*term.screen[i]));
	}

	// all done
	term.cols = cols;
	term.rows = rows;
}

void term_free(void)
{
	for (int i = 0; i < term.rows; ++i) {
		free(term.screen[i]);
		free(term.alt[i]);
	}

	free(term.alt);
	free(term.screen);
}
