#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "term.h"

const int tabspaces = 8;

void xclear_region(int, int, int, int);
void xdraw_char(char, int, int);
void xdraw_cursor(char, int, int);
void xdelete_char(int, int);

static void redraw_region(int, int, int, int);
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
	for (i = 0; i <= term.ycur - rows; ++i) {
		free(term.screen[i]);
		free(term.alt[i]);
	}
	if (i > 0) {
		memmove(term.screen, term.screen + i, rows * sizeof(*term.screen));
		memmove(term.alt, term.alt + i, rows * sizeof(*term.alt));
		term.ycur = rows - 1;
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

static void redraw_region(int col1, int col2, int row1, int row2)
{
	xclear_region(col1, col2, row1, row2);
	for (int i = row1; i <= row2; ++i) {
		for (int j = col1; j <= col2; ++j) {
			if (term.screen[i][j])
				xdraw_char(term.screen[i][j], j, i);
			else
				break;
		}
	}
	if (col1 <= term.xcur && term.xcur <= col2
	    && row1 <= term.ycur && term.ycur <= row2)
		xdraw_cursor(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
}

void redraw(void)
{
	redraw_region(0, term.cols - 1, 0, term.rows - 1);
}

void insert_char(char c)
{
	term.screen[term.ycur][term.xcur] = c;
	xdraw_char(c, term.xcur, term.ycur);
	
	if (++term.xcur == term.cols)
		newline();

	term.screen[term.ycur][term.xcur] = ' ';
	xdraw_cursor(' ', term.xcur, term.ycur);
}

void delete_char(void)
{
	if (term.xcur == 0) {
		--term.ycur;
		term.xcur = term.cols - 1;
	}
	term.screen[term.ycur][--term.xcur] = 0;
	xdelete_char(term.xcur, term.ycur);
}

void newline(void)
{
	if (term.ycur == term.rows - 1)
		scrolldown(1);
	++term.ycur;
	term.xcur = 0;
}

void linefeed(void)
{
	/* scrolldown calls redraw(), so don't clear cursor until after */
	if (term.ycur == term.rows - 1)
		scrolldown(1);
	
	xdraw_char(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
	
	++term.ycur;
	term.screen[term.ycur][term.xcur] = ' ';
	xdraw_cursor(' ', term.xcur, term.ycur);
}

void carreturn(void)
{
	xdraw_char(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
	term.xcur = 0;
	xdraw_cursor(term.screen[term.ycur][0], 0, term.ycur);
}

void backspace(void)
{
	xdraw_char(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
	if (term.xcur != 0)
		--term.xcur;
	xdraw_cursor(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
}

void moveto(int x, int y)
{
	xdraw_char(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
	if (x >= term.cols)
		term.xcur = term.cols - 1;
	else
		term.xcur = x;

	if (y >= term.rows)
		term.ycur = term.rows - 1;
	else
		term.ycur = y;
	xdraw_cursor(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
}

void htab(void)
{
	xdraw_char(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
	while (term.xcur < term.cols - 1 && !term.tabs[term.xcur])
		term.screen[term.ycur][term.xcur++] = ' ';
	xdraw_cursor(term.screen[term.ycur][term.xcur], term.xcur, term.ycur);
}

/*
 * Scroll down, keeping the cursor on screen.
 * No scrollback buffer, so can only scroll in one direction.
 */
void scrolldown(int n)
{
	int shift;

	// check we have room
	if (n > term.ycur)
		shift = term.ycur;
	else
		shift = n;

	for (int i = 0; i < shift; ++i)
		free(term.screen[i]);

	memmove(term.screen, term.screen + shift, (term.rows - shift) * sizeof(*term.screen));

	for (int i = term.rows - shift; i < term.rows; ++i)
		term.screen[i] = calloc_test(term.cols, sizeof(*term.screen[i]));

	term.ycur -= shift;
	// TODO: delay redraw
	redraw();
}
