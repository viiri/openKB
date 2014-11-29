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
#include "kbauto.h"	/* KB_fopen_with */

#include <SDL.h>  	/* SDL data types */
#ifdef HAVE_LIBSDL_IMAGE
#include <SDL_image.h>	/* PNG support */
#else
#warning "SDL_Image is not used, PNG support disabled!"
#endif


#define TILE_W 48
#define TILE_H 34

#define STRL_MAX 1024

byte* GNU_downto_byte(dword *src, int len, int freesrc) {
	int i;
	byte *b = malloc(sizeof(byte) * len);
	for (i = 0; i < len; i++) {
		b[i] = src[i];
	}
	if (freesrc) {
		free(src);
	}
	return b;
}
word* GNU_downto_word(dword *src, int len, int freesrc) {
	int i;
	word *b = malloc(sizeof(word) * len);
	for (i = 0; i < len; i++) {
		b[i] = src[i];
	}
	if (freesrc) {
		free(src);
	}
	return b;
}

char* GNU_string_ini(KBmodule *mod, const char *inifile, const char *module, const char *name, int first, int num, char *dst) {

	int len = num * 32;

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
		dst = malloc(sizeof(char) * len);
		if (dst == NULL) {
			free(filename);
			return NULL; /* Out of memory */
		}
		memset(dst, 0, sizeof(char) * len);
		KB_strlist_clear(dst);
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
	if (strpbrk(module, "%") != NULL) {
		module_test_len = sprintf(module_test, module_fmt, section + 1);
	} else {
		strcpy(module_test, module_fmt);
		module_test_len = strlen(module_test);
	}
	name_test_len = strlen(name);

	int filled = 0;
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
			char val[256];
			test_len = sprintf(test, "%s = %%[^\t\n;]", name);
			if (sscanf(line, test, val) == 1) {
				KB_strlist_append(dst, val);
				filled++;
			}
			if (filled >= num - first) break;
		}

	}

	KB_fclose(fd);
	free(filename);
	return dst;
}

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
	if (strpbrk(module, "%") != NULL) {
		module_test_len = sprintf(module_test, module_fmt, section + 1);
	} else {
		strcpy(module_test, module_fmt);
		module_test_len = strlen(module_test);
	}
	name_test_len = strlen(name);

	int filled = 0;
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
				filled++;
			} else {
				char buf[16];
				test_len = sprintf(test, "%s = #%%6c", name);
				if (sscanf(line, test, &buf[0]) == 1) {
					val = hex2dec(buf);
					dst[section - first] = val;
					filled++;
				}
			}
			if (filled >= num - first) break;
		}

	}

	KB_fclose(fd);
	free(filename);
	return dst;
}

Uint32* GNU_ReadTextColors(KBmodule *mod, const char *inifile, const char *section) {
	dword values[1] = { 0 };

	const char *names[] = { /* Should map to color scheme for COL_TEXT enum from kbres.h */
		"background",   	/* 0 */
		"text1",        	/* 1 */
		"text2",        	/* 2 */
		"text3",        	/* 3 */
		"text4",        	/* 4 */
		"shadow1",      	/* 5 */
		"shadow2",      	/* 6 */
		"frame1",       	/* 7 */
		"frame2",       	/* 8 */
		"sel_background",	/* 9 */
		"sel_text1",    	/* 10 */
		"sel_text2",    	/* 11 */
		"sel_text3",    	/* 12 */
		"sel_text4",    	/* 13 */
		"sel_shadow1",  	/* 14 */
		"sel_shadow2",  	/* 15 */
		"sel_frame1",   	/* 16 */
		"sel_frame2",   	/* 17 */
		/* maintaining this index is hell, we should DRY it out */
	};
	int i;

	Uint32 *colors = malloc(sizeof(Uint32) * COLORS_MAX);
	if (colors == NULL) return NULL;

	for (i = 0; i < COLORS_MAX; i++) {
		colors[i] = 0;
		if (GNU_extract_ini(mod, inifile, section, names[i], 0, 1, &values[0])) {
			colors[i] = values[0];
		}
	}
	return colors;
}

