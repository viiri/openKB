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

#define GR_LOGO 	0x00	/* subId - undefined */
#define GR_TITLE 	0x01	/* subId - undefined */

#define GR_TROOP	0x02	/* subId - troop index */
#define GR_TILE		0x03	/* subId - tile index */
#define GR_TILESET	0x04	/* subId - continent */
#define GR_VILLAIN	0x05	/* subId - villain index */

#define GR_LOCATION 0x06	/* subId - 0 home 1 town 2 - 6 dwelling */

#define GR_FONT		0x08	/* subId - undefined */

#define GR_UI		0x20	/* subId - element index */
#define GR_SELECT	0x21	/* subId - undefined */

#define SN_TUNE		0x60	/* subId - tune index (0-10) */

#include "SDL.h"
extern SDL_Surface* KB_LoadIMG8(int id, int sub_id);

/* From dos-img.c: */
extern void SDL_add_DOS_palette(SDL_Surface *surf, int bpp);
extern void SDL_blitRAWIMG(SDL_Surface *surf, SDL_Rect *destrect, char *buf, int bpp, word offset, word mask_pos);

extern SDL_Surface* SDL_loadRAWCH(char *buf, int len);
extern SDL_Surface* SDL_loadRAWIMG(char *buf, int len, int bpp);
#include "kbdir.h"
extern SDL_Surface* SDL_loadROWIMG(KB_DIR *dirp, word first, word frames, byte bpp); 

#endif /* _OPENKB_LIBKB_RESOURCES */
