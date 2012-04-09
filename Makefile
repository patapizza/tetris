SHELL = /bin/sh
CC = gcc
prefix = /usr
includedir = $(prefix)/include
tetris: tetris.c
	$(CC) -Wall -I$(includedir)/SDL $< -o $@ -lSDL -lSDL_image -lSDL_gfx -lSDL_ttf -lm
