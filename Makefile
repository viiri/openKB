CC=gcc

CFLAGS=-g `sdl-config --cflags` -DHAVE_SDL
LFLAGS=`sdl-config --libs` -lSDL_image

VERSION=0.0.1

LIB_SOURCES=src/lib/kbauto.c src/lib/kbconf.c src/lib/kbres.c src/lib/kbfile.c src/lib/kbdir.c src/lib/dos-cc.c src/lib/dos-img.c src/lib/kbstd.c
LIB_BINARY=src/libkb.a

GAME_SOURCES=src/main.c src/save.c src/game.c src/bounty.c src/inprint.c
GAME_BINARY=openkb

GAME_DIST=openkb-$(VERSION)

LIB_OBJECTS=$(LIB_SOURCES:.c=.o)
GAME_OBJECTS=$(GAME_SOURCES:.c=.o)

all: $(GAME_BINARY)

$(LIB_BINARY): $(LIB_OBJECTS)
	ar rcs $(LIB_BINARY) $(LIB_OBJECTS)

vendor/scale2x.o: vendor
	$(CC) -c $(CFLAGS) vendor/scale2x-2.4/contrib/sdl/scale2x.c -o $@

$(GAME_BINARY): $(GAME_OBJECTS) $(LIB_BINARY) vendor/scale2x.o
	$(CC) $(LFLAGS) $(GAME_OBJECTS) $(LIB_BINARY) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf vendor/*.o src/*.o src/lib/*.o *.o *.a $(GAME_BINARY) $(LIB_BINARY)

.phony:
	@true

vendor: .phony
	vendor/drop.sh vendor/

dist: vendor clean
	echo "Making dist"
	mkdir -p $(GAME_DIST)
	cp Makefile $(GAME_DIST)
	cp -r data $(GAME_DIST)
	cp -r src $(GAME_DIST)
	tar -cvzf $(GAME_DIST).tar.gz $(GAME_DIST)
	rm -rf $(GAME_DIST)/
