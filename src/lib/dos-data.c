#include "kbauto.h"
#include "kbconf.h"
#include "kbfile.h"
#include "kbres.h"

#define DATA_SEGMENT 0x15850

#define MCGA_PALETTE_OFFSET	0x032D

#define DOS_TILE_W 48
#define DOS_TILE_H 34

typedef struct DOS_Cache {

	SDL_Color *vga_palette;

} DOS_Cache;

SDL_Color * DOS_ReadPalette_RW(SDL_RWops *rw) {
	SDL_Color *pal;
	KB_File *f;

	char buf[768], *p;
	int i, n;

	SDL_RWseek(rw, MCGA_PALETTE_OFFSET, 0);

	n = SDL_RWread(rw, &buf[0], sizeof(char), 768);
	if (n != 768) return NULL;

	pal = malloc(sizeof(SDL_Color) * 256);
	if (pal == NULL) return NULL;

	p = &buf[0];
	for (i = 0; i < 256; i++) {
		byte vga_r = READ_BYTE(p);
		byte vga_g = READ_BYTE(p);
		byte vga_b = READ_BYTE(p);
		/* 6-bit VGA to 8-bit RGB: */
		pal[i].r = (vga_r * 255) / 63;
		pal[i].g = (vga_g * 255) / 63;
		pal[i].b = (vga_b * 255) / 63;
	}
	return pal;
}

SDL_Color * DOS_ReadPalette_FD(KB_File *f) {
	SDL_Color *pal = NULL;
	SDL_RWops *rw = KBRW_open(f);
	pal = DOS_ReadPalette_RW(rw);
	KBRW_close(rw);
	return pal;
}

SDL_Color * DOS_ReadPalette(KBmodule *mod, const char *filename) {
	return DOS_ReadPalette_FD(KB_fopen_with(filename, "rb", mod));
}

void DOS_SetPalette(KBmodule *mod, SDL_Surface *dst, int bpp) {

	DOS_Cache *ch = mod->cache;
	if (ch) {
		SDL_Color *pal = ch->vga_palette;

		if (pal) {

			SDL_SetColors(dst, pal, 0, 256);
	
			return;
		}

	}

	DOS_SetColors(dst, bpp);

}


#if 0
/* Another method of loading strings -- by appropriate offsets into DATA_SEGMENT */
char* DOS_read_string_p(KBmodule *mod, int ptroff, int off, int endoff) {
	static char buf[24096];
	static char pbuf[24096];
	
	int segment = DATA_SEGMENT;

	int len = endoff - off;

	KB_File *f;	int n;   

	KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
	f = KB_fopen_with("kb.exe", "rb", mod);
	if (f == NULL) return NULL;

	KB_fseek(f, ptroff, 0);
	n = KB_fread(&pbuf[0], sizeof(char), len, f);

	KB_fseek(f, off, 0);
	n = KB_fread(&buf[0], sizeof(char), len, f);
	KB_fclose(f);

	char *p = &pbuf[0];

	int i;
	for (i = 0; i < 17; i++) {
		word soff;
		soff = READ_WORD(p);
		KB_debuglog(0, "%02x [%04x - %08x] Splitting: '%s' %c %d \n", i, soff, soff + segment, &buf[soff + segment - off]);
		//KB_fseek(f, segment + soff, 0);
	}

	return &buf[0];
}
#endif

char* DOS_read_strings(KBmodule *mod, int off, int endoff) {
	char *buf;

	int len = endoff - off;
	
	KB_File *f;	
	int n;

	buf = malloc(sizeof(char) * len);
	if (buf == NULL) return NULL;

	KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
	f = KB_fopen_with("kb.exe", "rb", mod);
	if (f == NULL) {
		free(buf);
		return NULL;
	}

	KB_fseek(f, off, 0);
	n = KB_fread(buf, sizeof(char), len, f);
	KB_fclose(f);
	if (n < len) {
		free(buf);
		return NULL;
	}

	return buf;
}

void DOS_compact_strings(KBmodule *mod, char *buf, int len) {
	int i;
	for (i = 0; i < len - 1; i++)
		if (buf[i] == '\0') buf[i] = '\n';
}

