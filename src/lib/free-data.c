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

#include "kbconf.h"	/* KBmodule type */
#include "kbres.h"	/* GR_? defines */
#include "kbfile.h"	/* KB_File operations */

#include <SDL.h>  	/* SDL data types */
#ifdef HAVE_LIBSDL_IMAGE
#include <SDL_image.h>	/* PNG support */
#else
#warning "SDL_Image is not used, PNG support disabled!"
#endif


#define TILE_W 48
#define TILE_H 34

#define STRL_MAX 1024


dword* GNU_extract_ini(KBmodule *mod, const char *inifile, const char *module, const char *name, int first, int num, dword *dst) {
	KB_File *fd;
	char *filename;
	char line[256];
	char module_fmt[256];
	char module_test[256];
	int module_test_len, name_test_len;
	int i, section;

	filename = KB_fastpath(mod->slotA_name, "/", inifile);
	if (filename == NULL) return NULL; /* Out of memory */

	if (dst == NULL) {
		dst = malloc(sizeof(dword) * num);
		if (dst == NULL) {
			free(filename);
			return NULL; /* Out of memory */
		}
		memset(dst, 0, sizeof(dword) * num);
	}

	KB_debuglog(0, "? FREE INI FILE: %s\n", filename);

	fd = KB_fopen(filename, "r");
	if (fd == NULL) {
		KB_debuglog(0, "> FAILED TO OPEN, %s\n", filename);
		free(filename);
		return NULL;
	}

	sprintf(module_fmt, "[%s]", module);
	section = -1;
	module_test_len = sprintf(module_test, module_fmt, section + 1);
	name_test_len = strlen(name);

	//int iter;
	while (KB_fgets(line, sizeof(line), fd)) {

		if (!strncasecmp(line, module_test, module_test_len)) {
			module_test_len = sprintf(module_test, module_fmt, (++section)+1);
			continue;
		}
		if (section < first) {
			continue;
		}
		if (section > num) {
			break;
		}

		if (!strncasecmp(line, name, name_test_len)) {
			char test[1024];
			int test_len;
			dword val;
			test_len = sprintf(test, "%s = %%d", name);
			if (sscanf(line, test, &val) == 1) {
				dst[section - first] = val;
			}
		}
		//if (iter++ > 21) break;
	}

	KB_fclose(fd);
	free(filename);
	return dst;
}

char* GNU_read_textfile(KBmodule *mod, const char *textfile) {

	KB_File *fd;
	char *filename, *buf;
	int n, i;

	filename = KB_fastpath(mod->slotA_name, "/", textfile);
	if (filename == NULL) return NULL; /* Out of memory */

	KB_debuglog(0, "? FREE TXT FILE: %s\n", filename);

	fd = KB_fopen(filename, "r");
	if (fd == NULL) {
		KB_debuglog(0, "> FAILED TO OPEN, %s\n", filename);
		free(filename);
		return NULL;
	}

	buf = malloc(sizeof(char) * STRL_MAX);
	if (buf == NULL) {
		KB_fclose(fd);
		free(filename);
		return NULL; /* Out of memory */
	}

	n = KB_fread(buf, sizeof(char), STRL_MAX, fd);
	buf[n] = '\0';

	/* Convert multi-line file to a strlist
	for (i = 0; i < n; i++)
		if (buf[i] == '\n') buf[i] = '\0';
	*/
	KB_fclose(fd);
	free(filename);

	return buf;
}

void* GNU_Resolve(KBmodule *mod, int id, int sub_id) {

	char *image_name = NULL;
	char *image_suffix = NULL;
	char *image_subid = "";
	int is_transparent = 1;
#ifdef HAVE_LIBSDL_IMAGE
#define _EXTN ".png"
#else
#define _EXTN ".bmp"
#endif
	switch (id) {
		case GR_LOGO:
		{
			image_name = "nwcp";
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_TITLE:
		{
			image_name = "title";
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;		
		case GR_SELECT:
		{
			image_name = "select";
			image_subid = "-0";
			if (sub_id == 1) image_subid="-1";
			if (sub_id == 2) image_subid="-2";
			image_suffix = _EXTN;
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
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_TROOP:
		{
			image_name = DOS_troop_names[sub_id];
			image_suffix = _EXTN;
		}
		break;
		case GR_CURSOR:
		{
			image_name = "cursor";
			image_suffix = _EXTN;
		}
		break;
		case GR_UI:
		{
			image_name = "sidebar";
			image_suffix = _EXTN;
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
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_COINS:
		{
			image_name = "coins";
			image_suffix = _EXTN;
		}
		break;
		case GR_PIECE:
		{
			image_name = "piece";
			image_suffix = _EXTN;
		}
		break;
		case GR_COMTILES:
		{
			image_name = "comtiles";
			image_suffix = _EXTN;
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
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_TILESET:	/* subId - continent */
		{
			SDL_Rect tilesize = { 0, 0, TILE_W, TILE_H };
			if (sub_id) return KB_LoadTilesetSalted(sub_id, GNU_Resolve, mod);
			return KB_LoadTileset_ROWS(&tilesize, GNU_Resolve, mod);
		}
		break;
		case GR_TILESALT:	/* subId - 0=full; 1-3=continent */
		{
			/* A row of replacement tiles. */
			image_name = "tilesalt";
			image_suffix = _EXTN;
		}
		break;
		case STR_SIGN:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_SIGNS, 0), sub_id);
		}
		break;
		case STRL_SIGNS:
		{
			char *list = GNU_read_textfile(mod, "signs.txt");
			/* Convert multi-line file to a strlist (aka "signs need some extra work") */
			int n = (list ? strlen(list) : 0);
			int i, j = 0;
			for (i = 0; i < n; i++) {
				if (list[i] == '\n') {
					if (j) list[i] = '\0';
					j = 1 - j;
				}
			}
			return list;
		}
		case STRL_CREDITS:	/* multiple lines of credits */
		{
			return GNU_read_textfile(mod, "credits.txt");
		}
		break;
		case STRL_VDESCS:	/* multiple lines of villain description */
		{
			char tmp[128];
			KB_strcpy(tmp, DOS_villain_names[sub_id]);
			KB_strcat(tmp, ".txt");
			return GNU_read_textfile(mod, tmp);
		}
		break;
		case DAT_HPS:  	/* [MAX_TROOPS] hit points for specific troop; subId - undefined */ \
		{
			dword *hit_points =
				GNU_extract_ini(mod, "troops.ini", "troop%d", "hp", 0, 128, NULL);
			return hit_points;
		}
		break;
		default: break;
	}

	if (image_name) {

		char *realname;

		realname = KB_fastpath(mod->slotA_name, "/", image_name, image_subid, image_suffix); 

		KB_debuglog(0, "? FREE IMG FILE: %s\n", realname);

#ifdef HAVE_LIBSDL_IMAGE
		SDL_Surface *surf = IMG_Load(realname);
		if (surf == NULL) KB_debuglog(0, "> FAILED TO OPEN, %s\n", IMG_GetError());
#else
		SDL_Surface *surf = SDL_LoadBMP(realname);
		if (surf == NULL) KB_debuglog(0, "> FAILED TO OPEN, %s\n", SDL_GetError());
#endif

		if (surf && is_transparent)
			SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);

		free(realname);
		return surf;
	}

	return NULL;
}
