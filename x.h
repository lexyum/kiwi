#ifndef X_WIN_H
#define X_WIN_H

#include <X11/Xlib.h>

struct xclient {
	Display *display;
	Window root;
	Window win;
	GC gc;
	XWindowAttributes attr;
	int screen;
};

void xinit(void);
void xfree(void);

#endif /* X_WIN_H */
	