char *DOS_troop_names[] = {
    "peas","spri","mili","wolf","skel","zomb","gnom","orcs","arcr","elfs",
    "pike","noma","dwar","ghos","kght","ogre","brbn","trol","cavl","drui",
    "arcm","vamp","gian","demo","drag",
};

char *DOS_villain_names[] = {
    "mury","hack","ammi","baro","drea","cane","mora","barr","barg","rina",
    "ragf","mahk","auri","czar","magu","urth","arec",
};

char *DOS_class_names[] = {
	"knig","pala","sorc","barb"
};

char *DOS_location_names[] = {
	"cstl","town","plai","frst","cave","dngn"
};

/* Border frame sizes in DOS layout, in pixels */
SDL_Rect DOS_frame_ui[6] = {
	{	0, 0, 320, 8 	}, /* Top */
	{	0, 0, 16, 200	}, /* Left */
	{	300, 0, 16, 200	}, /* Right */ 
	{	0, 0, 320, 8	}, /* Bottom */
	{	0, 17, 280, 5	}, /* Bar */
};

/* 'map viewscreen' position in DOS layout, in pixels */
SDL_Rect DOS_frame_map =
	{	16, 14 + 7, DOS_TILE_W * 5, DOS_TILE_H * 5	};

SDL_Rect DOS_frame_tile =	/* Size of one tile */
	{	0, 0, DOS_TILE_W, DOS_TILE_H };


void DOS_Init(KBmodule *mod) {

	DOS_Cache *cache = malloc(sizeof(DOS_Cache));

	if (cache) { 
		/* Init */
		cache->vga_palette = NULL;
		/* Set */
		mod->cache = cache;
		/* Pre-cache some things */
		if (mod->bpp == 8) {
			cache->vga_palette = DOS_ReadPalette(mod, "MCGA.DRV");
		}
	}

}

void DOS_Stop(KBmodule *mod) {

	DOS_Cache *cache = mod->cache;
	
	free(cache->vga_palette);
	free(mod->cache);

}

/* Whatever static resources need run-time initialization - do it here */
void DOS_PrepareStatic(KBmodule *mod) {


}


