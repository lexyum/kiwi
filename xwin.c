#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <poll.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "pty.h"
#include "term.h"

const char program_name[] = "temu";
const char fontname[] = "fixed";

/* initial terminal dimensions */
const unsigned int cols = 80;
const unsigned int rows = 24;

/* input polling timeout */
const int timeout = 30;

/* program window and global X information */
static struct {
	Display *display;
	XFontStruct *font_info;
	Window root;
	Window win;
	GC gc;
	int screen;
	int width;
	int height;
} client;

/* XEvent table */
static void configure(XEvent *);
static void destroy(XEvent *);
static void expose(XEvent *);
static void keypress(XEvent *);

static void (*handler[LASTEvent])(XEvent *) =
	{
	 [Expose] = &expose,
	 [DestroyNotify] = &destroy,
	 [KeyPress] = &keypress,
	 [ConfigureNotify] = &configure
	};

/* Initialise X font */
void font_init(void)
{
	if (!(client.font_info = XLoadQueryFont(client.display, fontname))) {
		fprintf(stderr, "%s: unable to load font %s, using fixed\n",
			program_name, fontname);
		client.font_info = XLoadQueryFont(client.display, "fixed");
	}
}

/* Set window manager hints */
void wm_hints(void)
{
	XClassHint class = { .res_name = strdup(program_name),
			     .res_class = strdup(program_name) };
	XSetClassHint(client.display, client.win, &class);
	free(class.res_name);
	free(class.res_class);
}

/* Initialise program window  */
void window_init(void)
{
	unsigned long gcmask;
	XGCValues gcvalues;
	XSetWindowAttributes set_attr;

	client.display = XOpenDisplay(NULL);
	if (!client.display) {
		fputs("error: cannot connect to X server", stderr);
		exit(EXIT_FAILURE);
	}
	
	font_init();
	
	// populate xclient
	client.screen = DefaultScreen(client.display);
	client.root = RootWindow(client.display, client.screen);

	/* determine window size from terminal geometry */

	client.width =  cols * client.font_info->max_bounds.width;
	client.height =  rows * client.font_info->max_bounds.ascent;

	set_attr = (XSetWindowAttributes)
		{ .background_pixel = BlackPixel(client.display, client.screen),
		  .border_pixel     = BlackPixel(client.display, client.screen),
		  .event_mask       = StructureNotifyMask | KeyPressMask | ExposureMask};

	client.win = XCreateWindow(client.display, client.root,
				   0, 0,
				   client.width, client.height, 5,
				   CopyFromParent,
				   InputOutput,
				   CopyFromParent,
				   CWBackPixel | CWBorderPixel | CWEventMask,
				   &set_attr);

	gcmask = GCForeground | GCBackground | GCFont;
	gcvalues = (XGCValues){ .foreground = WhitePixel(client.display, client.screen),
				.background = BlackPixel(client.display, client.screen),
				.font = client.font_info->fid };

	wm_hints();

	client.gc = XCreateGC(client.display, client.win, gcmask, &gcvalues);
	

	XMapWindow(client.display, client.win);

	/* is this necessary? */
	XSync(client.display, False);
}

void xdraw_char(char c, int col, int row)
{
	int cw, ch, x, y;
	cw = client.font_info->max_bounds.width;
	ch =  client.font_info->max_bounds.ascent
		+ client.font_info->max_bounds.descent;

	// drawing origin such thattop-left bounding box of first character at (0, 0)
	y = client.font_info->max_bounds.ascent + (row * ch);
	x = (col * cw);

	XDrawString(client.display, client.win, client.gc, x, y, &c, 1);
}

void xdelete_char(int col, int row)
{
	int cw, ch, x1, y1;
	cw = client.font_info->max_bounds.width;
	ch = client.font_info->max_bounds.ascent
		+ client.font_info->max_bounds.descent;

	// origin of char bounding box, not drawing origin
	y1 = row * ch;
	x1 = client.font_info->min_bounds.lbearing + col * cw;

	XClearArea(client.display, client.win, x1, y1, cw, ch, False);
}

int main(int argc, char *argv[])
{
	/*
	 * TODO: argument processing! Need proper spec.
	 */
	int pty_fd;
	XEvent ev;
	char *locale = setlocale(LC_CTYPE, "");
	if (!XSupportsLocale()) {
		fprintf(stderr, "%s: locale %s not supported, using default\n", program_name, locale);
		setlocale(LC_CTYPE, "C");
	}
	XSetLocaleModifiers("");

	window_init();       	// configure and open terminal window


	if ((pty_fd = pty_init(NULL, NULL)) == -1) {
		XDestroyWindow(client.display, client.win);
		destroy(NULL);
	}

	do {
		XNextEvent(client.display, &ev);

		if (ev.type == ConfigureNotify)
			configure(&ev);
	} while (ev.type != MapNotify);

	/* main X11 event loop - poll for XEvent and call handler. */
	
	while (1) {
		struct pollfd fds = { .fd = pty_fd, .events = POLLIN };

		/*
		 * This works for now, but is really not the best way to run
		 * - polling with timeout 0 is necessary to ensure we don't 
		 *   miss XEvents, but means a continuous stream of syscalls
		 *   when idling.
		 *
		 * TODO: come up with a way to 'sleep' when nothing is happening,
		 *       without affecting long-running commands.
		 */
		if (poll(&fds, 1, timeout) == -1) {
			if (errno = EINTR)
				continue;
			else {
				perror("poll");
				exit(EXIT_FAILURE);
			}
		}

		if (fds.revents & POLLIN)
			pty_read();


		while (XPending(client.display)) {
			XNextEvent(client.display, &ev);
			if (handler[ev.type]) {
				handler[ev.type](&ev);
			}
		}

	}
}

static void expose(XEvent *ev)
{
	XClearWindow(client.display, client.win);
	redraw();
}

static void destroy(XEvent *ev)
{
	term_free();
	XFreeFont(client.display, client.font_info);
	XFreeGC(client.display, client.gc);
	XCloseDisplay(client.display);
	exit(EXIT_SUCCESS);
}

/*
 * This works, but we get ugly flickering when a window is resized by 
 * dragging the corner.
 *
 * TODO: Fix this! Be more intelligent about resizing and redisplay
 */
static void configure(XEvent *ev)
{
	if (ev->xconfigure.width == client.width && ev->xconfigure.height == client.height)
		return;

	client.width = ev->xconfigure.width;
	client.height = ev->xconfigure.height;

	/* resize terminal */
	int cheight, cwidth;
	int rows, cols;

	cheight = client.font_info->max_bounds.ascent + client.font_info->max_bounds.descent;
	cwidth = client.font_info->max_bounds.width;

	rows = client.height/cheight;
	cols = client.width/cwidth;

	/* Don't resize if window is not mapped */
	if (rows && cols)
		term_resize(rows, cols);
}

static void keypress(XEvent *ev)
{
	KeySym key;
	char buf[8];
	int len;

	len = XLookupString(&ev->xkey, buf, 8, &key, NULL);

	if (len == 0)
		return;

	pty_write(buf, len);
}

void xclear(void)
{
	XClearWindow(client.display, client.win);
}
