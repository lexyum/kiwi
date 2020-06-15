#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef char glyph;

static struct {
	int rows;
	int cols;
	glyph **screen;
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
	// correct number of rows
	for (int i = rows; i < term.rows; ++i) 
		free(term.screen[i]);

	term.screen = realloc_test(term.screen, rows*sizeof(*term.screen));

	// resize each existing row, zeroing if necessary
	for (int i = 0; i < term.rows && i < rows; ++i) {
		term.screen[i] = realloc_test(term.screen[i], cols * sizeof(*term.screen[i]));
		if (cols > term.cols)
			memset(term.screen[i], 0, (cols - term.cols) * sizeof(*term.screen[i]));
	}

	// allocate new rows
	for (int i = term.rows; i < rows; ++i)
		term.screen[i] = calloc_test(cols, sizeof(*term.screen[i]));

	// all done
	term.cols = cols;
	term.rows = rows;
}

void term_free(void)
{
	for (int i = 0; i < term.rows; ++i)
		free(term.screen[i]);

	free(term.screen);
}
