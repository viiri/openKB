/*
 *  free-data.c -- Free module
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

#include <SDL.h> 
#include <SDL_image.h>	/* PNG support */

#include "kbconf.h"	/* KBmodule type */
#include "kbres.h"	/* GR_? defines */

#define TILE_W 48
#define TILE_H 34

void* GNU_Resolve(KBmodule *mod, int id, int sub_id) {

	char *image_name = NULL;
	char *image_suffix = NULL;
	char *image_subid = "";
	int is_transparent = 1;

	switch (id) {
		case GR_LOGO:
		{
			image_name = "nwcp";
			image_suffix = ".png";
			is_transparent = 0;
		}
		break;
		case GR_TITLE:
		{
			image_name = "title";
			image_suffix = ".png";
			is_transparent = 0;
		}
		break;		
		case GR_SELECT:
		{
			image_name = "select";
			image_subid = "-0";
			if (sub_id == 1) image_subid="-1";
			if (sub_id == 2) image_subid="-2";
			image_suffix = ".png";
			is_transparent = 0;
		}
		break;
		case GR_FONT:
		{
			image_name = "openkb8x8";
			image_suffix = ".bmp";
			is_transparent = 0;
		}
		break;
		case GR_VILLAIN:
		{
			image_name = DOS_villain_names[sub_id];
			image_suffix = ".png";
			is_transparent = 0;
		}
		break;
		case GR_TROOP:
		{
			image_name = DOS_troop_names[sub_id];
			image_suffix = ".png";
		}
		break;
		case GR_CURSOR:
		{
			image_name = "cursor";
			image_suffix = ".png";
		}
		break;
		case GR_UI:
		{
			image_name = "sidebar";
			image_suffix = ".png";
			is_transparent = 0;
		}
		break;
		case GR_PURSE:
		{
			SDL_Surface *ts = SDL_CreatePALSurface(TILE_W, TILE_H);
			return ts;
		}
		break;
		case GR_VIEW:
		{
			image_name = "view";
			image_suffix = ".png";
			is_transparent = 0;
		}
		break;
		case GR_COINS:
		{
			image_name = "coins";
			image_suffix = ".png";
		}
		break;
		case GR_PIECE:
		{
			image_name = "piece";
			image_suffix = ".png";
		}
		break;
		case GR_COMTILES:
		{
			image_name = "comtiles";
			image_suffix = ".png";
			is_transparent = 1;
		}
		break;
		case GR_TILEROW:	/* subId - row index */
		{
			/* A tile */
			if (sub_id > 35) {
				sub_id -= 36;
				image_name = "tilesetb";
			} else {
				image_name = "tileseta";
			}
			image_suffix = ".png";
			is_transparent = 0;
		}
		break;
		case GR_TILESET:	/* subId - continent */
		{

			/* This one must be assembled */
			int i;
			SDL_Rect dst = { 0, 0, TILE_W, TILE_H };
			SDL_Rect src = { 0, 0, TILE_W, TILE_H };
			SDL_Surface *ts = SDL_CreatePALSurface(8 * dst.w, 70/7 * dst.h);
			SDL_Surface *row = NULL;
			row = GNU_Resolve(mod, GR_TILEROW, 0);
			if (row->format->palette)
				SDL_ClonePalette(ts, row);
			else {
				KB_errlog("Warning - can't read palette in a 24-bpp file\n");
			}
			for (i = 0; i < 72; i++) {
				if (i == 36) {
					SDL_FreeSurface(row);
					row = GNU_Resolve(mod, GR_TILEROW, 36);
					src.x = 0;
					src.y = 0;
				}
				if (dst.x >= dst.w*8) {
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
		break;
		default: break;
	}

	if (image_name) {

		char realname[1024];

		realname[0] = '\0';
		KB_dircpy(realname, mod->slotA_name);
		KB_dirsep(realname);
		KB_strcat(realname, image_name);
		KB_strcat(realname, image_subid);
		KB_strcat(realname, image_suffix);

		KB_debuglog(0, "? FREE IMG FILE: %s\n", realname);

		SDL_Surface *surf = IMG_Load(realname);

		if (surf == NULL) KB_debuglog(0, "> FAILED TO OPEN, %s\n", IMG_GetError());

		if (surf && is_transparent)
			SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);

		return surf;
	}

	return NULL;
}
