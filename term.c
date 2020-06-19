#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "term.h"

const int tabspaces = 8;

void xdraw_char(char, int, int);
void xdelete_char(int, int);

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
	glyph_t **screen; // current screen
	glyph_t **alt;    // alternate screen
	int *tabs;        // tab stops
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
	term.tabs   = realloc_test(term.tabs, cols * sizeof(*term.tabs));

	// Insert line breaks for cursor if needed by shifting screen 
	while (term.xcur >= cols) {
		glyph_t *saved = term.screen[0];
		
		memmove(term.screen, term.screen + 1, term.ycur * sizeof(*term.screen));
		term.screen[term.ycur] = saved;
		memcpy(term.screen[term.ycur], term.screen[term.ycur - 1] + cols,
		       (term.cols - cols) * sizeof(**term.screen));
		
		term.xcur -= cols;
		
		memset(term.screen[term.ycur] + term.xcur, 0,
		       (term.cols - term.xcur) * sizeof(**term.screen));
	}
		
	// resize each existing row, zeroing if necessary
	for (int i = 0; i < term.rows && i < rows; ++i) {
		term.screen[i] = realloc_test(term.screen[i], cols * sizeof(*term.screen[i]));
		term.alt[i]    = realloc_test(term.alt[i], cols * sizeof(*term.alt[i]));
		if (cols > term.cols) {
			memset(term.screen[i] + term.cols, 0, (cols - term.cols) * sizeof(*term.screen[i]));
			memset(term.alt[i] + term.cols, 0, (cols - term.cols) * sizeof(*term.screen[i]));
		}

	}

	// allocate new rows
	for (int i = term.rows; i < rows; ++i) {
		term.screen[i] = calloc_test(cols, sizeof(*term.screen[i]));
		term.alt[i] = calloc_test(cols, sizeof(*term.screen[i]));
	}
	
	if (cols > term.cols) {
		int *tstop = term.tabs + term.cols;
		memset(tstop, 0, (cols - term.cols) * sizeof(*term.tabs));
		/* extend tab stops if necessary */
		while (*tstop != 1 && tstop != term.tabs)
			--tstop;
		for (tstop += tabspaces; tstop < term.tabs + cols;  tstop += tabspaces)
			*tstop = 1;
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

void redraw(void)
{
	for (int i = 0; i < term.rows; ++i) {
		for (int j = 0; j < term.cols; ++j) {
			if (term.screen[i][j])
				xdraw_char(term.screen[i][j], j, i);
			else
				break;
		}
	}
}
		   
void insert_char(char c)
{
	xdraw_char(c, term.xcur, term.ycur);
	term.screen[term.ycur][term.xcur++] = c;
	
	if (term.xcur == term.cols) {
		++term.ycur;
		term.xcur = 0;
	}
}

void delete_char(void)
{
	if (term.xcur == 0) {
		--term.ycur;
		term.xcur = term.cols;
	}
	term.screen[term.ycur][--term.xcur] = 0;
	xdelete_char(term.xcur, term.ycur);
}

void newline(void)
{
	++term.ycur;
	term.xcur = 0;
}

void linefeed(void)
{
	++term.ycur;
}

void carreturn(void)
{
	term.xcur = 0;
}

void backspace(void)
{
	if (term.xcur == 0) {
		term.xcur = term.cols - 1;
		--term.ycur;
	}
	else {
		--term.xcur;
	}
}

void moveto(int x, int y)
{
	if (x >= term.cols)
		term.xcur = term.cols - 1;
	else
		term.xcur = x;

	if (y >= term.rows)
		term.ycur = term.rows - 1;
	else
		term.ycur = y;
}

void htab(void)
{
	while(term.xcur < term.cols && !term.tabs[term.xcur])
		++term.xcur;

	if (term.xcur == term.cols) {
		term.xcur = 0;
		++term.ycur;
	}
}

