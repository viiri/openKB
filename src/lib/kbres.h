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
 * PAL_ are arrays of 256 SDL_Colors.
 * DAT_ are byte arrays of pre-defined length.
 * STR_ are asiiz strings.
 * STRL_ are asiiz-lists (\0 acts as a separator, \0\0 ends the list).
 */
#define GR_LOGO 	0x00	/* subId - undefined */
#define GR_TITLE 	0x01	/* subId - undefined */

#define GR_TROOP	0x02	/* subId - troop index */
#define GR_TILE		0x03	/* subId - tile index */
#define GR_TILESET	0x04	/* subId - continent flavor */
#define GR_TILEROW	0x0B	/* subId - row index (1 row = 36 tiles) */
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

#define GR_GOLD 	0x25	/* subId - undefined */
#define GR_PURSE	0x26	/* subId - undefined */
#define GR_COIN		0x27	/* subId - coin index (0-3) */
#define GR_COINS	0x28	/* subId - undefined */
#define GR_PIECE 	0x29	/* subId - undefined */

#define GR_VIEW 	0x30	/* artifacts, maps, empty slot, empty map ; subId - undefined */
#define GR_ARTIFACTS 0x31	/* artifacts ; subId - undefined */
#define GR_MAPS 	0x32	/* filled maps; subId - undefined */
#define GR_INVSLOT	0x33	/* an empty inventory slot; subId - undefined */
#define GR_BLANKMAP	0x34	/* an empty map tile; subId - undefined */
#define GR_ARTIFACT	0x35	/* filled map; subId - artifact index (0-7) */
#define GR_ORBMAP	0x36	/* filled map; subId - continent index (0-3) */

#define GR_ENDING	0x40	/* subId - 0=won, 1=lost */
#define GR_ENDTILE	0x41	/* subId - 0=grass, 1=wall, 2=hero */
#define GR_ENDTILES	0x42	/* subId - undefined */

#define SN_TUNE		0x60	/* subId - tune index (0-10) */
#define PAL_PALETTE	0x70	/* subId - undefined */
#define COL_TEXT	0x71	/* textbox colorscheme ; subId - scheme type, see CS_ defines below */
#define COL_MINIMAP	0x72	/* subId - undefined */

#define DAT_WORLD	0x90	/* complete world map ; subId - undefined */
#define DAT_LAND	0x91	/* map for specific continent ; subId - continent index */

#define RECT_MAP	0xA0	/* SDL_Rect describing map area */
#define RECT_UI 	0xA1	/* SDL_Rect describing ui border area; subId - element index */
#define RECT_TILE 	0xA2	/* SDL_Rect describing one map tile */
#define RECT_UITILE	0xA3	/* SDL_Rect describing sidebar button */

#define STR_SIGN	0xE0	/* signpost text ; subId - signpost index */
#define STR_TROOP	0xE1	/* troop name ; subId - troop index */
#define STR_MULTI	0xE2	/* troops name ; subId - troop index */
#define STR_VNAME	0xE3	/* villain name ; subId - villain index */
#define STR_VDESC	0xE4	/* villain description line ; subId - line (villain index * 14) */
#define STR_CREDIT	0xE5	/* a line of credits ; subId - line */
#define STR_ENDING	0xE6	/* line of ending text ; subId - line, 100<=game won, >100=game lost */

#define STRL_SIGNS	0xF0	/* signpost texts ; subId - undefined */
#define STRL_TROOPS	0xF1	/* troop names ; subId - undefined */
#define STRL_MULTIS	0xF2	/* troops names ; subId - undefined */
#define STRL_VNAMES 0xF3	/* villains names ; subId - undefined */
#define STRL_VDESCS 0xF4	/* villains descriptions ; subId - villain id */
#define STRL_CREDITS 0xF5	/* credits ; subId - undefined */
#define STRL_ENDINGS 0xF6	/* ending text ; subId - 0=game won, 1=game lost */

/* Possible values for RECT_UI subid: */
#define FRAME_TOP   	0
#define FRAME_LEFT  	1
#define FRAME_RIGHT 	2
#define FRAME_BOTTOM	3
#define FRAME_MIDDLE	4

/* Refrence to EGA pallete, mostly used by DOS module. */
#define EGA_BLACK	0
#define EGA_DBLUE	1
#define EGA_DGREEN	2
#define EGA_DCYAN	3
#define EGA_DRED	4
#define EGA_MAGENTA	5
#define EGA_BROWN	6
#define EGA_GREY	7
#define EGA_DGREY	8
#define EGA_BLUE	9
#define EGA_GREEN	10
#define EGA_CYAN	11
#define EGA_RED 	12
#define EGA_VIOLET	13
#define EGA_YELLOW	14
#define EGA_WHITE	15
#define EGA_DVIOLET EGA_MAGENTA
#define EGA_DYELLOW EGA_BROWN

