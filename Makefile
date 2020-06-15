#!/usr/bin/make -f
#
# Makefile for temu - simple terminal emulator

SHELL = /bin/sh

### Configuration ###
TARGET = temu

CC = gcc

LIBS = -lX11 -lutil

# Compiler flags
CFLAGS = -Og -ggdb -Wall -Wextra -pedantic -std=gnu99

# Linker flags
LDFLAGS =

# Install directory
BINDIR = ./

### Rules ###

SRC = xwin.c pty.c term.c
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d)


$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

include $(DEP)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c, %.d
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY:
clean:
	rm -f $(OBJ) $(DEP)





