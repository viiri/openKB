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
#include "kbauto.h"

#include "kbfile.h"

#include "SDL.h"

void SDL_Blit1BPP(char *src, SDL_Surface *dest, SDL_Rect *dstrect)
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
			*p = c[j];

			if (++x >= dstrect->w) { x = 0; y++; }
		}
	}
}
void SDL_Blit2BPP(char *src, SDL_Surface *dest, SDL_Rect *dstrect)
{
	/* 8 bits contain 4 pixels */
	dword j;
	dword x = 0, y = 0;

	while (y < dstrect->h) {

		char test = *src++;
		char c[4];

		c[0] = ((test & 0xC0) >> 6);// bin:11000000
		c[1] = ((test & 0x30) >> 4);// bin:00110000
		c[2] = ((test & 0x0C) >> 2);// bin:00001100 
		c[3] = ((test & 0x03) >> 0);// bin:00000011

		for (j = 0; j < 4; j++) {
			Uint8 *p;

			p = (Uint8*) dest->pixels + (y + dstrect->y) * dest->w + (x + dstrect->x);
			*p = c[j];

			if (++x >= dstrect->w) { x = 0; y++; }
		}
	}
}
void SDL_Blit4BPP(char *src, SDL_Surface *dest, SDL_Rect *dstrect)
{
	/* 8 bits contain 2 pixels */
	dword j;
	dword x = 0, y = 0;

	while (y < dstrect->h) {

		char test = *src++;
		char c[2];

		c[0] = ((test & 0xF0) >> 4);// bin:11110000
		c[1] = ((test & 0x0F) >> 0);// bin:00001111

		for (j = 0; j < 2; j++) {
			Uint8 *p;

			p = (Uint8*) dest->pixels + (y + dstrect->y) * dest->w + (x + dstrect->x);
			*p = c[j];

			if (++x >= dstrect->w) { x = 0; y++; }
		}
	}
}
void SDL_Blit8BPP(char *src, SDL_Surface *dest, SDL_Rect *dstrect)
{
	/* 8 bits contain 1 pixel */
	dword x = 0, y = 0;
	while (y < dstrect->h) {
		Uint8 *p;

		p = (Uint8*) dest->pixels + (y + dstrect->y) * dest->w + (x + dstrect->x);
		*p = *src++;

		if (++x >= dstrect->w) { x = 0; y++; }
	}
}
void SDL_BlitMASK(char *src, SDL_Surface *dest, SDL_Rect *dstrect)
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
	0, // 00 // black // bin:00
	15, // 01 // cyan // bin:01
	15, // 02 // magenta // bin:10
	15, // 03 // white // bin:11
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
	int i;
	SDL_Color pal[256];
	for (i = 0; i < 256; i++) {
		Uint32 color = ega_pallete_rgb[i];
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
