#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pty.h>
#include <utmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "term.h"

static int pty_fd;
static pid_t slave_pid;

static void child_proc(char *cmd, char *argv[]);

void sigchld(int sig)
{
	pid_t w;
	int wstatus;

	if ((w = waitpid(slave_pid, &wstatus, 0)) == -1) {
		// something wrong - die ungracefully
		perror("waidpid");
		exit(EXIT_FAILURE);
	}
	if (WIFEXITED(wstatus)) {
		fprintf(stderr, "child exited with status %d\n",
			WEXITSTATUS(wstatus));
	}
	else if (WIFSIGNALED(wstatus)) {
		fprintf(stderr, "child terminated due to signal %d\n",
			WTERMSIG(wstatus));
	}
	// TODO: cleanup on exit
	exit(EXIT_SUCCESS);
}		


int pty_init(char *cmd, char *argv[])
{
	switch(slave_pid = forkpty(&pty_fd, NULL, NULL, NULL)) {
	case -1 : // error, let main() handle it
		close(pty_fd);
		return -1;

	case 0 : // child process (slave)
		child_proc(cmd, argv);
		_exit(0);
		
	default : // parent process (master)
		signal(SIGCHLD, &sigchld);
	}
	return pty_fd;
}

void pty_read(void)
{
	static char buf[BUFSIZ];
	ssize_t nread;

	switch (nread = read(pty_fd, buf, BUFSIZ)) {
	case -1 :
		perror("pty_read");
		exit(EXIT_FAILURE);
	case 0 :
		return;
	default :
		/* Control handling */
		for (int i = 0; i < nread; ++i) {
			switch(buf[i]) {
			case '\0' : // Null (do nothing)
				break;
			case '\a' : // Bell (NOT IMPLEMENTED)
				break;
			case '\b' : // Backspace
				backspace();
				break;
			case '\f' : // Formfeed (NOT IMPLEMENTED)
				break;
			case '\t' : // Horizontal tab
				htab();
				break;
			case '\n' : // Line feed
				linefeed();
				break;
			case '\v' : // Vertical tab (NOT IMPLEMENTED)
				break;
			case '\r' : // Carriage return
				carreturn();
				break;
			case 0x1b : // ESC (NEEDS TO BE IMPLEMENTED!)
				break;
			default :
				insert_char(buf[i]);
			}
			
		}
	}
}

void pty_write(char *buf, size_t count)
{
	ssize_t nwritten;
	struct pollfd fds = { .fd = pty_fd, .events = POLLOUT };

	/*
	 * If write() is not complete, try again until the slave clears
	 * the buffer. With tdisc echoing there could be a problem if
	 * the return buffer is filled with echoed characters - calling
	 * pty_read() should stop this.
	 *
	 * TODO: How likely is this really?
	 */
	
	while (count > 0) {
		if (poll(&fds, 1, -1) == -1) {
			if (errno != EINTR) {
				perror("poll");
				exit(EXIT_FAILURE);
			}
		}
		if (fds.revents & POLLOUT) {
			if ((nwritten = write(pty_fd, buf, count)) == -1) {
				perror("write");
				exit(EXIT_FAILURE);
			}
			count -= nwritten;
			buf += nwritten;

			// clear return buffer
			if (count)
				pty_read();
		}
	}
}

void pty_resize(unsigned short rows,
		unsigned short cols,
		unsigned short xpixels,
		unsigned short ypixels)
{
	struct winsize wsize = { rows, cols, xpixels, ypixels };

	if (ioctl(pty_fd, TIOCSWINSZ, &wsize) == -1)
		perror("could not set window size");
}

static void child_proc(char *cmd, char *argv[])
{
	char *sh = "/bin/sh";

	char *args[2] = {sh, NULL};
	execvp(sh, args);
}
		
		
		
