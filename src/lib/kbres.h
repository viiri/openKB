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
 * SN_ resources are sounds in KBsound format
 * RECT_ are single SDL_Rects.
 * PAL_ are arrays of 256 SDL_Colors.
 * COL_ are arrays of Uint32 colors in 0xAARRGGBB format;
 * DAT_ are byte arrays of pre-defined length.
 * WDAT_ are word arrays of pre-defined length.
 * STR_ are asiiz strings.
 * STRL_ are asiiz-lists (\0 acts as a separator, \0\0 ends the list).
 */
#define RESOURCES \
	_(GR_LOGO)  	/* "company logo" ; subId - undefined */ \
	_(GR_TITLE) 	/* game title ; subId - undefined */ \
	_(GR_TROOP) 	/* troop spritesheet ; subId - troop index */ \
	_(GR_TILE)  	/* single map tile ; subId - tile index */ \
	_(GR_TILESET)	/* while map tileset ; subId - continent flavor */ \
	_(GR_TILEROW)	/* a row of tiles ; subId - row index (1 row = 36 tiles) */ \
	_(GR_TILESALT)	/* continent flavor tiles ; subId - 0=full; 1-3=continent */ \
	_(GR_VILLAIN)	/* villain portrait ; subId - villain index */ \
\
	_(GR_FACE)  	/* ??? ; subId - face index (villain index * 4) */ \
	_(GR_FACES) 	/* ??? ; subId - undefined */ \
\
	_(GR_COMTILE)	/* combat map tile ; subId - combat tile index */ \
	_(GR_COMTILES)	/* combat map tileset; subId - undefined */ \
\
	_(GR_LOCATION)	/* location background ; subId - 0=home 1=town 2-6=dwelling */ \
\
	_(GR_FONT)  	/* bitmap font ; subId - undefined */ \
\
	_(GR_CURSOR)	/* hero spritesheet + UI elements ; subId - undefined */ \
\
	_(GR_UI)    	/* UI elements ; subId - undefined */ \
	_(GR_SELECT)	/* title screen + misc elements ; subId - undefined */ \
\
	_(GR_PORTRAIT)	/* player portrait ; subId - player class */ \
\
	_(GR_GOLD)  	/* ??? ; subId - undefined */ \
	_(GR_PURSE) 	/* ??? ; subId - undefined */ \
	_(GR_COIN)  	/* coin ; subId - coin index (0-3) */ \
	_(GR_COINS) 	/* coins ; subId - undefined */ \
	_(GR_PIECE) 	/* puzzle piece ; subId - undefined */ \
\
	_(GR_VIEW)  	/* artifacts, maps, empty slot, empty map ; subId - undefined */ \
	_(GR_ARTIFACTS)	/* artifacts ; subId - undefined */ \
	_(GR_MAPS)  	/* filled maps ; subId - undefined */ \
	_(GR_INVSLOT)	/* an empty inventory slot; subId - undefined */ \
	_(GR_BLANKMAP)	/* an empty map tile ; subId - undefined */ \
	_(GR_ARTIFACT)	/* artifact ; subId - artifact index (0-7) */ \
	_(GR_ORBMAP)	/* filled map ; subId - continent index (0-3) */ \
\
	_(GR_ENDING)	/* endscreen background ; subId - 0=won, 1=lost */ \
	_(GR_ENDTILE)	/* endscreen tile ; subId - 0=grass, 1=wall, 2=hero */ \
	_(GR_ENDTILES)	/* endscreen tileset ; subId - undefined */ \
\
	_(SN_TUNE)  	/* subId - tune index (0-10) */ \
	_(PAL_PALETTE)	/* subId - undefined */ \
	_(COL_TEXT) 	/* textbox colorscheme ; subId - scheme type, see CS_ defines below */ \
	_(COL_MINIMAP) 	/* minimap colorscheme ; subId - undefined */ \
	_(COL_DWELLING)	/* dwellings colorscheme ; subId - undefined */ \
