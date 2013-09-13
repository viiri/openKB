CC=gcc

GROFF=nroff -man

INSTALL=/usr/bin/install -c
INSTALL_DATA=${INSTALL} -m 644

prefix=/usr/local
exec_prefix=${prefix}
datarootdir=${prefix}/share
mandir=${datarootdir}/man
bindir=${exec_prefix}/bin
datadir=${datarootdir}


CFLAGS=-g -O2 -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
LDFLAGS=-lpng -lSDL_image  -L/usr/lib/x86_64-linux-gnu -lSDL

VERSION=0.0.1

MSYS_TEST=$(shell uname -so | grep -i msys | grep -i mingw32)
ifneq ("$(MSYS_TEST)","")
	CFLAGS+=-I/mingw/include/SDL -DUSE_WINAPI
endif

CONFIG_SOURCE=src/config.h.in
CONFIG_HEADER=src/config.h

LIB_SOURCES=\
	vendor/strlcat.c vendor/strlcpy.c \
	src/lib/kbstd.c src/lib/kbconf.c \
	src/lib/kbfile.c src/lib/kbdir.c src/lib/kbres.c \
	src/lib/free-data.c \
	src/lib/dos-data.c src/lib/dos-cc.c src/lib/dos-img.c src/lib/dos-snd.c \
	src/lib/md-rom.c \
	src/lib/kbauto.c

LIB_BINARY=src/libkb.a

VEND_SOURCES=vendor/scale2x.c vendor/inprint.c vendor/savepng.c
VEND_DEPS=vendor/hfs.h vendor/rsrc.h vendor/libhfs/hfs.c vendor/librsrc/rsrc.c

GAME_SOURCES=src/main.c src/save.c src/game.c src/play.c src/bounty.c src/env-sdl.c src/ui.c
GAME_BINARY=openkb

GAME_DIST=$(DESTDIR)$(GAME_BINARY)-$(VERSION)

GAME2_SOURCES=src/combat.c src/bounty.c src/play.c src/env-sdl.c
GAME2_BINARY=netkb

MAN_SOURCES=docs/openkb.man docs/netkb.man
MAN_PAGES=$(MAN_SOURCES:.man=.6)

LIBHFS_BINARY=vendor/libhfs.a
LIBRSRC_BINARY=vendor/librsrc.a

LIB_OBJECTS=$(LIB_SOURCES:.c=.o) $(LIBHFS_BINEARY) $(LIBRSRC_BINARY)
VEND_OBJECTS=$(VEND_SOURCES:.c=.o)
GAME_OBJECTS=$(GAME_SOURCES:.c=.o)
GAME2_OBJECTS=$(GAME2_SOURCES:.c=.o)

.SUFFIXES: .6 .man

all: $(GAME_BINARY)

$(LIBHFS_BINARY): $(VEND_DEPS)
	cd vendor/libhfs; ./configure; cd ../..
	make -C vendor/libhfs
	cp vendor/libhfs/libhfs.a vendor/.

$(LIBRSRC_BINARY): $(VEND_DEPS)
	cd vendor/librsrc; ./configure; cd ../..
	make -C vendor/librsrc
	cp vendor/librsrc/librsrc.a vendor/.

$(LIB_BINARY): $(CONFIG_HEADER) $(LIB_OBJECTS)
	ar rcs $(LIB_BINARY) $(LIB_OBJECTS)

$(GAME_BINARY): $(CONFIG_HEADER) $(GAME_OBJECTS) $(LIB_BINARY) $(VEND_OBJECTS)
	$(CC) $(GAME_OBJECTS) $(VEND_OBJECTS) $(LIB_BINARY) $(LDFLAGS) -o $@

$(GAME2_BINARY): $(CONFIG_HEADER) $(GAME2_OBJECTS) $(LIB_BINARY) $(VEND_OBJECTS)
	$(CC) $(GAME2_OBJECTS) $(VEND_OBJECTS) $(LIB_BINARY) $(LDFLAGS) -lSDL_net -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf vendor/*.o src/*.o $(LIB_OBJECTS) *.o *.a $(GAME_BINARY) $(LIB_BINARY)

.man.6:
	$(GROFF) $< > $@

mans: $(MAN_PAGES)
	@true

$(VEND_DEPS):
	vendor/drop.sh vendor/

$(VEND_SOURCES):
	vendor/drop.sh vendor/

vendor-clean:
	rm $(VEND_SOURCES) $(VEND_DEPS)

vendor: $(VEND_SOURCES)
	@true

config:
	echo "Making config"
	autoheader
	autoconf
	automake -a -c >/dev/null || echo "Ignoring any errors"

dist: vendor-clean vendor clean config $(MAN_PAGES)
	echo "Making dist"
	mkdir -p $(GAME_DIST)
	cp Makefile $(GAME_DIST)
	cp install-sh $(GAME_DIST)
	cp -r -L data $(GAME_DIST)
	cp -r src $(GAME_DIST)
	mkdir $(GAME_DIST)/vendor
	cp vendor/*.c vendor/*.h $(GAME_DIST)/vendor/
	cp -r vendor/libhfs $(GAME_DIST)/vendor/libhfs
	cp -r vendor/librsrc $(GAME_DIST)/vendor/librsrc
	cp configure $(GAME_DIST)/.
	cp configure.ac $(GAME_DIST)/.
	mkdir $(GAME_DIST)/man
	cp docs/*.6 $(GAME_DIST)/man/
	tar -cvzf $(GAME_DIST).tar.gz $(GAME_DIST)
	rm -rf $(GAME_DIST)/

install: all
	@echo "Making install into $(DESTDIR)"
	$(INSTALL) openkb $(DESTDIR)$(bindir)/openkb
	$(INSTALL_DATA) data $(DESTDIR)$(datadir)/openkb-data
	$(INSTALL_DATA) docs/openkb.6 $(DESTDIR)$(mandir)/man6/openkb.6
