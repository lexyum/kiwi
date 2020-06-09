#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "x.h"

static struct xclient client;

void xinit(void)
{
	unsigned int width = 20;
	unsigned int height = 20;
	unsigned int border_width = 5;

	client.display = XOpenDisplay(NULL);
	if (!client.display) {
		fputs("error: cannot connect to X server", stderr);
		exit(EXIT_FAILURE);
	}

	// populate xclient
	client.screen = DefaultScreen(client.display);
	client.root = RootWindow(client.display, client.screen);

	XSetWindowAttributes set_attr =
		{ .background_pixel = WhitePixel(client.display, client.screen),
		  .border_pixel     = BlackPixel(client.display, client.screen),
		  .event_mask       = ExposureMask | StructureNotifyMask };

	client.win = XCreateWindow(client.display, client.root,
				   0, 0,
				   width, height, border_width,
				   CopyFromParent,
				   InputOutput,
				   CopyFromParent,
				   CWBackPixel | CWBorderPixel | CWEventMask,
				   &set_attr);

	XMapWindow(client.display, client.win);
	XGetWindowAttributes(client.display, client.win, &client.attr);

	XSync(client.display, False);
}

void xfree(void)
{
	XCloseDisplay(client.display);
}

int main(void)
{
	xinit();

	while (1) {
		XEvent ev;
		XNextEvent(client.display, &ev);
		switch(ev.type) {
		case Expose :
			continue;
		case DestroyNotify :
			goto cleanup;
		default :
			continue;
		}
	}

 cleanup:
	xfree();
	exit(EXIT_SUCCESS);
}