\
	_(DAT_WORLD)	/* complete world map ; subId - undefined */ \
	_(DAT_LAND) 	/* map for specific continent ; subId - continent index */ \
	_(WDAT_SGOLD)	/* [4] starting gold for specific class; subId - undefined */ \
	_(WDAT_STROOP)	/* [2] starting troop type; subId - class */ \
	_(WDAT_SNUMBER)	/* [2] starting troop count; subId - class */ \
	_(WDAT_COMM)	/* [4] commissions for specific rank; subId - class */ \
	_(WDAT_LDRSHIP)	/* [4] leaderships for specific rank; subId - class */ \
	_(DAT_VNEED)	/* [4] villains needed for specific rank; subId - class */ \
	_(DAT_KMAGIC)	/* [4] "knows magic" for specific rank; subId - class */ \
	_(DAT_SPOWER)	/* [4] "spell power" for specific rank; subId - class */ \
	_(DAT_MAXSPELL)	/* [4] "max spells" for specific rank; subId - class */ \
	_(DAT_FAMILIAR)	/* [4] "instant army familiar" for specific rank; subId - class */ \
	_(DAT_SKILLS)	/* [MAX_TROOPS] skill levels for specific troop; subId - undefined */ \
	_(DAT_MOVES)	/* [MAX_TROOPS] move rates for specific troop; subId - undefined */ \
	_(DAT_MELEEMIN)	/* [MAX_TROOPS] melee min for specific troop; subId - undefined */ \
	_(DAT_MELEEMAX)	/* [MAX_TROOPS] melee max for specific troop; subId - undefined */ \
	_(DAT_RANGEMIN)	/* [MAX_TROOPS] ranged min for specific troop; subId - undefined */ \
	_(DAT_RANGEMAX)	/* [MAX_TROOPS] ranged max for specific troop; subId - undefined */ \
	_(DAT_SHOTAMMO)	/* [MAX_TROOPS] ranged ammo for specific troop; subId - undefined */ \
	_(DAT_HPS)  	/* [MAX_TROOPS] hit points for specific troop; subId - undefined */ \
	_(DAT_GCOST)	/* [MAX_TROOPS] gold cost for specific troop; subId - undefined */ \
	_(DAT_SPOILS)	/* [MAX_TROOPS] spoils factor for specific troop; subId - undefined */ \
	_(DAT_DWELLS)	/* [MAX_TROOPS] dwelling index for specific troop; subId - undefined */ \
	_(DAT_MGROUP)	/* [MAX_TROOPS] morale group for specific troop; subId - undefined */ \
	_(DAT_MAXPOP)	/* [MAX_TROOPS] max population for specific troop; subId - undefined */ \
	_(DAT_GROWTH)	/* [MAX_TROOPS] growth factor for specific troop; subId - undefined */ \
	_(DAT_ABILS)	/* [MAX_TROOPS] abilitiy flags for specific troop; subId - undefined */ \
\
	_(RECT_MAP) 	/* SDL_Rect describing map area */ \
	_(RECT_UI)  	/* SDL_Rect describing ui border area; subId - element index */ \
	_(RECT_TILE)	/* SDL_Rect describing one map tile */ \
	_(RECT_UITILE)	/* SDL_Rect describing sidebar button */ \
\
	_(STR_SIGN) 	/* signpost text ; subId - signpost index */ \
	_(STR_TROOP)	/* troop name ; subId - troop index */ \
	_(STR_MULTI)	/* troops name ; subId - troop index */ \
	_(STR_RANK) 	/* rank name ; subId - rank + (class index * 4) */ \
	_(STR_VNAME)	/* villain name ; subId - villain index */ \
	_(STR_VDESC)	/* villain description line ; subId - line (villain index * 14) */ \
	_(STR_ANAME)	/* artifact name ; subId - artifact index */ \
	_(STR_ADESC)	/* artifact description line ; subId - line (artifact index * 5) */ \
	_(STR_CREDIT)	/* a line of credits ; subId - line */ \
	_(STR_ENDING)	/* line of ending text ; subId - line, 100<=game won, >100=game lost */ \
\
	_(STRL_SIGNS)	/* signpost texts ; subId - undefined */ \
	_(STRL_TROOPS)	/* troop names ; subId - undefined */ \
	_(STRL_MULTIS)	/* troops names ; subId - undefined */ \
	_(STRL_RANKS)	/* ranks names ; subId - class id */ \
	_(STRL_VNAMES)	/* villains names ; subId - undefined */ \
	_(STRL_VDESCS)	/* villains descriptions ; subId - villain id */ \
	_(STRL_ANAMES)	/* artifact names ; subId - undefined */ \
	_(STRL_ADESCS)	/* artifact descriptions ; subId - artifact id */ \
	_(STRL_CREDITS)	/* credits ; subId - undefined */ \
	_(STRL_ENDINGS)	/* ending text ; subId - 0=game won, 1=game lost */

#define _(R) R,
enum { 
	RESOURCES 
	GR_HERO = GR_CURSOR,
	FIRST_STR = STR_SIGN,
	FIRST_STRL = STRL_SIGNS,
};
#undef _

extern const char *KBresid_names[]; 

/* Possible values for RECT_UI subid: */
#define FRAME_TOP   	0
#define FRAME_LEFT  	1
#define FRAME_RIGHT 	2
#define FRAME_BOTTOM	3
#define FRAME_MIDDLE	4

/* Possible values for SN_TUNE subid: */
#define TUNE_WALK	0
#define TUNE_BUMP	1
#define TUNE_CHEST	5

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
#define CS_VIEWCHAR	9

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
#include "kbconf.h" // for KBresolve_cb
/* Provide usefull functions to modules */
inline SDL_Surface* SDL_CreatePALSurface(Uint32 width, Uint32 height);
inline void SDL_ClonePalette(SDL_Surface *dst, SDL_Surface *src);
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

/* Tileset factories (make single SDL_Surface from multiple ones) */
SDL_Surface* KB_LoadTileset_TILES(SDL_Rect *tilesize, KBresolve_cb resolve, KBmodule *mod);
SDL_Surface* KB_LoadTileset_ROWS(SDL_Rect *tilesize, KBresolve_cb resolve, KBmodule *mod);
SDL_Surface* KB_LoadTilesetSalted(byte continent, KBresolve_cb resolve, KBmodule *mod);

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

extern SDL_Surface* DOS_LoadRAWIMG_RW(SDL_RWops *rw, byte bpp);
extern SDL_Surface* DOS_LoadIMGROW_DIR(KB_DIR *d, word first, word frames);

/* Laziest function */ 
SDL_Surface* KB_LoadIMG(const char *filename);

#endif
#endif /* _OPENKB_LIBKB_RESOURCES */