/* Possible values for COL_TEXT subid: */
#define CS_GENERIC	0
#define CS_STATUS_1	1
#define CS_STATUS_2	2
#define CS_STATUS_3	3
#define CS_STATUS_4	4
#define CS_STATUS_5	5
#define CS_TOPMENU	6
#define CS_CHROME	7
#define CS_MINIMENU	8

/* Color scheme for COL_TEXT: */
enum {
	COLOR_BACKGROUND,
	COLOR_TEXT1,
	COLOR_TEXT = COLOR_TEXT1,
	COLOR_TEXT2,
	COLOR_TEXT3,
	COLOR_TEXT4,
	COLOR_SHADOW1,
	COLOR_SHADOW = COLOR_SHADOW1,
	COLOR_SHADOW2,
	COLOR_FRAME1,
	COLOR_FRAME = COLOR_FRAME1,
	COLOR_FRAME2,

	COLOR_SEL_BACKGROUND,
	COLOR_SELECTION = COLOR_SEL_BACKGROUND, //Mark first used "SEL_" value
	COLOR_SEL_TEXT1,
	COLOR_SEL_TEXT = COLOR_SEL_TEXT1,
	COLOR_SEL_TEXT2,
	COLOR_SEL_TEXT3,
	COLOR_SEL_TEXT4,
	COLOR_SEL_SHADOW1,
	COLOR_SEL_SHADOW = COLOR_SEL_SHADOW1,
	COLOR_SEL_SHADOW2,
	COLOR_SEL_FRAME1,
	COLOR_SEL_FRAME2,

	COLORS_MAX
};

/* Tile indexes from DOS version. */
#define TILE_GRASS      	0
#define TILE_DEEP_WATER 	32
#define TILE_CASTLE     	0x85
#define TILE_TOWN       	0x8A
#define TILE_CHEST      	0x8B
#define TILE_DWELLING_1 	0x8C
#define TILE_DWELLING_2 	0x8D
#define TILE_DWELLING_3 	0x8E
#define TILE_DWELLING_4 	0x8F
#define TILE_TELECAVE   	TILE_DWELLING_3
#define TILE_SIGNPOST   	0x90
#define TILE_FOE        	0x91
#define TILE_ARTIFACT_1 	0x92
#define TILE_ARTIFACT_2 	0x93

#define IS_GRASS(M) 	((M) < 2 || (M) == 0x80)
#define IS_CASTLE(M)	((M) >= 0x02 && (M) <= 0x07)
#define IS_MAPOBJECT(M)	((M) >= 0x0a && (M) <= 0x13)
#define IS_WATER(M) 	((M) >= 0x14 && (M) <= 0x20)
#define IS_TREE(M)  	((M) >= 0x21 && (M) <= 0x2D)
#define IS_DESERT(M)	((M) >= 0x2e && (M) <= 0x3a)
#define IS_ROCK(M)  	((M) >= 0x3b && (M) <= 0x47)

#define IS_DEEP_WATER(M) ((M) == TILE_DEEP_WATER)

#define IS_INTERACTIVE(M) ((M) & 0x80)


#ifdef HAVE_LIBSDL
/* SDL flavor. */
#include <SDL.h>
#include "kbdir.h" // for KB_DIR
/* Provide usefull functions to modules */
inline SDL_Surface* SDL_CreatePALSurface(Uint32 width, Uint32 height);
extern void SDL_BlitXBPP(const char *src, SDL_Surface *dest, SDL_Rect *dstrect, int bpp);
extern void SDL_BlitMASK(const char *src, SDL_Surface *dest, SDL_Rect *dstrect);
extern void SDL_ReplaceIndex(SDL_Surface *dest, SDL_Rect *dstrect, byte search, byte replace);

/* Simple palette manipulation */
extern void put_mono_pal(SDL_Surface *dest);
extern void put_cga_pal(SDL_Surface *dest);
extern void put_ega_pal(SDL_Surface *dest);
extern void put_vga_pal(SDL_Surface *dest);
extern void put_color_pal(SDL_Surface *dest, Uint32 fore, Uint32 back);
extern Uint32 ega_pallete_rgb[16]; 


/* Provide useful functions to potential resource loader */
/*
 * DOS module
 */
extern void DOS_BlitRAWIMG(SDL_Surface *surf, SDL_Rect *destrect, const char *buf, byte bpp, word mask_pos);
extern void DOS_SetColors(SDL_Surface *surf, byte bpp);

extern char* DOS_villain_names[];
extern char* DOS_troop_names[];

extern SDL_Surface* DOS_LoadRAWCH_BUF(char *buf, int len);
extern SDL_Surface* DOS_LoadRAWIMG_BUF(char *buf, int len, byte bpp);
extern SDL_Surface* DOS_LoadIMGROW_RW(SDL_RWops *rw, word first, word frames); 

extern SDL_Surface* DOS_LoadIMGROW_DIR(KB_DIR *d, word first, word frames);

/* Laziest function */ 
SDL_Surface* KB_LoadIMG(const char *filename);

#endif
#endif /* _OPENKB_LIBKB_RESOURCES */