Uint32* GNU_ReadMinimapColors(KBmodule *mod, const char *inifile) {
	dword values[1] = { 0 };

	const char *names[] = { /* Should map to COL_MINIMAP special enum */
		"shallow_water",	/* 0 */
		"deep_water",   	/* 1 */
		"grass",        	/* 2 */
		"desert",       	/* 3 */
		"rock",         	/* 4 */
		"tree",         	/* 5 */
		"castle",       	/* 6 */
		"object",       	/* 7 */
	};

	int i;

	Uint32 *colors = malloc(sizeof(Uint32) * 8);
	if (colors == NULL) return NULL;

	for (i = 0; i < 8; i++) {
		GNU_extract_ini(mod, inifile, "minimap", names[i], 0, 1, &values[0]);
		colors[i] = values[0];
	}

	return colors;
}

Uint32* GNU_ReadDwellingColors(KBmodule *mod, const char *inifile) {
	dword values[1] = { 0 };

	const char *names[] = { /* Should map to COL_MINIMAP special enum */
		"dwelling0",	/* 0 */
		"dwelling1",   	/* 1 */
		"dwelling2",   	/* 2 */
		"dwelling3",   	/* 3 */
		"dwelling4"    	/* 4 */
	};

	int i;

	Uint32 *colors = malloc(sizeof(Uint32) * 5);
	if (colors == NULL) return NULL;

	for (i = 0; i < 5; i++) {
		GNU_extract_ini(mod, inifile, "dwellings", names[i], 0, 1, &values[0]);
		colors[i] = values[0];
	}

	return colors;
}

SDL_Rect* GNU_ReadRect(KBmodule *mod, const char *inifile, int which) {
	dword values[4] = { 0 };

	const char *names[] = { /* This doesn't map directly into any of the RECT_* defines :/ */
		"top",  	/* 0 */
		"left", 	/* 1 */
		"right",	/* 2 */
		"bottom",	/* 3 */
		"bar",  	/* 4 */
		"map",  	/* 5 */
		"tile", 	/* 6 */
		"uitile",	/* 7 */
		/* Another horrible index to maintain and watch out for :/ */
	};

	/* Just to make it clear: the "which" argument is magic number :( */
	const char *section = names[which];

	SDL_Rect *rect = malloc(sizeof(SDL_Rect));
	if (rect == NULL) return NULL;

	GNU_extract_ini(mod, "ui.ini", section, "x", 0, 1, &values[0]);
	GNU_extract_ini(mod, "ui.ini", section, "y", 0, 1, &values[1]);
	GNU_extract_ini(mod, "ui.ini", section, "w", 0, 1, &values[2]);
	GNU_extract_ini(mod, "ui.ini", section, "h", 0, 1, &values[3]);

	rect->x = values[0];
	rect->y = values[1];
	rect->w = values[2];
	rect->h = values[3];

/*	KB_debuglog(0, "Read RECT '%s' [%d,%d] - [%d,%d]\n"); */

	return rect;
}

char* GNU_read_textfile(KBmodule *mod, const char *textfile, int split) {

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
	if (buf[n] != '\n') {
		KB_debuglog(0, "Warning: no newline at the end of file\n");
	}
	buf[n] = '\0';

	/* Convert multi-line file to a strlist */
	if (split) {
		for (i = 0; i < n; i++)
			if (buf[i] == '\n') buf[i] = '\0';
		buf[n] = (char)0xFF;
	}

	KB_fclose(fd);
	free(filename);

	return buf;
}

