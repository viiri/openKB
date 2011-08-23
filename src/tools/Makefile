CC=gcc

CFLAGS=-g
LFLAGS=

SDL_CFLAGS=-g `sdl-config --cflags`
SDL_LFLAGS=`sdl-config --libs`

all: kbcc kbview

kbcc:
	gcc $(CFLAGS) kbcc.c -o kbcc $(LFLAGS)

kbview:
	gcc $(SDL_CFLAGS) kbview.c -o kbview $(SDL_LFLAGS)

clean:
	rm -rf *.o *.a kbcc kbview 