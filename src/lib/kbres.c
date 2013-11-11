/*
 *  kbres.c -- lots of different resource/SDL-related functions, a mess really 
 *  Copyright (C) 2011 Vitaly Driedfruit
 *
 *  This file is part of openkb.
 *
 *  openkb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  openkb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with openkb.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "kbres.h"

/* Expand resource macros */
#define _(R) # R ,
const char *KBresid_names[] = {
RESOURCES
};
#undef _ 

#ifdef HAVE_LIBSDL
/* SDL flavor. */
#include <SDL.h>
#ifdef HAVE_LIBSDL_IMAGE
#include <SDL_image.h>
#endif
#include "kbfile.h"

inline SDL_Surface* SDL_CreatePALSurface(Uint32 width, Uint32 height)
{
	return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 8, 0xFF, 0xFF, 0xFF, 0x00);
}

inline void SDL_ClonePalette(SDL_Surface *dst, SDL_Surface *src)
{
	SDL_SetPalette(dst, SDL_LOGPAL | SDL_PHYSPAL, src->format->palette->colors, 0, src->format->palette->ncolors);
}

void SDL_BlitXBPP(const char *src, SDL_Surface *dest, SDL_Rect *dstrect, int bpp)
{
	byte base_mask = 0;	

	int j;
	dword x = 0, y = 0;
	
	for (j = 0; j < bpp; j++) base_mask |= (0x01 << j);

	while (y < dstrect->h)	{
		char test = *src++;
#if 1 /* MSB */		
		for (j = 8 / bpp - 1; j > -1 ; j--) {
#else /* LSB */
		for (j = 0; j < 8 / bpp; j++) {
#endif
			Uint8 *p = 

			(Uint8*) dest->pixels + (y + dstrect->y) * dest->pitch + (x + dstrect->x);

			*p = ((test & (base_mask << (j * bpp))) >> (j * bpp));

			if (++x >= dstrect->w) { x = 0; y++; }
		}
	}
}

void SDL_BlitMASK(const char *src, SDL_Surface *dest, SDL_Rect *dstrect)
{
	/* 8 bits contain 8 pixels o_O */
	dword j;
	dword x = 0, y = 0;

	while (y < dstrect->h) {
		char test = *src++;
		char c[8];

		c[0] = ((test & 0x80) >> 7);// bin:10000000
		c[1] = ((test & 0x40) >> 6);// bin:01000000
		c[2] = ((test & 0x20) >> 5);// bin:00100000
		c[3] = ((test & 0x10) >> 4);// bin:00010000
		c[4] = ((test & 0x08) >> 3);// bin:00001000
		c[5] = ((test & 0x04) >> 2);// bin:00000100
		c[6] = ((test & 0x02) >> 1);// bin:00000010
		c[7] = ((test & 0x01) >> 0);// bin:00000001

		for (j = 0; j < 8; j++) {
			Uint8 *p;

			p = (Uint8*) dest->pixels + (y + dstrect->y) * dest->w + (x + dstrect->x);
			if (c[j]) *p = 0xFF;

			if (++x >= dstrect->w) { x = 0; y++; }
		}
	}
}
void SDL_ReplaceIndex(SDL_Surface *dest, SDL_Rect *dstrect, byte search, byte replace)
{
	Uint8 *p;
	dword x = 0, y = 0;
	while (y < dstrect->h) {
		p = (Uint8*) dest->pixels + (y + dstrect->y) * dest->w + (x + dstrect->x);
		if (*p == search) *p = replace;
		if (++x >= dstrect->w) { x = 0; y++; }
	}
}

Uint8 herc_pallete_ega[16] =
{
	0, // 00 
	15, // 01
	15, // 02
	15, // 03
};

Uint8 cga_pallete_ega[16] =
{
	0, // 00 // black // bin:00
	3, // 01 // cyan // bin:01
	5, // 02 // magenta // bin:10
	7, // 03 // white // bin:11
};

Uint32 ega_pallete_rgb[16] = 
{
	0x000000, // 00 // dark black
	0x0000AA, // 01 // dark blue
	0x00AA00, // 02 // dark green
	0x00AAAA, // 03 // cyan	0xAA0000, // 04 // dark red
	0xAA00AA, // 05 // magenta
 	0xAA5500, // 06 // brown
	0xAAAAAA, // 07 // dark white / light gray 	0x555555, // 08 // dark gray / light black	0x5555FF, // 09 // light blue	0x55FF55, // 10 // light green	0x55FFFF, // 11 // light cyan	0xFF5555, // 12 // light red	0xFF55FF, // 13 // light magenta	0xFFFF55, // 14 // light yellow	0xFFFFFF, // 15 // bright white};


void put_mono_pal(SDL_Surface *dest)
{
	int i;
	SDL_Color pal[4];
	for (i = 0; i < 4; i++) {
		Uint32 color = ega_pallete_rgb[herc_pallete_ega[i]];
		pal[i].r = (Uint8)((color & 0x00FF0000) >> 16); 
		pal[i].g = (Uint8)((color & 0x0000FF00) >> 8);
		pal[i].b = (Uint8)((color & 0x000000FF));
	}
	SDL_SetColors(dest, pal, 0, 4);
}

void put_cga_pal(SDL_Surface *dest)
{
	int i;
	SDL_Color pal[4];
	for (i = 0; i < 4; i++) {
		Uint32 color = ega_pallete_rgb[cga_pallete_ega[i]];
		pal[i].r = (Uint8)((color & 0x00FF0000) >> 16); 
		pal[i].g = (Uint8)((color & 0x0000FF00) >> 8);
		pal[i].b = (Uint8)((color & 0x000000FF));
	}
	SDL_SetColors(dest, pal, 0, 4);
}

void put_ega_pal(SDL_Surface *dest)
{
	int i;
	SDL_Color pal[16];
	for (i = 0; i < 16; i++) {
		Uint32 color = ega_pallete_rgb[i];
		pal[i].r = (Uint8)((color & 0x00FF0000) >> 16); 
		pal[i].g = (Uint8)((color & 0x0000FF00) >> 8);
		pal[i].b = (Uint8)((color & 0x000000FF));
	}
	SDL_SetColors(dest, pal, 0, 16);
}

void put_vga_pal(SDL_Surface *dest)
{
	int i, j = 0;
	SDL_Color pal[256];
	for (i = 0; i < 256; i++) {
		Uint32 color = ega_pallete_rgb[i % 16];
		pal[i].r = (Uint8)((color & 0x00FF0000) >> 16); 
		pal[i].g = (Uint8)((color & 0x0000FF00) >> 8);
		pal[i].b = (Uint8)((color & 0x000000FF));
	}
	SDL_SetColors(dest, pal, 0, 256);
}

void put_color_pal(SDL_Surface *dest, Uint32 fore, Uint32 back)
{
	SDL_Color pal[2];
	pal[0].r = (Uint8)((back & 0x00FF0000) >> 16); 
	pal[0].g = (Uint8)((back & 0x0000FF00) >> 8);
	pal[0].b = (Uint8)((back & 0x000000FF));
	pal[1].r = (Uint8)((fore & 0x00FF0000) >> 16); 
	pal[1].g = (Uint8)((fore & 0x0000FF00) >> 8);
	pal[1].b = (Uint8)((fore & 0x000000FF));
	SDL_SetColors(dest, pal, 0, 2);
}

SDL_Surface* KB_LoadIMG(const char *filename) {
	SDL_RWops *rw;
	SDL_Surface *surf = NULL;
	KB_File *f;

//	byte magic = KB_MagicType(filename);

	f = KB_fopen(filename, "rb");
	if (f == NULL) return NULL;
	rw = KBRW_open(f);
	if (f->type == KBFTYPE_INIMG)
		surf = DOS_LoadRAWIMG_RW(rw, imgGroup_filename_to_bpp(filename));
	if (surf == NULL)
		surf = DOS_LoadIMGROW_RW(rw, 0, 255);
#ifdef HAVE_LIBSDL_IMAGE
	if (surf == NULL)
		surf = IMG_Load_RW(rw, 0);
#endif
	if (surf == NULL)
		surf = SDL_LoadBMP_RW(rw, 0);
	SDL_RWclose(rw);
	return surf;
}


/*
 * Load each individual tile surface using the "resolve" callback and copy
 * them into a 8 x 7 tileset.
 * Returns newly allocated SDL_Surface.
 */
SDL_Surface* KB_LoadTileset_TILES(SDL_Rect *tilesize, KBresolve_cb resolve, KBmodule *mod) {

	int i;
	SDL_Rect dst = { 0, 0, tilesize->w, tilesize->h };
	SDL_Rect src = { 0, 0, tilesize->w, tilesize->h };

	SDL_Surface *ts = SDL_CreatePALSurface(8 * dst.w, 70/7 * dst.h);
	if (ts == NULL) return NULL;	

	for (i = 0; i < 72; i++) {
		SDL_Surface *tile = resolve(mod, GR_TILE, i);
		SDL_ClonePalette(ts, tile);
		SDL_BlitSurface(tile, &src, ts, &dst);
		dst.x += dst.w;
		if (dst.x >= ts->w) {
			dst.x = 0;
			dst.y += dst.h;
		}
		SDL_FreeSurface(tile);
	}

	return ts;
}

/*
 * Load two rows of tiles (36 tiles each) using the "resolve" callback and copy
 * them into a 8 x 7 tileset.
 * Returns newly allocated SDL_Surface.
 */
SDL_Surface* KB_LoadTileset_ROWS(SDL_Rect *tilesize, KBresolve_cb resolve, KBmodule *mod) {

	int i;
	SDL_Rect dst = { 0, 0, tilesize->w, tilesize->h };
	SDL_Rect src = { 0, 0, tilesize->w, tilesize->h };
	SDL_Surface *row = NULL;

	SDL_Surface *ts = SDL_CreatePALSurface(8 * dst.w, 70/7 * dst.h);
	if (ts == NULL) return NULL;

	row = resolve(mod, GR_TILEROW, 0);
	if (row->format->palette)
		SDL_ClonePalette(ts, row);
	else
		KB_errlog("Warning - can't read palette in a 24-bpp file\n");

	for (i = 0; i < 72; i++) {

		/* Reload source row in the middle of the loop: */
		if (i == 36) {
			SDL_FreeSurface(row);
			row = resolve(mod, GR_TILEROW, 36);
			src.x = 0;
			src.y = 0;
		}
		if (dst.x >= ts->w) {
			dst.x = 0;
			dst.y += dst.h;
		}
		SDL_BlitSurface(row, &src, ts, &dst);
		if (dst.w != src.w) {
			KB_errlog("Missing pixels for tile %d -- need %d, have %d\n", i, src.w, dst.w);
			dst.w = src.w;
		}
		dst.x += dst.w;
		src.x += src.w;
	}
	SDL_FreeSurface(row);

	return ts;
}

/* Load new tileset, salted according to continent */
SDL_Surface* KB_LoadTilesetSalted(byte continent, KBresolve_cb resolve, KBmodule *mod) {

	int i, tiles_per_row;
	SDL_Rect tilesize, src, dst;
	SDL_Surface *tileset, *tilesalt;

	tileset = resolve(mod, GR_TILESET, 0);
	if (!continent) return tileset; /* Continent 0 is never salted */

	tilesalt = resolve(mod, GR_TILESALT, continent);

	/* Figure out the size of single tile */
	tilesize.w = tilesalt->w / 3;
	tilesize.h = tilesalt->h;
	tiles_per_row = tileset->w / tilesize.w;

	/* Copy tilesalt tiles 0, 1, 2 to tileset tiles 17, 18, 19 */
	src.y = 0;
	dst.w = src.w = tilesize.w;
	dst.h = src.h = tilesize.h;

	for (i = 0; i < 3; i++) {
		int tid = 17 + i;

		int row = tid / tiles_per_row;
		int col = tid % tiles_per_row;

		src.x = i * tilesize.w;
		dst.x = col * tilesize.w;
		dst.y = row * tilesize.h;

		SDL_BlitSurface(tilesalt, &src, tileset, &dst);
	}

	SDL_FreeSurface(tilesalt);
	return tileset;
}

#endif /* HAVE_LIBSDL */
