#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

const char program_name[] = "temu";
const char fontname[] = "fixed";

const unsigned int cols = 80;
const unsigned int rows = 24;

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

/* KEvent table */
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
		  .event_mask       = StructureNotifyMask | KeyPressMask };

	client.win = XCreateWindow(client.display, client.root,
				   0, 0,
				   client.width, client.height, 0,
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

int main(int argc, char *argv[])
{
	/*
	 * TODO: argument processing! Need proper spec.
	 */
	
	 char *locale = setlocale(LC_CTYPE, "");
	 if (!XSupportsLocale()) {
		 fprintf(stderr, "%s: locale %s not supported, using default\n", program_name, locale);
		 setlocale(LC_CTYPE, "C");
	 }
	 
	XSetLocaleModifiers("");
	
	window_init(); // configure and open terminal window
	
	/* main X11 event loop - poll for XEvent and call handler. */
	
	while (1) {
		XEvent ev;
		XNextEvent(client.display, &ev);
		if (handler[ev.type]) {
			handler[ev.type](&ev);
			continue;
		}
	}
}

static void expose(XEvent *ev)
{
}

static void destroy(XEvent *ev)
{
	XFreeFont(client.display, client.font_info);
	XFreeGC(client.display, client.gc);
	XCloseDisplay(client.display);
	exit(EXIT_SUCCESS);
}

static void configure(XEvent *ev)
{
	if (ev->xconfigure.width == client.width && ev->xconfigure.height == client.height)
		return;

	client.width = ev->xconfigure.width;
	client.height = ev->xconfigure.height;
}

static void keypress(XEvent *ev)
{
	KeySym key;
	char buf[64];
	int len;

	len = XLookupString(&ev->xkey, buf, 64, &key, NULL);

	static int x = 10;
	static int y = 10;

	if (len == 0)
		return;
	else if (buf[0] < 0x20) {
		buf[1] = buf[0] + '@';
		buf[0] = '^';

		XDrawString(client.display, client.win, client.gc,
			    x, y, buf, 2);
		x += XTextWidth(client.font_info, buf, 2);
	}
	else if (buf[0] == 0x7f) {
		buf[1] = '?';
		buf[0] = '^';

		XDrawString(client.display, client.win, client.gc,
			    x, y, buf, 2);
		x += XTextWidth(client.font_info, buf, 2);
	}
	else {
		XDrawString(client.display, client.win, client.gc,
			    x, y, buf, 1);
		x += XTextWidth(client.font_info, buf, 1);
	}

	if (x >= client.width - client.font_info->max_bounds.width) {
		x = 5;
		y += client.font_info->ascent;
	}
}
