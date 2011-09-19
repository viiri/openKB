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

#define GR_UI		0x20	/* subId - element index */
#define GR_SELECT	0x21	/* subId - undefined */

#define GR_PORTRAIT	0x22	/* subId - player class */
#define GR_VIEW 	0x24	/* subId - undefined */

#define GR_GOLD 	0x25	/* subId - undefined */

#define SN_TUNE		0x60	/* subId - tune index (0-10) */
#define PAL_PALETTE	0x70	/* subId - undefined */

#define DAT_WORLD	0x90	/* complete world map ; subId - undefined */
#define DAT_LAND	0x91	/* map for specific continent ; subId - continent index */

#define STR_SIGN	0xE0	/* signpost text ; subId - signpost index */
#define STR_TROOP	0xE1	/* troop name ; subId - troop index */
#define STR_MULTI	0xE2	/* troops name ; subId - troop index */

#define STRL_SIGNS	0xF0	/* signpost texts ; subId - undefined */
#define STRL_TROOPS	0xF1	/* troop names ; subId - undefined */
#define STRL_MULTIS	0xF2	/* troops names ; subId - undefined */

#include "SDL.h"

/* From dos-img.c: */
extern void SDL_add_DOS_palette(SDL_Surface *surf, int bpp);
extern void SDL_blitRAWIMG(SDL_Surface *surf, SDL_Rect *destrect, char *buf, int bpp, word offset, word mask_pos);

extern SDL_Surface* SDL_loadRAWCH(char *buf, int len);
extern SDL_Surface* SDL_loadRAWIMG(char *buf, int len, int bpp);
#include "kbdir.h"
extern SDL_Surface* SDL_loadROWIMG(KB_DIR *dirp, word first, word frames, byte bpp); 

#endif /* _OPENKB_LIBKB_RESOURCES */
