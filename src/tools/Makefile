CC=gcc

CFLAGS=-g
LFLAGS=

SDL_CFLAGS=-g `sdl-config --cflags`
SDL_LFLAGS=`sdl-config --libs`

LIB_SOURCES=../lib/kbdir.c ../lib/kbfile.c ../lib/dos-img.c ../lib/dos-cc.c

all: kbcc kbview kbplay kbrowse

kbcc:
	gcc $(CFLAGS) kbcc.c -o kbcc $(LFLAGS)

kbview:
	gcc $(SDL_CFLAGS) kbview.c -o kbview $(SDL_LFLAGS)

kbplay:
	gcc $(SDL_CFLAGS) kb_play.c -o kbplay $(SDL_LFLAGS)

kbrowse:
	gcc $(SDL_CFLAGS) kbrowse.c ../print.c $(LIB_SOURCES) -o kbrowse $(SDL_LFLAGS)

clean:
	rm -rf *.o *.a kbcc kbview kbplay kbrowse