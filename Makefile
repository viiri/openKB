CC=gcc

CFLAGS=-g `sdl-config --cflags` -DHAVE_SDL
LDFLAGS=`sdl-config --libs` -lSDL_image

VERSION=$(shell sed -nr '/define\s+VERSION/ s/.*"(.*?)"/\1/p' src/main.c)

MSYS_TEST=$(shell uname -so | grep -i msys | grep -i mingw32)
ifneq ("$(MSYS_TEST)","")
	CFLAGS+=-I/mingw/include/SDL -DUSE_WINAPI
endif

LIB_SOURCES=src/lib/kbauto.c src/lib/kbconf.c src/lib/kbres.c src/lib/kbfile.c src/lib/kbdir.c src/lib/dos-cc.c src/lib/dos-img.c src/lib/kbstd.c vendor/strlcat.c vendor/strlcpy.c 
LIB_BINARY=src/libkb.a

VEND_SOURCES=vendor/scale2x.c vendor/inprint.c

GAME_SOURCES=src/main.c src/save.c src/game.c src/bounty.c
GAME_BINARY=openkb

GAME_DIST=$(GAME_BINARY)-$(VERSION)

LIB_OBJECTS=$(LIB_SOURCES:.c=.o)
VEND_OBJECTS=$(VEND_SOURCES:.c=.vo)
GAME_OBJECTS=$(GAME_SOURCES:.c=.o)

.SUFFIXES: .vo

all: $(GAME_BINARY)

$(LIB_BINARY): $(LIB_OBJECTS)
	ar rcs $(LIB_BINARY) $(LIB_OBJECTS)

.c.vo: vendor
	$(CC) -c $(CFLAGS) $< -o $@

$(GAME_BINARY): $(GAME_OBJECTS) $(LIB_BINARY) $(VEND_OBJECTS)
	$(CC) $(GAME_OBJECTS) $(VEND_OBJECTS) $(LIB_BINARY) $(LDFLAGS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf vendor/*.o src/*.o src/lib/*.o *.o *.a $(GAME_BINARY) $(LIB_BINARY)

.phony:
	@true

vendor-clean:
	rm vendor/*.c

vendor: .phony
	vendor/drop.sh vendor/

dist: vendor clean
	echo "Making dist"
	mkdir -p $(GAME_DIST)
	cp Makefile $(GAME_DIST)
	cp -r data $(GAME_DIST)
	cp -r src $(GAME_DIST)
	mkdir $(GAME_DIST)/vendor
	cp vendor/*.c vendor/*.h $(GAME_DIST)/vendor/
	tar -cvzf $(GAME_DIST).tar.gz $(GAME_DIST)
	rm -rf $(GAME_DIST)/
