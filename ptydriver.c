/* 
 * Simple pty setup for testing read/write blocking, terminal settings etc.
 * without hooking up stdin/stdout/stderr to the pty on the slave side.
 * 
 * DO NOT CHANGE THIS FILE - create another source file with definitions for
 * master() and slave() and link against it
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <wait.h>

void master(int);
void slave(int);

static int pty_init(void (*slave_fn)(int))
{
	int m_fd, s_fd; // master/slave file descriptors
	
	if (openpty(&m_fd, &s_fd, NULL, NULL, NULL) == -1)
		return -1;

	switch(fork()) {
	case -1 :
		close(s_fd);
		close(m_fd);
		return -1;
	case 0 :
		close(m_fd);
		slave_fn(s_fd);
		close(s_fd);
		_exit(0);
	default :
		close(s_fd);
		return m_fd;
	}
}

int main(void)
{
	int m_fd;
	if ((m_fd = pty_init(&slave)) == -1) {
		perror("failed to open pty");
		exit(EXIT_FAILURE);
	}

	master(m_fd);
	wait(NULL);
	close(m_fd);
	
	exit(EXIT_SUCCESS);
}
