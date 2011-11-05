#include "kbconf.h"
#include "kbfile.h"
#include "kbres.h"

#define DATA_SEGMENT 0x15850

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

char *DOS_troop_names[] = {
    "peas","spri","mili","wolf","skel","zomb","gnom","orcs","arcr","elfs",
    "pike","noma","dwar","ghos","kght","ogre","brbn","trol","cavl","drui",
    "arcm","vamp","gian","demo","drag",
};

char *DOS_villain_names[] = {
    "mury","hack","ammi","baro","drea","cane","mora","barr","barg","rina",
    "ragf","mahk","auri","czar","magu","urth","arec",
};

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
			SDL_Rect dst = { 0, 0, 48, 34 };
			SDL_Rect src = { 0, 0, 48, 34 };
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
			middle_name = "knig";
			suffix = bpp_names[mod->bpp];
			ident = "#0";
			if (sub_id == 1) middle_name = "pala";
			if (sub_id == 2) middle_name = "sorc";
			if (sub_id == 3) middle_name = "barb";
		}
		break;
		case GR_LOCATION:	/* subId - 0 home 1 town 2 - 6 dwelling */
		{
			method = RAW_IMG;
			middle_name = "cstl";
			suffix = bpp_names[mod->bpp];
			ident = "#0";
			if (sub_id == 1) middle_name = "town";
			if (sub_id == 2) middle_name = "plai";
			if (sub_id == 3) middle_name = "frst";
			if (sub_id == 4) middle_name = "cave";
			if (sub_id == 5) middle_name = "dngn";
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
				0x999999,
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
			static char buf[4096];
			static char pbuf[4096];

	int segment = 0x15850;

			int ptroff = 0x1844D;
			int off = 0x191CB;
			int len = 0x19815 - 0x191CB;
			
			KB_File *f;	int n;   

			KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
			f = KB_fopen_with("kb.exe", "rb", mod);
			if (f == NULL) return NULL;
#if 0			
			KB_fseek(f, ptroff, 0);
			n = KB_fread(&pbuf[0], sizeof(char), len, f);
#endif
			KB_fseek(f, off, 0);
			n = KB_fread(&buf[0], sizeof(char), len, f);
			KB_fclose(f);
			
			char *p = &pbuf[0];
			
		
			int i, j = 1;
			for (i = 0; i < n - 1; i++)  {
				if (buf[i] == '\0') {
					if (j) buf[i] = '\n';
					j=1-j;
				}
			}

#if 0			
			//READ_WORD_BE(p);
			//p+=2;
			for (i = 1; i < 20; i++) {
				word soff;
				soff = READ_WORD_BE(p);
				//p+=2;
				buf[soff + segment - off - 1] = '\0';
				printf("\n####");
				printf("Splitting: '%s'\n", &buf[soff + segment - off]);
				sleep(1);
				//KB_fseek(f, segment + soff, 0);
			}
#endif

			return &buf[0];
		}
		break;
		case STR_VNAME:
		{
			return KB_strlist_ind(DOS_Resolve(mod, STRL_VNAMES, 0), sub_id);
		}
		break;
		case STRL_VNAMES:
		{
			static char buf[4096];
			static char pbuf[4096];

	int segment = 0x15850;

			int ptroff = 0x1842C;
			int off = 0x190A5;
			int len = 0x191cb - off;

			KB_File *f;	int n;   

			KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
			f = KB_fopen_with("kb.exe", "rb", mod);
			if (f == NULL) return NULL;
#if 1			
			KB_fseek(f, ptroff, 0);
			n = KB_fread(&pbuf[0], sizeof(char), len, f);
#endif
			KB_fseek(f, off, 0);
			n = KB_fread(&buf[0], sizeof(char), len, f);
			KB_fclose(f);

			char *p = &pbuf[0];

#if 1		
			int i;
			for (i = 0; i < 17; i++) {
				word soff;
				soff = READ_WORD(p);
				KB_debuglog(0, "%02x [%04x - %08x] Splitting: '%s' %c %d \n", i, soff, soff + segment, &buf[soff + segment - off]);
				//KB_fseek(f, segment + soff, 0);
			}
#endif

			return &buf[0];
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
			static char buf[24096];
			static char pbuf[24096];

	int segment = 0x15850;

			int ptroff = 0x16bf4;
			int off = 0x16edf;
			int len = 0x18427- off;

			KB_File *f;	int n;   

			KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
			f = KB_fopen_with("kb.exe", "rb", mod);
			if (f == NULL) return NULL;
#if 1			
			KB_fseek(f, ptroff, 0);
			n = KB_fread(&pbuf[0], sizeof(char), len, f);
#endif
			KB_fseek(f, off, 0);
			n = KB_fread(&buf[0], sizeof(char), len, f);
			KB_fclose(f);

			char *p = &pbuf[0];
#if 1
			int i, j;
			for (i = 0; i < 17; i++) {
				for (j = 0; j < 13; j ++) {
					word soff;
					soff = READ_WORD(p);
					//printf("%02x [%04x - %08x] Splitting: '%s' %c %d \n", i, soff, soff + segment, &buf[soff + segment - off]);
					//KB_fseek(f, segment + soff, 0);
				}
			}
#endif

			return &buf[0];
		}
		break;
		case STR_CREDIT:
		{
			return (void*)KB_strlist_ind(DOS_Resolve(mod, STRL_CREDITS, 0), sub_id);
		}
		break;
		case STRL_CREDITS:
		{
			return DOS_read_strings(mod, 0x16031, 0x160FF);
		}
		break;		
		case RECT_MAP:
		{
			static SDL_Rect map = {	16, 14 + 7, 48 * 5, 34 * 5	};
			return &map;
		}
		break;
		case RECT_UI:
			switch (sub_id) {
			case 0:		/* Top */
			{
				static SDL_Rect ui = {	0, 0, 320, 8 	};
				return &ui;
			}
			break;
			case 1:		/* Left */
			{
				static SDL_Rect ui = {	0, 0, 16, 200	};
				return &ui;
			}
			break;
			case 2:		/* Right */
			{
				static SDL_Rect ui = {	300, 0, 16, 200	};
				return &ui;
			}
			break;
			case 3:		/* Bottom */
			{
				static SDL_Rect ui = {	0, 0, 320, 8	};
				return &ui;
			}
			break;
			case 4:		/* Bar */
			{
				static SDL_Rect ui = {	0, 17, 280, 5	};
				return &ui;
			}
			break;
			default: break;
			}
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
						if (mod->bpp == 1) DOS_SetColors(surf, 1);
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
					if (mod->bpp == 1) DOS_SetColors(surf, 1);	
					
					KB_closedir(d);
				}
				break;
				default: break;
			}

			return (void*)surf;
		}
		break;
		default: break;
	}
	return NULL;
}

