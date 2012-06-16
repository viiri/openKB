CC=gcc

CFLAGS=-g
LFLAGS=

SDL_CFLAGS=-g `sdl-config --cflags` -DHAVE_LIBSDL
SDL_LFLAGS=`sdl-config --libs` -lSDL_image

LIB_SOURCES=../lib/kbdir.c ../lib/kbfile.c ../lib/dos-data.c ../lib/dos-img.c ../lib/dos-cc.c ../lib/kbstd.c ../lib/kbres.c ../lib/kbauto.c ../../vendor/strlcat.c ../../vendor/strlcpy.c

all: kbcc kbview kbplay kbrowse

unexepack:
	gcc $(CFLAGS) unexepack.c -o unexepack $(LFLAGS)

kbcc:
	gcc $(CFLAGS) kbcc.c -o kbcc $(LFLAGS)

kbview:
	gcc $(SDL_CFLAGS) kbview.c -o kbview $(SDL_LFLAGS)

kbplay:
	gcc $(SDL_CFLAGS) kb_play.c -o kbplay $(SDL_LFLAGS)

kbrowse:
	gcc $(SDL_CFLAGS) kbrowse.c ../../vendor/inprint.c $(LIB_SOURCES) -o kbrowse $(SDL_LFLAGS)

kbmaped: kbmaped.c
	gcc $(SDL_CFLAGS) kbmaped.c ../save.c ../../vendor/inprint.c $(LIB_SOURCES) -o kbmaped $(SDL_LFLAGS)

clean:
	rm -rf *.o *.a kbcc kbview kbplay kbrowse