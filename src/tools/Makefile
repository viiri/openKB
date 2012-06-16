CC=gcc

CFLAGS=-g
LDFLAGS=

SDL_CFLAGS=-g `sdl-config --cflags` -DHAVE_LIBSDL
SDL_LDFLAGS=`sdl-config --libs` -lSDL_image

LIB_SOURCES=../lib/kbdir.c ../lib/kbfile.c ../lib/dos-data.c ../lib/dos-img.c ../lib/dos-snd.c ../lib/dos-cc.c ../lib/kbstd.c ../lib/kbres.c ../lib/kbsound.c ../lib/kbauto.c ../../vendor/strlcat.c ../../vendor/strlcpy.c

all: unexepack kbcc kbview kbrowse kbmaped

unexepack:
	$(CC) $(CFLAGS) unexepack.c -o unexepack $(LDFLAGS)

kbcc:
	$(CC) $(CFLAGS) kbcc.c -o kbcc $(LDFLAGS)

kbview:
	$(CC) $(SDL_CFLAGS) kbview.c -o kbview $(SDL_LDFLAGS)

kbplay:
	$(CC) $(SDL_CFLAGS) kb_play.c -o kbplay $(SDL_LDFLAGS)

kbrowse:
	$(CC) $(SDL_CFLAGS) kbrowse.c ../../vendor/inprint.c $(LIB_SOURCES) -o kbrowse $(SDL_LDFLAGS)

kbmaped: kbmaped.c
	$(CC) $(SDL_CFLAGS) kbmaped.c ../save.c ../../vendor/inprint.c $(LIB_SOURCES) -o kbmaped $(SDL_LDFLAGS)

clean:
	rm -f *.o *.a unexepack kbcc kbview kbplay kbrowse kbmaped
