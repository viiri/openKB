GROFF=nroff -man
CC=gcc

INSTALL=install

prefix=/usr
exec_prefix=$(prefix)
datarootdir=$(prefix)/share
mandir=$(datarootdir)/man
bindir=$(exec_prefix)/bin
#datadir=$(datarootdir)
datadir=/var/games/openkb

CFLAGS=-g `sdl-config --cflags` -DHAVE_LIBSDL
LDFLAGS=`sdl-config --libs` -lSDL_image

VERSION=$(shell sed -nr '/define\s+PACKAGE_VERSION/ s/.*"(.*?)"/\1/p' src/config.h)

MSYS_TEST=$(shell uname -so | grep -i msys | grep -i mingw32)
ifneq ("$(MSYS_TEST)","")
	CFLAGS+=-I/mingw/include/SDL -DUSE_WINAPI
endif

LIB_SOURCES=src/lib/kbauto.c src/lib/kbconf.c src/lib/kbres.c src/lib/kbfile.c src/lib/kbdir.c src/lib/dos-cc.c src/lib/dos-img.c src/lib/kbstd.c vendor/strlcat.c vendor/strlcpy.c src/lib/md-rom.c 
LIB_BINARY=src/libkb.a

VEND_SOURCES=vendor/scale2x.c vendor/inprint.c

GAME_SOURCES=src/main.c src/save.c src/game.c src/bounty.c src/env-sdl.c
GAME_BINARY=openkb

GAME_DIST=$(GAME_BINARY)-$(VERSION)

GAME2_SOURCES=src/combat.c src/bounty.c src/env-sdl.c
GAME2_BINARY=netkb

MAN_SOURCES=docs/openkb.man docs/netkb.man
MAN_PAGES=$(MAN_SOURCES:.man=.6)

LIB_OBJECTS=$(LIB_SOURCES:.c=.o)
VEND_OBJECTS=$(VEND_SOURCES:.c=.vo)
GAME_OBJECTS=$(GAME_SOURCES:.c=.o)
GAME2_OBJECTS=$(GAME2_SOURCES:.c=.o)

.SUFFIXES: .vo .6 .man

all: $(GAME_BINARY)

$(LIB_BINARY): $(LIB_OBJECTS)
	ar rcs $(LIB_BINARY) $(LIB_OBJECTS)

.c.vo: vendor
	$(CC) -c $(CFLAGS) $< -o $@

$(GAME_BINARY): $(GAME_OBJECTS) $(LIB_BINARY) $(VEND_OBJECTS)
	$(CC) $(GAME_OBJECTS) $(VEND_OBJECTS) $(LIB_BINARY) $(LDFLAGS) -o $@

$(GAME2_BINARY): $(GAME2_OBJECTS) $(LIB_BINARY) $(VEND_OBJECTS)
	$(CC) $(GAME2_OBJECTS) $(VEND_OBJECTS) $(LIB_BINARY) $(LDFLAGS) -lSDL_net -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf vendor/*.o vendor/*.vo src/*.o src/lib/*.o *.o *.a $(GAME_BINARY) $(LIB_BINARY)

.man.6:
	$(GROFF) $< > $@

mans: $(MAN_PAGES)
	@true

.phony:
	@true

vendor-clean:
	rm vendor/*.c

vendor: .phony
	vendor/drop.sh vendor/

config: .phony
	echo "Making config"
	autoheader
	autoconf
	./configure

dist: vendor clean config
	echo "Making dist"
	mkdir -p $(GAME_DIST)
	cp Makefile $(GAME_DIST)
	cp -r -L data $(GAME_DIST)
	cp -r src $(GAME_DIST)
	mkdir $(GAME_DIST)/vendor
	cp vendor/*.c vendor/*.h $(GAME_DIST)/vendor/
	cp configure $(GAME_DIST)/.
	cp configure.ac $(GAME_DIST)/.
	mkdir $(GAME_DIST)/man
	cp docs/*.6 $(GAME_DIST)/man/
	tar -cvzf $(GAME_DIST).tar.gz $(GAME_DIST)
	rm -rf $(GAME_DIST)/

install: all
	echo "Making install into $(DESTDIR)"
	$(INSTALL) openkb $(DESTDIR)$(bindir)/openkb
	$(INSTALL) -d data $(DESTDIR)$(datadir)/openkb-data
	$(INSTALL) docs/openkb.6 $(DESTDIR)$(mandir)/man6/openkb.6