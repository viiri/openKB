/*
 *  kbres.h -- high-level "resource" API (SDL-targeted) 
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
#ifndef _OPENKB_LIBKB_RESOURCES
#define _OPENKB_LIBKB_RESOURCES

#include "kbsys.h"
/*
 * A "resource" is identified by 2 integers 'id' and 'sub_id'.
 *
 * GR_ resources are ready-to-blit SDL_Surfaces.
 * SN_ resources are tunes in ? format
 * PAL_ are arrays of 255 SDL_Colors.
 * DAT_ are byte arrays of pre-defined length.
 * STR_ are asiiz strings.
 * STRL_ are asiiz-lists (\0 acts as a separator, \0\0 ends the list).
 */
#define GR_LOGO 	0x00	/* subId - undefined */
#define GR_TITLE 	0x01	/* subId - undefined */

#define GR_TROOP	0x02	/* subId - troop index */
#define GR_TILE		0x03	/* subId - tile index */
#define GR_TILESET	0x04	/* subId - continent flavor */
#define GR_VILLAIN	0x05	/* subId - villain index */

#define GR_FACE 	0x07	/* subId - face index (villain index * 4) */
#define GR_FACES 	0x0C	/* subId - undefined */

#define GR_COMTILE	0x09	/* subId - combat tile index */
#define GR_COMTILES	0x0A	/* subId - undefined */

#define GR_LOCATION 0x06	/* subId - 0 home 1 town 2 - 6 dwelling */

#define GR_FONT		0x08	/* subId - undefined */

#define GR_CURSOR	0x10	/* subId - undefined */
#define GR_HERO	GR_CURSOR

#define GR_UI		0x20	/* subId - undefined */
#define GR_SELECT	0x21	/* subId - undefined */

#define GR_PORTRAIT	0x22	/* subId - player class */
#define GR_VIEW 	0x24	/* subId - undefined */

#define GR_GOLD 	0x25	/* subId - undefined */
#define GR_PURSE	0x26	/* subId - undefined */
#define GR_COIN		0x27	/* subId - coin index (0-3) */
#define GR_COINS	0x28	/* subId - undefined */

#define GR_VIEW 	0x30	/* artifacts, maps, empty slot, empty map ; subId - undefined */
#define GR_ARTIFACTS 0x31	/* artifacts ; subId - undefined */
#define GR_MAPS 	0x32	/* filled maps; subId - undefined */
#define GR_INVSLOT	0x33	/* an empty inventory slot; subId - undefined */
#define GR_BLANKMAP	0x34	/* an empty map tile; subId - undefined */
#define GR_ARTIFACT	0x35	/* filled map; subId - artifact index (0-7) */
#define GR_ORBMAP	0x36	/* filled map; subId - continent index (0-3) */

#define SN_TUNE		0x60	/* subId - tune index (0-10) */
#define PAL_PALETTE	0x70	/* subId - undefined */
#define COL_TEXT	0x71	/* subId - undefined */

#define DAT_WORLD	0x90	/* complete world map ; subId - undefined */
#define DAT_LAND	0x91	/* map for specific continent ; subId - continent index */

#define RECT_MAP	0xA0	/* SDL_Rect describing map area */
#define RECT_UI 	0xA1	/* SDL_Rect describing ui border area; subId - element index */

#define STR_SIGN	0xE0	/* signpost text ; subId - signpost index */
#define STR_TROOP	0xE1	/* troop name ; subId - troop index */
#define STR_MULTI	0xE2	/* troops name ; subId - troop index */

#define STRL_SIGNS	0xF0	/* signpost texts ; subId - undefined */
#define STRL_TROOPS	0xF1	/* troop names ; subId - undefined */
#define STRL_MULTIS	0xF2	/* troops names ; subId - undefined */

#ifdef HAVE_LIBSDL
/* SDL flavor. */
#include "SDL.h"
#include "kbdir.h" // for KB_DIR
/* Provide usefull functions to modules */
inline SDL_Surface* SDL_CreatePALSurface(Uint32 width, Uint32 height);
extern void SDL_BlitXBPP(const char *src, SDL_Surface *dest, SDL_Rect *dstrect, int bpp);
extern void SDL_ReplaceIndex(SDL_Surface *dest, SDL_Rect *dstrect, byte search, byte replace);

/* Simple palette manipulation */
extern void put_mono_pal(SDL_Surface *dest);
extern void put_cga_pal(SDL_Surface *dest);
extern void put_ega_pal(SDL_Surface *dest);
extern void put_vga_pal(SDL_Surface *dest);
extern void put_color_pal(SDL_Surface *dest, Uint32 fore, Uint32 back);


/* Provide useful functions to potential resource loader */
/*
 * DOS module
 */
extern void DOS_BlitRAWIMG(SDL_Surface *surf, SDL_Rect *destrect, const char *buf, byte bpp, word mask_pos);
extern void DOS_SetColors(SDL_Surface *surf, byte bpp);

extern SDL_Surface* DOS_LoadRAWCH_BUF(char *buf, int len);
extern SDL_Surface* DOS_LoadRAWIMG_BUF(char *buf, int len, byte bpp);
extern SDL_Surface* DOS_LoadIMGROW_RW(SDL_RWops *rw, word first, word frames); 

extern SDL_Surface* DOS_LoadIMGROW_DIR(KB_DIR *d, word first, word frames);

/* Laziest function */ 
SDL_Surface* KB_LoadIMG(const char *filename);

#endif
#endif /* _OPENKB_LIBKB_RESOURCES */