void* DOS_Resolve(KBmodule *mod, int id, int sub_id) {

	char *middle_name;
	char *suffix;
	char *ident;

	int row_start = 0;
	int row_frames = 4;

	enum {
		UNKNOWN,
		RAW_IMG,
		RAW_CH,
		IMG_ROW,
	} method = UNKNOWN;

	char *bpp_names[] = {
		NULL, ".4", ".4",//0, 1, 2
		NULL, ".16",	//3, 4
		NULL, NULL, 	//5,6
		NULL, ".256",	//7, 8
	};

	middle_name = suffix = ident = NULL;

	switch (id) {
		case GR_LOGO:
		{
			/* NWCP logo */
			method = RAW_IMG;
			middle_name = "nwcp";
			suffix = bpp_names[mod->bpp];
			ident = "#0";
		}
		break;
		case GR_TITLE:
		{
			/* KB title screen */
			method = RAW_IMG;
			middle_name = "title";
			suffix = bpp_names[mod->bpp];
			ident = "#0";
		}
		break;
		case GR_SELECT:
		{
			/* Character select screen */
			method = RAW_IMG;
			middle_name = "select";
			suffix = bpp_names[mod->bpp];
			ident = "#0";
						char buffl[8];
			sprintf(buffl, "#%d", sub_id);
			ident = &buffl[0];
		}
		break;
		case GR_ENDING:	/* subId - 0=won, 1=lost */
		{
			/* Ending screen */
			method = RAW_IMG;
			middle_name = "endpic";
			suffix = bpp_names[mod->bpp];
			if (sub_id) {
				ident = "#1";
			} else {
				ident = "#0";
			}
		}
		break;
		case GR_ENDTILE:	/* subId - 0=grass, 1=wall, 2=hero */
		{
			/* Ending tile */
			method = RAW_IMG;
			middle_name = "endpic";
			suffix = bpp_names[mod->bpp];
			char buffl[8];
			sprintf(buffl, "#%d", sub_id - 2);
			ident = &buffl[0];
		}
		break;
		case GR_ENDTILES:
		{
			/* Ending tiles */
			method = IMG_ROW;
			middle_name = "endpic";
			suffix = bpp_names[mod->bpp];
			ident = "";
			row_start = 2;
			row_frames = 3;
		}
		break;
		case GR_FONT:
		{
			/* KB font */
			method = RAW_CH;
			middle_name = "KB";
			suffix = ".CH";
			ident = "";
		}
		break;
		case GR_TROOP:	/* subId - troop index */
		{
			/* A troop (with animation) */
			method = IMG_ROW;
			middle_name = DOS_troop_names[sub_id];
			suffix = bpp_names[mod->bpp];
			ident = "";
		}
		break;
		case GR_TILE:	/* subId - tile index */
		{
			/* A tile */
			method = RAW_IMG;
			if (sub_id > 35) {
				sub_id -= 36;
				middle_name = "tilesetb";
			} else {
				middle_name = "tileseta";
			}
			suffix = bpp_names[mod->bpp];
			char buffl[8];
			sprintf(buffl, "#%d", sub_id);
			ident = &buffl[0];
		}
		break;
		case GR_TILESET:	/* subId - continent */
		{
#define SDL_ClonePalette(DST, SRC) SDL_SetPalette((DST), SDL_LOGPAL | SDL_PHYSPAL, (SRC)->format->palette->colors, 0, (SRC)->format->palette->ncolors)
			/* This one must be assembled */
			int i;
			SDL_Rect dst = { 0, 0, DOS_TILE_W, DOS_TILE_H };
			SDL_Rect src = { 0, 0, DOS_TILE_W, DOS_TILE_H };
			SDL_Surface *ts = SDL_CreatePALSurface(8 * dst.w, 70/7 * dst.h);
			for (i = 0; i < 72; i++) {
				SDL_Surface *tile = DOS_Resolve(mod, GR_TILE, i);
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
		break;
		case GR_CURSOR:	/* subId - undefined */
		{
			/* Hero sprite */
			method = IMG_ROW;
			middle_name = "cursor";
			suffix = bpp_names[mod->bpp];
			ident = "";
			row_frames = 16;
		}
		break;
		case GR_UI:	/* subId - undefined */
		{
			/* Sidebar tiles */
			method = IMG_ROW;
			middle_name = "cursor";
			suffix = bpp_names[mod->bpp];
			ident = "";
			row_start = 12;
			row_frames = 13;
		}
		break;
		case GR_PURSE:	/* subId - undefined */
		{
			/* Single sidebar tile (gold purse) */
			method = RAW_IMG;
			middle_name = "cursor";
			suffix = bpp_names[mod->bpp];
			ident = "#13";
		}
		break;
		case GR_COINS:
		{
			/* coins */
			method = IMG_ROW;
			middle_name = "cursor";
			suffix = bpp_names[mod->bpp];
			ident = "";
			row_start = 25;
			row_frames = 3;
		}
		break;
		case GR_COIN:	/* subId - coin index */
		{
			/* one coin */
			method = RAW_IMG;
			middle_name = "cursor";
			suffix = bpp_names[mod->bpp];
			ident = "#0";
			if (sub_id == 1) ident = "#1";
			if (sub_id == 2) ident = "#2";
		}
		break;
		case GR_PIECE:
		{
			SDL_Surface *piece = SDL_CreatePALSurface(9, 6);
			SDL_Rect mrect = { 0, 0, 9, 6 };
			/* Black (color "0") outline */
			SDL_FillRect(piece, &mrect, 0);
			mrect.w -= 1; mrect.h -= 1;
			/* Use color "4" (red) for EGA and above, color "2" (magenta) for CGA and below */
			SDL_FillRect(piece, &mrect, (mod->bpp < 3 ? 2 : 4)); 
			/* Use generic palette :( */
			DOS_SetColors(piece, mod->bpp);
			return piece;
		}
		break;
		case GR_VILLAIN:	/* subId - villain index */
		{
			/* A villain (with animation) */
			method = IMG_ROW;
			middle_name = DOS_villain_names[sub_id];
			suffix = bpp_names[mod->bpp];
			ident = "";
		}
		break;
		case GR_COMTILES:	/* subId - undefined */
		{
			/* Combat tileset */
			method = IMG_ROW;
			middle_name = "comtiles";
			suffix = bpp_names[mod->bpp];
			ident = "";	
			row_frames = 15;
		}
		break;
		case GR_VIEW:	/* subId - undefined */
		{
			/* Character sheet tiles */
			method = IMG_ROW;
			middle_name = "view";
			suffix = bpp_names[mod->bpp];
			ident = "";
			row_start = 0;
			row_frames = 14;
		}
		break;
		case GR_PORTRAIT:	/* subId - class */
		{
			method = RAW_IMG;
			suffix = bpp_names[mod->bpp];
			ident = "#0";
			if (sub_id < 0 || sub_id > 3) sub_id = 0;
			middle_name = DOS_class_names[sub_id];
		}
		break;
		case GR_LOCATION:	/* subId - 0 home 1 town 2 - 6 dwelling */
		{
			method = RAW_IMG;
			suffix = bpp_names[mod->bpp];
			ident = "#0";
			if (sub_id < 0 || sub_id > 5) sub_id = 0;
			middle_name = DOS_location_names[sub_id];
		}
		break;
		case COL_MINIMAP:
		{
			enum {
				SHALLOW_WATER,
				DEEP_WATER,
				GRASS,
				DESERT,
				ROCK,
				TREE,
				CASTLE,
				MAP_OBJECT,
			} tile_type;
			Uint32 ega_minimap_index[8] = {
				0x0000FF,
				0x0000AA,
				0x00FF00,
				0xFFFF55,
				0xAA5500,
				0x00AA00,
				0xFFFFFF,
				0xFF0000,
			};
			static Uint32 colors[255];

			byte tile;
			for (tile = 0; tile < 255; tile++) {
#define IS_GRASS(M) ((M) < 2 || (M) == 0x80)
#define IS_WATER(M) ((M) >= 0x14 && (M) <= 0x20)
#define IS_DESERT(M) ((M) >= 0x2e && (M) <= 0x3a)
#define IS_INTERACTIVE(M) ((M) & 0x80)

#define IS_ROCK(M) ((M) >= 59 && (M) <= 71)
#define IS_TREE(M) ((M) >= 33 && (M) <= 45)
#define IS_CASTLE(M) ((M) >= 0x02 && (M) <= 0x07)
#define IS_MAPOBJECT(M) ((M) >= 10 && (M) <= 19)
#define IS_DEEP_WATER(M) ((M) == 32)

				if ( IS_GRASS(tile) ) tile_type = GRASS;
				else if ( IS_DEEP_WATER(tile) ) tile_type = DEEP_WATER;
				else if ( IS_WATER(tile) ) tile_type = SHALLOW_WATER;
				else if ( IS_DESERT(tile) ) tile_type = DESERT;
				else if ( IS_ROCK(tile) ) tile_type = ROCK;
				else if ( IS_TREE(tile) ) tile_type = TREE;
				else if ( IS_CASTLE(tile) ) tile_type = CASTLE;
				else if ( IS_MAPOBJECT(tile) || IS_INTERACTIVE(tile)) tile_type = MAP_OBJECT;

				colors[tile] = ega_minimap_index[tile_type];
			}
			return &colors;
		}
		break;
		case COL_TEXT:
		{
			static Uint32 colors[16] = {
				0xAA0000, 0xFFFFFF,
				0xAA0000, 0xFFFFFF,
			};
			return &colors; 
		}	
		break;
		case STR_SIGN:
		{
			char *list = DOS_Resolve(mod, STRL_SIGNS, 0);
			char *rem = list;
			int i = 0;
			while (1) {
				if (*list == '\0') {
					if (i == sub_id) break;
					i++;
					list++;
					if (*list == '\0') break;
					rem = list;
					continue;
				}
				list++;				
			}
			return rem;
		}
		break;
		case STRL_SIGNS:
		{
			char *buf = DOS_read_strings(mod, 0x191CB, 0x19815);
			//int ptroff = 0x1844D;
			int len = 0x19815 - 0x191CB;
			/* Signs need some extra work */
			int i, j = 1;
			for (i = 0; i < len - 1; i++)  {
				if (buf[i] == '\0') {
					if (j) buf[i] = '\n';
					j= 1 - j;
				}
			}

			return buf;
		}
		break;
		case STR_VNAME:
		{
			return KB_strlist_ind(DOS_Resolve(mod, STRL_VNAMES, 0), sub_id);
		}
		break;
		case STRL_VNAMES:
		{
			return DOS_read_strings(mod, 0x190A5, 0x191cb);
			//int ptroff = 0x1842C;
		}
		break;
		case STR_VDESC:
		{
			int villain = sub_id / 13;
			int line = sub_id - (villain * 13);
			return KB_strlist_ind(DOS_Resolve(mod, STRL_VDESCS, 0), sub_id);
		}
		break;
		case STRL_VDESCS:
		{
			return DOS_read_strings(mod, 0x16edf, 0x18427);
			//int ptroff = 0x16bf4;//17 * 13
		}
		break;
		case STR_CREDIT:
		{
			return KB_strlist_ind(DOS_Resolve(mod, STRL_CREDITS, 0), sub_id);
		}
		break;
		case STRL_CREDITS:
		{
			return DOS_read_strings(mod, 0x16031, 0x160FF);
		}
		break;
		case STR_ENDING: /* subId - string index (indexes above 100 indicate next group) */
		{
			/* A small string containing part of the ending message. */
			if (sub_id > 100)	/* Game lost */
				return KB_strlist_ind(DOS_Resolve(mod, STRL_ENDINGS, 1), sub_id - 100);
			else            	/* Game won */
				return KB_strlist_ind(DOS_Resolve(mod, STRL_ENDINGS, 0), sub_id);
		}
		break;
		case STRL_ENDINGS:
		{
			/* Returns large buffer of \0-separated strings, forming an ending text */
			if (sub_id) /* Game lost */
				return DOS_read_strings(mod, 0x1B0E3, 0x1B212);
			else    	/* Game won */
				return DOS_read_strings(mod, 0x1AFB4, 0x1B0E2);
		}
		break;
		case RECT_MAP:
		{
			return &DOS_frame_map;
		}
		break;
		case RECT_TILE:
		{
			return &DOS_frame_tile;
		}
		break;
		case RECT_UI:
		{
			if (sub_id < 0 || sub_id > 4) sub_id = 0;
			return &DOS_frame_ui[sub_id];
		}
		case RECT_UITILE:
		{
			return &DOS_frame_tile;
		}
		break;
		break;
		default: break;
	}

	switch (method) {
		case IMG_ROW:
		case RAW_IMG:
		case RAW_CH:
		{
			SDL_Surface *surf = NULL;
			KB_File *f;
			char realname[1024];
			char buf[0xFA00];
			int n;
			
			int bpp = mod->bpp;
			
			if (bpp == 1) bpp = 2;

			realname[0] = '\0';
			KB_strcat(realname, middle_name);
			KB_strcat(realname, suffix);
			KB_strcat(realname, ident);

			switch (method) {
				case RAW_IMG:
				case RAW_CH:
				{
					KB_debuglog(0,"? DOS IMG FILE: %s\n", realname);
					f = KB_fopen_with(realname, "rb", mod);
					if (f == NULL) return NULL;
					n = KB_fread(&buf[0], sizeof(char), 0xFA00, f);
					KB_fclose(f);

					if (method == RAW_IMG) {
						surf = DOS_LoadRAWIMG_BUF(&buf[0], n, bpp);
						//if (mod->bpp == 1) DOS_SetColors(surf, 1);
						DOS_SetPalette(mod, surf, mod->bpp);
					}
					else
						surf = DOS_LoadRAWCH_BUF(&buf[0], n);
				}
				break;
				case IMG_ROW:	
				{
					KB_debuglog(0,"? DOS IMG DIR: %s\n", realname);
					KB_DIR *d = KB_opendir_with(realname, mod);

					if (d == NULL) return NULL;				

					surf = DOS_LoadIMGROW_DIR(d, row_start, row_frames);
					//if (mod->bpp == 1) DOS_SetColors(surf, 1);	
					DOS_SetPalette(mod, surf, mod->bpp);
					
					KB_closedir(d);
				}
				break;
				default: break;
			}

			return surf;
		}
		break;
		default: break;
	}
	return NULL;
}