void* GNU_Resolve(KBmodule *mod, int id, int sub_id) {

	char *image_name = NULL;
	char *image_suffix = NULL;
	char *image_subid = "";
	SDL_Rect image_cutout_rect;
	int image_cutout = 0;
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
		case GR_PORTRAIT:	/* subId - class */
		{
			if (sub_id < 0 || sub_id > 3) sub_id = 0;
			image_name = DOS_class_names[sub_id];
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_LOCATION:	/* subId - 0 home 1 town 2 - 6 dwelling */
		{
			if (sub_id < 0 || sub_id > 5) sub_id = 0;
			image_name = DOS_location_names[sub_id];
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
		case GR_TILE:	/* subId - tile index */
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
			image_cutout = 1;
			image_cutout_rect.x = sub_id * TILE_W;
			image_cutout_rect.y = 0;
			image_cutout_rect.w = TILE_W;
			image_cutout_rect.h = TILE_H;
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
		case GR_ENDING:
		{
			image_name = "endpic";
			image_suffix = _EXTN;
			image_subid = "-0";
			if (sub_id == 1) image_subid="-1";
		}
		break;
		case GR_ENDTILE:
		{
			image_name = "endpic";
			image_suffix = _EXTN;
			image_subid = "-2";
			if (sub_id == 1) image_subid="-3";
			if (sub_id == 2) image_subid="-4";
		}
		break;
		case STRL_ENDINGS:
		{
			if (sub_id)
				return GNU_read_textfile(mod, "endlose.txt", 1);
			else
				return GNU_read_textfile(mod, "endwin.txt", 1);
		}
		break;
		case STR_SIGN:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_SIGNS, 0), sub_id);
		}
		break;
		case STRL_SIGNS:
		{
			char *list = GNU_read_textfile(mod, "signs.txt", 1);
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
			return GNU_read_textfile(mod, "credits.txt", 0);
		}
		break;
		case STRL_VDESCS:	/* multiple lines of villain description */
		{
			char tmp[128];
			KB_strcpy(tmp, DOS_villain_names[sub_id]);
			KB_strcat(tmp, ".txt");
			return GNU_read_textfile(mod, tmp, 0);
		}
		break;
		case STRL_VNAMES:	/* villain names */
		{
			return GNU_string_ini(mod, "villains.ini", "villain%d", "name", 0, 17, NULL);
		}
		break;
		case STRL_TROOPS:	/* troop names */
		{
			return GNU_string_ini(mod, "troops.ini", "troop%d", "name", 0, 25, NULL);
		}
		break;
		case STRL_SPELLS:	/* spell names */
		{
			return GNU_string_ini(mod, "spells.ini", "spell%d", "name", 0, 14, NULL);
		}
		break;
		case STRL_CONTINENTS:	/* continent names */
		{
			return GNU_string_ini(mod, "land.ini", "continent%d", "name", 0, 4, NULL);
		}
		break;
		case STRL_CASTLES:	/* castle names */
		{
			return GNU_string_ini(mod, "castles.ini", "castle%d", "name", 0, 26, NULL);
		}
		break;
		case STRL_TOWNS:	/* town names */
		{
			return GNU_string_ini(mod, "towns.ini", "town%d", "name", 0, 26, NULL);
		}
		break;
		case STR_VNAME:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_VNAMES, 0), sub_id);
		}
		break;
		case STR_CASTLE:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_CASTLES, 0), sub_id);
		}
		break;
		case STR_TOWN:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_CASTLES, 0), sub_id);
		}
		break;
		case STR_TROOP:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_TROOPS, 0), sub_id);
		}
		break;
		case WDAT_VREWARD:
		{
			word *reward =
				GNU_downto_word(
					GNU_extract_ini(mod, "villains.ini", "villain%d", "reward", 0, 17, NULL)
					, 17, 1);
			return reward;
		}
		break;
		case WDAT_SCOST:
		{
			word *goldcost =
				GNU_downto_word(
					GNU_extract_ini(mod, "spells.ini", "spell%d", "gold", 0, 14, NULL)
					, 14, 1);
			return goldcost;
		}
		break;
		case DAT_SPECIALX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "special%d", "x", 0, 2, NULL)
					, 2, 1);
			return coord;
		}
		break;
		case DAT_SPECIALY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "special%d", "y", 0, 2, NULL)
					, 2, 1);
			return coord;
		}
		break;
		case DAT_SPECIALC:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "special%d", "continent", 0, 2, NULL)
					, 2, 1);
			return coord;
		}
		break;
		case DAT_CASTLEX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "castles.ini", "castle%d", "x", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_CASTLEY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "castles.ini", "castle%d", "y", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_CASTLEC:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "castles.ini", "castle%d", "continent", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_TOWNY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "y", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_TOWNX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "x", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_TOWNINV:
		{
			byte *inversion =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "castle_id", 0, 26, NULL)
					, 26, 1);
			return inversion;
		}
		break;
		case DAT_BOATY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "boat_y", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_BOATX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "boat_x", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_NAVY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "continent%d", "nav_y", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_NAVX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "continent%d", "nav_x", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_HPS:  	/* [MAX_TROOPS] hit points for specific troop; subId - undefined */ \
		{
			byte *hp =
				GNU_downto_byte(
					GNU_extract_ini(mod, "troops.ini", "troop%d", "hp", 0, 25, NULL)
					, 25, 1);
			return hp;
		}
		break;
		case DAT_WORLD:
		{
			KB_File *f;
			int n;
			byte *world;
			int len = 64 * 64 * 4;
			world = malloc(sizeof(byte) * len);
			if (!world) return NULL;
			f = KB_fopen_with("land.org", "rb", mod);
			if (f == NULL) return NULL;
			n = KB_fread(world, sizeof(byte), len, f);
			KB_fclose(f);
			return world;
		}
		break;
		case RECT_UI:
		{
			if (sub_id < 0 || sub_id > 4) return NULL;
			return GNU_ReadRect(mod, "ui.ini", sub_id);
		}
		case RECT_MAP:
		{
			return GNU_ReadRect(mod, "ui.ini", 5);
		}
		break;
		case RECT_TILE:
		{
			return GNU_ReadRect(mod, "ui.ini", 6);
		}
		break;
		case RECT_UITILE:
		{
			return GNU_ReadRect(mod, "ui.ini", 7);
		}
		break;
		case COL_TEXT:
		{
			const char *CS_names[] = { /* Index is one of CS_ defines from kbres.h */
				"generic",  /* CS_GENERIC  == 0 */
				"status1",  /* CS_STATUS_1 == 1 */
				"status2",  /* CS_STATUS_2 == 2 */
				"status3",  /* CS_STATUS_3 == 3 */
				"status4",  /* CS_STATUS_4 == 4 */
				"status5",  /* CS_STATUS_5 == 5 */
				"submenu",  /* CS_MINIMENU == 6 */
				"character",/* CS_VIEWCHAR == 7 */
				"army",     /* CS_VIEWARMY == 8 */
				"chrome",   /* CS_CHROME   == 9 */
				"ending",   /* CS_ENDING   == 10 */
			};
			if (sub_id < 0 || sub_id > 9) return NULL;
			return GNU_ReadTextColors(mod, "colors.ini", CS_names[sub_id]);
		}
		break;
		case COL_DWELLING:
		{
			return GNU_ReadDwellingColors(mod, "colors.ini");
		}
		break;
		case COL_MINIMAP:
		{
			return GNU_ReadMinimapColors(mod, "colors.ini");
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
		if (image_cutout) {
			SDL_Surface *piece = SDL_CreatePALSurface(image_cutout_rect.w, image_cutout_rect.h);
			SDL_ClonePalette(piece, surf);
			SDL_BlitSurface(surf, &image_cutout_rect, piece, NULL);
			SDL_FreeSurface(surf);
			surf = piece;
		}

		if (surf && is_transparent)
			SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);

		free(realname);
		return surf;
	}

	return NULL;
}
