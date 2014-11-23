CC=gcc

CFLAGS=-g -Wall
LDFLAGS=

SDL_CFLAGS=-g `sdl-config --cflags` -DHAVE_LIBSDL
SDL_LDFLAGS=`sdl-config --libs` -lSDL_image

LIB_SOURCES=../lib/kbdir.c ../lib/kbfile.c ../lib/free-data.c ../lib/md-rom.c ../lib/dos-exe.c ../lib/dos-data.c ../lib/dos-img.c ../lib/dos-snd.c ../lib/dos-cc.c ../lib/kbstd.c ../lib/kbres.c ../lib/kbconf.c ../lib/kbsound.c ../lib/kbauto.c ../../vendor/strlcat.c ../../vendor/strlcpy.c

all: unexecomp unexepack kbcc kbview kbrowse kbmaped

unexecomp: unexecomp.c
	$(CC) $(CFLAGS) unexecomp.c -o unexecomp $(LDFLAGS)

unexepack: unexepack.c
	$(CC) $(CFLAGS) unexepack.c -o unexepack $(LDFLAGS)

kbcc: kbcc.c
	$(CC) $(CFLAGS) kbcc.c -o kbcc $(LDFLAGS)

kbview: kbview.c
	$(CC) $(SDL_CFLAGS) kbview.c -o kbview $(SDL_LDFLAGS)

kbplay:
	$(CC) $(SDL_CFLAGS) kb_play.c -o kbplay $(SDL_LDFLAGS)

kbrowse: kbrowse.c
	$(CC) $(SDL_CFLAGS) -DHAVE_LIBPNG -c ../../vendor/savepng.c $(SDL_LDFLAGS)
	$(CC) $(SDL_CFLAGS) kbrowse.c ../../vendor/inprint.c -DHAVE_LIBPNG ../../vendor/savepng.c -UHAVE_LIBPNG $(LIB_SOURCES) -o kbrowse $(SDL_LDFLAGS) savepng.o

kbmaped: kbmaped.c
	$(CC) $(SDL_CFLAGS) kbmaped.c ../save.c ../env-sdl.c ../../vendor/inprint.c ../../vendor/scale2x.c $(LIB_SOURCES) -o kbmaped $(SDL_LDFLAGS)

clean:
	rm -f *.o *.a unexecomp unexepack kbcc kbview kbplay kbrowse kbmaped
