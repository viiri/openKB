/*
 *  dos-data.c -- DOS module
 *  Copyright (C) 2011-2014 Vitaly Driedfruit
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

#include "kbauto.h"
#include "kbconf.h"
#include "kbfile.h"
#include "kbres.h"
#include "kbsound.h"

#include "dos-snd.h"

#define KBEXE95_FILESIZE 113718 //bytes

#define KBEXETYPE_90	0
#define KBEXETYPE_95	1

enum DOS_offset_names {
	DATA_SEGMENT,
	DOS_CREDITS,
	DOS_GAME_LOST,
	DOS_GAME_WON,
	DOS_SIGNS,
	DOS_VNAMES,
	DOS_VDESCS,
	DOS_ANAMES,
	DOS_ADESCS,
	TUNE_NOTES,	/* Frequency palette */
	TUNE_DELAY,	/* Delay palette */
	TUNE_PTR,	/* Pointers into "tunes" (offsets to "freq,delay,...,0xFF" locations) */
	DOS_VREWARDS,
};
int DOS_exe_offsets[][4] = {
	/* KB90              KB95
	   offest  len       offset    len */
	{ 0x15690,	0,  	0x15850,	0   	},	//DATA SEGMENT
	{ 0x15E6D,	0xCB,	0x16031,	0xCE	},	//DOS CREDITS //????, 0x10a2
	{ 0x1AF1D,	0x130,	0x1B0E3,	0x130	},	//GAME LOST //????, ????
	{ 0x1ADEE,	0x12F,	0x1AFB4,	0x12F	},	//GAME WON  //????, ????
	{ 0x19005,	0x64A,	0x191CB,	0x64A	},	//SIGNS  //????, ????
	{ 0x18EDF,	0x126,	0x190A5,	0x126	},	//VNAMES  //????, ????
	{ 0x16D19,	0x1548,	0x16EDF,	0x1548	},	//VDESCS  //????, ????
	{ 0,    	0,  	0,      	0   	},	//ANAMES  //????, ????
	{ 0x19650,	0x351,	0x19816, 	0x351 	},	//ADESCS  //????, 0x2D1E
	{ 0x189D1,	0     ,	0x18B97,	0     	},	//TUNE_NOTES  //????, 0x3347
	{ 0x18A81,	0     ,	0x18C47,	0     	},	//TUNE_DELAY  //????, 0x33F7
	{ 0x18AA1,	0     ,	0x18C67,	0       },	//TUNE_PTR  //????, 0x3417 | ????, 0x330d
	{ 0x1873A, 	0     ,	0x18900,	0       },	//VREWARDS
};

//TODO: convert all string offsets to relative to DS AND
// use the string_p function, to read from provided pointer tables
// and not directly.
// This will allow reading modded files.

#define KBEXE_OFFSET(ETYP, WH) DOS_exe_offsets[WH][ETYP * 2]
#define KBEXE_LEN(ETYP, WH) DOS_exe_offsets[WH][ETYP * 2 + 1]

#define KBEXE_POS(ETYP, WH) KBEXE_OFFSET(ETYP, WH), \
                            KBEXE_OFFSET(ETYP, WH) + \
                            KBEXE_LEN(ETYP, WH)

#define KBEXE_PTR(ETYP, WH) KBEXE_OFFSET(ETYP, DATA_SEGMENT), \
                            KBEXE_OFFSET(ETYP, WH)

#define MCGA_PALETTE_OFFSET	0x032D

#define DOS_TILE_W 48
#define DOS_TILE_H 34

typedef struct DOS_Cache {

	SDL_Color *vga_palette;
	SDL_Color *cga_palette;
	byte cga_index;

	struct tunGroup *tunes;

	KB_File *exe_file;
	byte exe_type;

} DOS_Cache;

KB_File* DOS_UnpackExe(KB_File *f, int freesrc);

KB_File* DOS_fopen_exe(KBmodule *mod) {
	KB_File *f;
	DOS_Cache *ch = mod->cache;

	KB_debuglog(0, "[dos] '%s' Cached Pointer to exe: %p\n", mod->name, ch->exe_file);

	/* File already opened */
	if (ch->exe_file) {
		KB_fseek(ch->exe_file, 0, 0); /* Rewind */
		ch->exe_file->ref_count++; /* Inc. ref count */
		return ch->exe_file;
	}

	/* Try case-insensitive search in all slots of current module */
	f = KB_fcaseopen_with("kb.exe", "rb", mod);

	/* Switch to new, unpacked file (if appropriate) */
	f = DOS_UnpackExe(f, 1);

	/* Note the exe type (for different offset locations) */
	if (f && f->len == KBEXE95_FILESIZE)
		ch->exe_type = KBEXETYPE_95;
	else
		ch->exe_type = KBEXETYPE_90;

	/* HACK! -- Keep handler opened */
	if (f) f->ref_count++;

	/* Store pointer */
	ch->exe_file = f;

	return f;
}
void DOS_fclose_exe(KB_File *f) {
	/* Do nothing */
	f->ref_count--;
}

KBsound* DOS_load_tune(KBmodule *mod, int tune_id) {
	DOS_Cache *ch = mod->cache;

	KBsound *snd;

	if (ch->tunes == NULL) return NULL;
	if (tune_id < 0 || tune_id > ch->tunes->num_files - 1) return NULL; 

	snd = malloc(sizeof(KBsound));

	snd->type = KBSND_DOS;
	snd->data = &ch->tunes->files[tune_id];

	return snd;
}

struct tunGroup * DOS_ReadTunes(KBmodule *mod) {
	struct tunGroup *tunes;
	KB_File *f;
	int exe_type;

	KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
	f = DOS_fopen_exe(mod);
	if (f == NULL) { KB_errlog("! DOS EXE FILE\n"); return NULL; }

	exe_type = ((DOS_Cache*)mod->cache)->exe_type;

	/* Load tune offsets */
	KB_fseek(f, KBEXE_OFFSET(exe_type, TUNE_PTR), 0);
	tunes = tunGroup_load(f);
	if (tunes == NULL) { return NULL; }

	/* Load palette */
	KB_fseek(f, KBEXE_OFFSET(exe_type, TUNE_NOTES), 0);
	tunPalette_load(&tunes->palette, f);

	/* Load all tunes form offsets (palette is memcpy'd into each tune) */
	tunGroup_loadfiles(tunes, f, KBEXE_OFFSET(exe_type, DATA_SEGMENT));

	DOS_fclose_exe(f);
	return tunes;
}

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

		if (ch->cga_palette) {

			pal = ch->cga_palette;

		}

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
	f = DOS_fopen_exe(mod);
	if (f == NULL) return NULL;

	KB_fseek(f, ptroff, 0);
	n = KB_fread(&pbuf[0], sizeof(char), len, f);

	KB_fseek(f, off, 0);
	n = KB_fread(&buf[0], sizeof(char), len, f);
	DOS_fclose_exe(f);

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
char* DOS_read_strings_p(KBmodule *mod, int segment, int ptroff, int num_strings) {
	char *buf, *pbuf, *p;
	int i, n, len, plen;
	KB_File *f;

	plen = 2 * num_strings; /* 2 BYTEs for each pointer */

	pbuf = malloc(plen);
	if (pbuf == NULL) return NULL;

	KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
	f = DOS_fopen_exe(mod);
	if (f == NULL) {
		free(pbuf);
		return NULL;
	}
	KB_fseek(f, segment + ptroff, 0);
	n = KB_fread(pbuf, sizeof(char), plen, f);
	if (n < plen) {
		DOS_fclose_exe(f);
		free(pbuf);
		return NULL;
	}

	int tsize = 32 * num_strings;
	char *ret;

	ret = malloc(sizeof(char) * tsize);
	if (ret == NULL) {
		DOS_fclose_exe(f);
		free(pbuf);
		return NULL;
	}

	len = 0;
	for (i = 0; i < num_strings; i++) {
		char buf[256]; /* arbitary max size of one string */
		word off;
		int l;
		char *p = pbuf + i * 2;
		off = READ_WORD(p);

		printf("Reading string %d\n", i);

		KB_fseek(f, segment + off, 0);
		n = KB_fread(&buf[0], sizeof(char), sizeof(buf), f);
		buf[n] = '\0';

		printf("Read: { %s }\n", buf);

		l = strlen(buf) + 1;
		if (l < n) n = l;

		if (len + n >= tsize) {
			tsize *= 2;
			ret = realloc(ret, sizeof(char) * tsize);
			if (ret == NULL) {
				DOS_fclose_exe(f);
				free(pbuf);
				return NULL;
			}
		}

		memcpy(ret + len, buf, n);
		len += n;
		ret[len - 1] = '\n';
	}

	free(pbuf);
	DOS_fclose_exe(f);
	return ret;
}
#endif

int DOS_read_word_array(KBmodule *mod, word *dst, int offset, int len) {
	char *buf, *p;

	KB_File *f;	
	int n, i;

	buf = malloc(sizeof(word) * len);
	if (buf == NULL) return -1;

	KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
	f = DOS_fopen_exe(mod);
	if (f == NULL) {
		free(buf);
		return -2;
	}

	KB_fseek(f, offset, 0);
	n = KB_fread(buf, sizeof(word), len, f);
	DOS_fclose_exe(f);
	if (n < len) {
		free(buf);
		return -3;
	}

	p = buf;	
	for (i = 0; i < len; i++) {
		dst[i] = READ_WORD(p);
	}

	free(buf);
	return 0;
}
word* DOS_word_array(KBmodule *mod, int offset, int len) {
	word *dst;
	int err;

	dst = malloc(sizeof(word) * len);
	if (dst == NULL) return NULL;

	err = DOS_read_word_array(mod, dst, offset, len);
	if (err) {
		free(dst);
		return NULL;
	}

	return dst;
}

char* DOS_read_strings(KBmodule *mod, int off, int endoff) {
	char *buf;

	int len = endoff - off;
	
	KB_File *f;	
	int n;

	buf = malloc(sizeof(char) * (len+1)); /* Extra 1 byte for 0xFF marker */
	if (buf == NULL) return NULL;

	KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
	f = DOS_fopen_exe(mod);
	if (f == NULL) {
		free(buf);
		return NULL;
	}

	KB_fseek(f, off, 0);
	n = KB_fread(buf, sizeof(char), len, f);
	//buf[len-1] = (char)0x00;
	buf[len] = (char)0xFF;
	DOS_fclose_exe(f);
	if (n < len) {
		free(buf);
		return NULL;
	}
	buf[n - 1] = '\0';
	buf[n] = (char)0xFF;

	//for (n = 0; n < len; n++) {
	//	if (buf[n] == '\0')
	//		buf[n] = '\n';
	//}

	return buf;
}

char* DOS_read_credits(KBmodule *mod, int off, int endoff) {

	char *buf, *raw, *ptr;
	int max = endoff - off, newmax = max + 50;

	raw = DOS_read_strings(mod, off, endoff);
	if (raw == NULL) return NULL;

	buf = malloc(sizeof(char) * newmax);

	ptr = raw;
	buf[0] = '\0';

	//DOS_compact_strings(mod, buf, len);

	int need_tab = 0;
	int need_jump = 0;
	int need_center = 0;

	int used = 0;
	while(*ptr) {
		int mlen = 0;
		mlen = strlen(ptr);

		/* Determine if decorations are needed */
		if (!strncasecmp(ptr, "copyright", 9)) { /* Line is a copyright notice */
			need_center = 1;
			need_jump = 1;
		}	
		if (ptr[mlen - 1] == ':') { /* Line ends with ':', it's a title */
			need_jump = used;
		} else {	/* Line is probably a person's name */
			need_tab = 2;
		}
		if (need_center) {
			need_tab = (30 - mlen) / 2;
		}
		/* Apply decorations */
		if (need_jump) {
			strlcat(buf, "\n", newmax);
			used++;
			need_jump = 0;
		}
		if (need_tab) {
			int i;
			for (i = 0; i < need_tab; i++) buf[used + i] = ' ';
			buf[used + i] = '\0';
			used += need_tab;
			need_tab = 0;
		}
		/* Apply actual string */
		strlcat(buf, ptr, newmax);
		used += mlen;

		/* Advance pointer */
		ptr += mlen;
		if (ptr - raw < max) ptr++; 

		/* Newline */
		if (ptr - raw < max && used < newmax) {
			buf[used] = '\n';
			used ++;
			buf[used] = '\0';
		}

		/* HACK! End of strlist? */
		if (*ptr == (char)0xFF) {
			buf[used] = '\0';
			used ++;
			buf[used] = (char)0xFF;
			break;
		}
	}

	free(raw);
	return buf;
}

char* DOS_read_vdescs(KBmodule *mod, int off, int endoff, int skip_lines) {

	char *buf, *raw, *ptr;
	int max = endoff - off,
		newmax = max + max / 2;

	raw = DOS_read_strings(mod, off, endoff);
	if (raw == NULL) return NULL;

	buf = malloc(sizeof(char) * newmax);
	if (buf == NULL) {
		free(raw);
		return NULL; /* Out of memory */
	}

	ptr = raw;
	buf[0] = '\0';

	int line = 0;
	int need_tab = 0;

	byte line_offsets[14] = {
		7,
		7,
		7,
		7,
		10,
		0,
	};

	int used = 0;
	while(*ptr && line < skip_lines + 14) {
		int mlen = 0;
		mlen = strlen(ptr);

		if (line < skip_lines) {
			ptr += mlen;
			if (ptr - raw < max) ptr++;
			line++;
			continue;
		}

		/* Determine if decorations are needed */
		need_tab = line_offsets[line - skip_lines];
		line++;
		/* Apply decorations */
		if (need_tab) {
			int i;
			for (i = 0; i < need_tab; i++) buf[used + i] = ' ';
			buf[used + i] = '\0';
			used += need_tab;
			need_tab = 0;
		}
		/* Apply actual string */
		strlcat(buf, ptr, newmax);
		used += mlen;
		/* Hack -- Apply extra '%s' */
		if (!strncasecmp(ptr, "last seen:", 10)
		 || !strncasecmp(ptr, "castle:", 7)) {
		 	KB_strlcat(buf, " %s", newmax);
		 	used += 3;
		 }

		/* Advance pointer */
		ptr += mlen;
		if (ptr - raw < max) ptr++; 
 
		/* Newline */
		if (ptr - raw < max && used < newmax) {
			buf[used] = '\n';
			used ++;
			buf[used] = '\0';
		}
	}

	free(raw);
	return buf;
}

char* DOS_read_adescs(KBmodule *mod, int off, int endoff, int skip_lines) {

	char *buf, *raw, *ptr;
	int max = endoff - off,
		newmax = max + max / 2;

	raw = DOS_read_strings(mod, off, endoff);
	if (raw == NULL) return NULL;

//	DOS_compact_strings(mod, raw, (endoff-off));
//	printf("READ: %s\n", raw);

	buf = malloc(sizeof(char) * newmax);
	if (buf == NULL) {
		free(raw);
		return NULL; /* Out of memory */
	}

	ptr = raw;
	buf[0] = '\0';

	int line = 0;
	int used = 0;
	while(ptr - raw < max && line < skip_lines + 5) {
		int mlen = 0;
		mlen = strlen(ptr);

		if (line < skip_lines) {
			ptr += mlen;
			if (ptr - raw < max) ptr++;
			line++;
			continue;
		}

		/* Apply actual string */
		strlcat(buf, ptr, newmax);
		used += mlen;

		/* Advance pointer */
		ptr += mlen;
		if (ptr - raw < max) ptr++;

		/* Newline */
		if (ptr - raw < max && used < newmax) {
			buf[used] = '\n';
			used ++;
			buf[used] = '\0';
		}
		
		line++;
	}

	free(raw);
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

Uint32* DOS_CGA_ColorScheme(KBmodule *mod, int sub_id) {

	/* 0=black,1=cyan,2=magenta,3=white */
	byte cga_scheme_chrome_index[] = {
		0,	// background
		3,	// text1
		3,	// text2
		3,	// text3
		3,	// text4
		1,	// shadow1
		1,	// shadow2
		3,	// frame1
		3,	// frame2
		3,	// sel_background
		0,	// sel_text1
		0,	// sel_text2
		0,	// sel_text3
		0,	// sel_text4
		1,	// sel_shadow1
		1,	// sel_shadow2
		3,	// sel_frame1
		3,	// sel_frame2
	};
	byte cga_scheme_minimap_index[] = {
		2,	// background
		3,	// text1
		3,	// text2
		3,	// text3
		3,	// text4
		1,	// shadow1
		1,	// shadow2
		3,	// frame1
		3,	// frame2
		2,	// sel_background
		0,	// sel_text1
		0,	// sel_text2
		0,	// sel_text3
		0,	// sel_text4
		1,	// sel_shadow1
		1,	// sel_shadow2
		3,	// sel_frame1
		3,	// sel_frame2
	};
	byte cga_scheme_char_index[] = {
		0,	// background
		3,	// text1
		3,	// text2
		3,	// text3
		3,	// text4
		1,	// shadow1
		1,	// shadow2
		2,	// frame1
		2,	// frame2
		0,	// sel_background
		3,	// sel_text1
		3,	// sel_text2
		3,	// sel_text3
		3,	// sel_text4
		1,	// sel_shadow1
		1,	// sel_shadow2
		2,	// sel_frame1
		2,	// sel_frame2
	};

	int i;
	byte *cptr;
	Uint32 *colors;

	DOS_Cache *cache = mod->cache;

	colors = malloc(sizeof(Uint32) * COLORS_MAX);
	if (colors == NULL) return NULL;

	switch (sub_id) {
		case CS_MINIMAP:
			cptr = cga_scheme_minimap_index;
		break;
		case CS_VIEWCHAR:
			cptr = cga_scheme_char_index;
		break;
		default:
			cptr = cga_scheme_chrome_index;
		break;
	}

	for (i = 0; i < COLORS_MAX; i++) {
		colors[i] = ega_pallete_rgb[cga_palletes_ega[cache->cga_index][cptr[i]]];
	}

	return colors;
}

SDL_Color* DOS_CGAPalette(KBmodule *mod) {
	SDL_Color *pal;
	int i;
	DOS_Cache *cache = mod->cache;

	pal = malloc(sizeof(SDL_Color) * 256);
	if (pal == NULL) return NULL;

	for (i = 0; i < 4; i++) {
		SDL_Color *col = &ega_pallete_sdl[cga_palletes_ega[cache->cga_index][i]];
		pal[i].r = col->r;
		pal[i].g = col->g;
		pal[i].b = col->b;
	}
	return pal;
}


void DOS_AdjustSlots(KBmodule *mod) {

	/* Hack -- if only one slot was provided, assume it's ROOT DIR with the CC and EXE files */
	if (mod->slotB_name[0] == 0) { //empty string

		char name[8];

		KB_dircpy(mod->slotB_name, mod->slotA_name);
		if (test_directory(mod->slotB_name, 0))
			KB_grpsep(mod->slotB_name);
		else
			KB_dirsep(mod->slotB_name);

		if (!match_file(mod->slotB_name, mod->bpp == 8 ? "256.CC" : "416.CC", &name[0]))
			return;

		KB_strcat(mod->slotB_name, name);
		KB_strcat(mod->slotB_name, "#");

		KB_debuglog(0, "* SlotB: %s\n", mod->slotB_name);
	}

}

void DOS_Init(KBmodule *mod) {

	DOS_Cache *cache = malloc(sizeof(DOS_Cache));

	DOS_AdjustSlots(mod);

	if (cache) {
		/* Init */
		cache->cga_palette = NULL;
		cache->vga_palette = NULL;
		cache->exe_file = NULL;
		/* Set */
		mod->cache = cache;
		/* Pre-cache some things */
		if (mod->bpp == 2) {
			cache->cga_index = 1;
			cache->cga_palette = DOS_CGAPalette(mod);
		}
		if (mod->bpp == 8) {
			cache->vga_palette = DOS_ReadPalette(mod, "MCGA.DRV");
		}
		cache->tunes = DOS_ReadTunes(mod);
	}

}

void DOS_Stop(KBmodule *mod) {
	DOS_Cache *cache = mod->cache;

	if (cache->exe_file) {
		KB_fclose(cache->exe_file);
	}

	free(cache->vga_palette);
	free(cache->cga_palette);
	free(cache->tunes);
	free(mod->cache);
}

/* Whatever static resources need run-time initialization - do it here */
void DOS_PrepareStatic(KBmodule *mod) {


}


void* DOS_Resolve(KBmodule *mod, int id, int sub_id) {

	char *middle_name;
	char *suffix;
	char *ident;

	byte exe_type = 0;

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

	char subId_str[8];

	middle_name = suffix = ident = NULL;

	exe_type = ((DOS_Cache*)mod->cache)->exe_type;

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
			snprintf(subId_str, 8, "#%d", sub_id);
			ident = subId_str;
		}
		break;
		case GR_ENDING:	/* subId - 0=won, 1=lost */
		{
			/* Ending screen */
			method = RAW_IMG;
			middle_name = "endpic";
			suffix = bpp_names[mod->bpp];
			snprintf(subId_str, 8, "#%d", sub_id);
			ident = subId_str;
		}
		break;
		case GR_ENDTILE:	/* subId - 0=grass, 1=carpet, 2=hero */
		{
			/* Ending tile */
			method = RAW_IMG;
			middle_name = "endpic";
			suffix = bpp_names[mod->bpp];
			snprintf(subId_str, 8, "#%d", sub_id + 2);
			ident = subId_str;
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
			snprintf(subId_str, 8, "#%d", sub_id);
			ident = subId_str;
		}
		break;
		case GR_TILESET:	/* subId - continent */
		{
			SDL_Rect tilesize = { 0, 0, DOS_TILE_W, DOS_TILE_H };
			if (sub_id) return KB_LoadTilesetSalted(sub_id, DOS_Resolve, mod);
			return KB_LoadTileset_TILES(&tilesize, DOS_Resolve, mod);
			//return KB_LoadTileset_ROWS(&tilesize, DOS_Resolve, mod);
		}
		break;
		case GR_TILESALT:	/* subId - 0=full; 1-3=continent */
		{
			/* A row of replacement tiles. */
			method = IMG_ROW;
			middle_name = "tilesalt";
			suffix = bpp_names[mod->bpp];
			ident = "";
			if (sub_id) {
				/* 3 tiles per continent */
				row_start = (sub_id - 1) * 3;
				row_frames = 3;
			} else {
				/* All of them */
				row_start = 0;
				row_frames = 9;
			}
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
			snprintf(subId_str, 8, "#%d", sub_id);
			ident = subId_str;
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
			/* For CGA sake use non-generic palette */
			DOS_SetPalette(mod, piece, mod->bpp);
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
		case PAL_PALETTE:
		{
			if (mod->bpp == 2) { /* CGA mode */
				DOS_Cache* cache = mod->cache;
				if (sub_id > 7 || sub_id < 0) return NULL;
				cache->cga_index = sub_id;
				free(cache->cga_palette);
				cache->cga_palette = DOS_CGAPalette(mod);
				return DOS_CGAPalette(mod);
			}
			return NULL;
		}
		case COL_DWELLING:
		{
			byte ega_dwelling_index[5] = {
				EGA_GREY, // plains
				EGA_DGREEN,// forest
				EGA_BLUE, // hill/cave
				EGA_DRED,  // dungeon
				EGA_MAGENTA, // castle
			};
			Uint32 *colors;
			int i;
			colors = malloc(sizeof(Uint32) * COLORS_MAX);
			if (colors == NULL) return NULL;
			for (i = 0; i < 5; i++) {
				colors[i] = ega_pallete_rgb[ega_dwelling_index[i]];

				if (mod->bpp == 2) { /* CGA */
					colors[i] = 0; /* black */
				}
			}
			return colors;
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
			} tile_type = DEEP_WATER;
			byte ega_minimap_index[8] = {
				EGA_BLUE,
				EGA_DBLUE,
				EGA_GREEN,
				EGA_YELLOW,
				EGA_BROWN,
				EGA_DGREEN,
				EGA_WHITE,
				EGA_DRED,
			};
			byte cga_minimap_index[8] = {
				1, // water is cyan
				1, // water is cyan
				2, // grass is magenta
				3, // desert is white
				0, // rock is black
				1, // tree is cyan
				3, // castle is white
				2, // hotspot is magenta
			};
			Uint32 *colors;
			int tile;
			byte cga_index = ((DOS_Cache*)mod->cache)->cga_index;

			colors = malloc(sizeof(Uint32) * 256);
			if (colors == NULL) return NULL;

			for (tile = 0; tile < 256; tile++) {

				if ( IS_GRASS(tile) ) tile_type = GRASS;
				else if ( IS_DEEP_WATER(tile) ) tile_type = DEEP_WATER;
				else if ( IS_WATER(tile) ) tile_type = SHALLOW_WATER;
				else if ( IS_DESERT(tile) ) tile_type = DESERT;
				else if ( IS_ROCK(tile) ) tile_type = ROCK;
				else if ( IS_TREE(tile) ) tile_type = TREE;
				else if ( IS_CASTLE(tile) ) tile_type = CASTLE;
				else if ( IS_MAPOBJECT(tile) || IS_INTERACTIVE(tile)) tile_type = MAP_OBJECT;

				colors[tile] = ega_pallete_rgb[ega_minimap_index[tile_type]];

				if (mod->bpp == 2) { /* CGA */
					colors[tile] = ega_pallete_rgb[cga_palletes_ega[cga_index][ cga_minimap_index[tile_type] ] ];
				}
			}
			/* Fog color */
			colors[0xFF] = ega_pallete_rgb[EGA_BLACK];
			if (mod->bpp == 2) { /* CGA */
				colors[0xFF] = ega_pallete_rgb[cga_palletes_ega[cga_index][ 0 ]];
			}
			return colors;
		}
		break;
		case COL_TEXT:
		{
			if (mod->bpp == 2) return DOS_CGA_ColorScheme(mod, sub_id);
			byte ega_scheme_status_index[] = {
				EGA_DRED,	// background
				EGA_WHITE,	// text1
				EGA_WHITE,	// text2
				EGA_WHITE,	// text3
				EGA_WHITE,	// text4
				EGA_MAGENTA,// shadow1
				EGA_MAGENTA,// shadow2
				EGA_YELLOW,	// frame1
				EGA_YELLOW,	// frame2
				EGA_YELLOW,	// sel_background
				EGA_DCYAN,	// sel_text1
				EGA_DCYAN,	// sel_text2
				EGA_DCYAN,	// sel_text3
				EGA_DCYAN,	// sel_text4
				EGA_MAGENTA,// sel_shadow1
				EGA_MAGENTA,// sel_shadow2
				EGA_YELLOW,	// sel_frame1
				EGA_YELLOW,	// sel_frame2
			};
			byte ega_scheme_status_replacers[] = {
				EGA_GREY,    /* Background for status 0 */
				EGA_DRED,    /* Background for status 1 */
				EGA_BLUE,    /* Background for status 2 */
				EGA_DVIOLET, /* Background for status 3 */
				EGA_DVIOLET, /* Background for status 4 (same as 3) */
			};
			byte ega_scheme_character_index[] = {
				EGA_DGREY,	// background
				EGA_WHITE,	// text1
				EGA_WHITE,	// text2
				EGA_WHITE,	// text3
				EGA_WHITE,	// text4
				EGA_MAGENTA,// shadow1
				EGA_MAGENTA,// shadow2
				EGA_DRED,	// frame1
				EGA_DRED,	// frame2
				EGA_DGREY,	// sel_background
				EGA_WHITE,	// sel_text1
				EGA_WHITE,	// sel_text2
				EGA_WHITE,	// sel_text3
				EGA_WHITE,	// sel_text4
				EGA_MAGENTA,// sel_shadow1
				EGA_MAGENTA,// sel_shadow2
				EGA_DRED,	// sel_frame1
				EGA_DRED,	// sel_frame2
			};
			byte ega_scheme_menu_index[] = {
				EGA_DBLUE,	// background
				EGA_WHITE,	// text1
				EGA_WHITE,	// text2
				EGA_WHITE,	// text3
				EGA_WHITE,	// text4
				EGA_MAGENTA,// shadow1
				EGA_MAGENTA,// shadow2
				EGA_YELLOW,	// frame1
				EGA_YELLOW,	// frame2
				EGA_WHITE,	// sel_background
				EGA_DBLUE,	// sel_text1
				EGA_DBLUE,	// sel_text2
				EGA_DBLUE,	// sel_text3
				EGA_DBLUE,	// sel_text4
				EGA_MAGENTA,// sel_shadow1
				EGA_MAGENTA,// sel_shadow2
				EGA_YELLOW,	// sel_frame1
				EGA_YELLOW,	// sel_frame2
			};
			byte ega_scheme_chrome_index[] = {
				EGA_BLACK,	// background
				EGA_WHITE,	// text1
				EGA_WHITE,	// text2
				EGA_WHITE,	// text3
				EGA_WHITE,	// text4
				EGA_MAGENTA,// shadow1
				EGA_MAGENTA,// shadow2
				EGA_YELLOW,	// frame1
				EGA_YELLOW,	// frame2
				EGA_WHITE,	// sel_background
				EGA_BLACK,	// sel_text1
				EGA_BLACK,	// sel_text2
				EGA_BLACK,	// sel_text3
				EGA_BLACK,	// sel_text4
				EGA_MAGENTA,// sel_shadow1
				EGA_MAGENTA,// sel_shadow2
				EGA_MAGENTA,// sel_frame1
				EGA_MAGENTA,// sel_frame2
			};			
			byte ega_scheme_mb_index[] = {
				EGA_DBLUE,	// background
				EGA_WHITE,	// text1
				EGA_WHITE,	// text2
				EGA_WHITE,	// text3
				EGA_WHITE,	// text4
				EGA_MAGENTA,// shadow1
				EGA_MAGENTA,// shadow2
				EGA_YELLOW,	// frame1
				EGA_YELLOW,	// frame2
				EGA_WHITE,	// sel_background
				EGA_DBLUE,	// sel_text1
				EGA_DBLUE,	// sel_text2
				EGA_DBLUE,	// sel_text3
				EGA_DBLUE,	// sel_text4
				EGA_MAGENTA,// sel_shadow1
				EGA_MAGENTA,// sel_shadow2
				EGA_MAGENTA,// sel_frame1
				EGA_MAGENTA,// sel_frame2
			};
			byte ega_scheme_savefile_index[] = {
				EGA_BLUE,	// background
				EGA_WHITE,	// text1
				EGA_WHITE,	// text2
				EGA_WHITE,	// text3
				EGA_WHITE,	// text4
				EGA_MAGENTA,// shadow1
				EGA_MAGENTA,// shadow2
				EGA_MAGENTA,// frame1
				EGA_MAGENTA,// frame2
				EGA_WHITE,	// sel_background
				EGA_BLUE,	// sel_text1
				EGA_BLUE,	// sel_text2
				EGA_BLUE,	// sel_text3
				EGA_BLUE,	// sel_text4
				EGA_MAGENTA,// sel_shadow1
				EGA_MAGENTA,// sel_shadow2
				EGA_MAGENTA,// sel_frame1
				EGA_MAGENTA,// sel_frame2
			};
			byte ega_scheme_ending_index[] = {
				EGA_DBLUE,	// win_background
				EGA_WHITE,	// win_text1
				EGA_WHITE,	// win_text2
				EGA_WHITE,	// win_text3
				EGA_WHITE,	// win_text4
				EGA_MAGENTA,// win_shadow1
				EGA_MAGENTA,// win_shadow2
				EGA_MAGENTA,// win_frame1
				EGA_MAGENTA,// win_frame2
				EGA_DBLUE,	// lose_background
				EGA_WHITE,	// lose_text1
				EGA_WHITE,	// lose_text2
				EGA_WHITE,	// lose_text3
				EGA_WHITE,	// lose_text4
				EGA_MAGENTA,// lose_shadow1
				EGA_MAGENTA,// lose_shadow2
				EGA_MAGENTA,// lose_frame1
				EGA_MAGENTA,// lose_frame2
			};
			byte *ega_index_ptr;
			int i;
			Uint32 *colors;

			colors = malloc(sizeof(Uint32) * COLORS_MAX);
			if (colors == NULL) return NULL;

			switch (sub_id) {
				case CS_ENDING: 	/* Ending message */
					ega_index_ptr = ega_scheme_ending_index;
					break;
				case CS_CHROME: 	/* "Raw" menus (black & white) */
					ega_index_ptr = ega_scheme_chrome_index;
					break;
				case CS_MINIMENU:	/* Savefile selection */
					ega_index_ptr = ega_scheme_savefile_index;
					break;
				case CS_VIEWARMY:	/* Army screen */
				case CS_VIEWCHAR:	/* Character screen */
					ega_index_ptr = ega_scheme_character_index;
					break;
				case CS_STATUS_1: /* Status bar .. by difficulty */
				case CS_STATUS_2:
				case CS_STATUS_3:
				case CS_STATUS_4:
				case CS_STATUS_5:
					ega_index_ptr = ega_scheme_status_index;
					/* Replace BackgroundColor based on difficulty */
					ega_scheme_status_index[COLOR_BACKGROUND] =
						ega_scheme_status_replacers[sub_id - 1];
					break;
				case CS_MINIMAP:	/* Minimap text */
				case CS_GENERIC:
				default: /* Default Message Box */
					ega_index_ptr = ega_scheme_mb_index;
					break;
			}

			for (i = 0; i < COLORS_MAX; i++) {

				colors[i] = ega_pallete_rgb[ega_index_ptr[i]];

			}

			return colors;
		}	
		break;
		case STR_SIGN:
		{
			char *list = DOS_Resolve(mod, STRL_SIGNS, 0);
			char *rem = KB_strlist_peek(list, sub_id);
			return rem;
		}
		break;
		case STRL_SIGNS:
		{
			char *buf = DOS_read_strings(mod, KBEXE_POS(exe_type, DOS_SIGNS));
			int len = KBEXE_LEN(exe_type, DOS_SIGNS);
			/* HACK -- do not process failed read_strings result */
			if (buf == NULL) len = 0;
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
			return KB_strlist_peek(DOS_Resolve(mod, STRL_VNAMES, 0), sub_id);
		}
		break;
		case STRL_VNAMES:
		{
			char *buf = DOS_read_strings(mod, KBEXE_POS(exe_type, DOS_VNAMES));
			return buf;
			//int ptroff = 0x1842C;
		}
		break;
		case STR_VDESC:
		{
			return KB_strlist_peek(DOS_Resolve(mod, STRL_VDESCS, 0), sub_id);
		}
		break;
		case STRL_VDESCS:
		{
			int line = sub_id * 14;
			return DOS_read_vdescs(mod, KBEXE_POS(exe_type, DOS_VDESCS), line);
		}
		break;
		case STR_ADESC:
		{
			return KB_strlist_peek(DOS_Resolve(mod, STRL_ADESCS, 0), sub_id);
		}
		break;
		case STRL_ADESCS:
		{
			int line = sub_id * 5;
			return DOS_read_adescs(mod, KBEXE_POS(exe_type, DOS_ADESCS), line);
		}
		break;
		case STR_CREDIT:
		{
			return KB_strlist_peek(DOS_Resolve(mod, STRL_CREDITS, 0), sub_id);
		}
		break;
		case STRL_CREDITS:
		{
			return DOS_read_credits(mod, KBEXE_POS(exe_type, DOS_CREDITS));
		}
		break;
		case STR_ENDING: /* subId - string index (indexes above 100 indicate next group) */
		{
			/* A small string containing part of the ending message. */
			if (sub_id > 100)	/* Game lost */
				return KB_strlist_peek(DOS_Resolve(mod, STRL_ENDINGS, 1), sub_id - 100);
			else            	/* Game won */
				return KB_strlist_peek(DOS_Resolve(mod, STRL_ENDINGS, 0), sub_id);
		}
		break;
		case STRL_ENDINGS:
		{
			char *buf;
			/* Returns large buffer of \0-separated strings, forming an ending text */
			if (sub_id) /* Game lost */
				buf = DOS_read_strings(mod, KBEXE_POS(exe_type, DOS_GAME_LOST));
			else    	/* Game won */
				buf = DOS_read_strings(mod, KBEXE_POS(exe_type, DOS_GAME_WON));
			if (!buf) return NULL;
			/* Make adjustments */
			if (sub_id) { /* Game lost */
				buf = KB_strlist_replace(buf, 0, "Oh, %s the", 1);
				buf = KB_strlist_replace(buf, 1, "%s, you", 1);
			} else { /* Game won */
				buf = KB_strlist_replace(buf, 1, "%s the", 1);
				buf = KB_strlist_replace(buf, 2, "%s! You", 1);
			}
			KB_strlist_debug(buf);
			return buf;
			/* Note: this is extremely suboptimal way to do it...*/
			char *next;
			int max;
			int line = 0;
			int k;
			int len;

			max = KB_strlist_len(buf) + 10;
			next = malloc(sizeof(char) * max);
			len = 0;

			printf("Got strlist: %s\n", buf);
			for (k = 0; k < max; k++) {
				int mlen;
				char *one = KB_strlist_peek(buf, k);
				printf("next: %s\n", one);
				if (one == NULL && k == 1) {
					one = "%s";
				}
				mlen = strlen(one);
				memcpy(&next[len], one, mlen);
				len += mlen;
				next[len] = '\0';
				if (k == 0) {
					one = "%s";
					mlen = strlen(one);
					memcpy(&next[len], one, mlen);
					len += mlen;
					next[len] = '\0';
				}
			}
			return next;
		}
		break;
		case SN_TUNE:
		{
			return DOS_load_tune(mod, sub_id);
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
		break;
		case RECT_UITILE:
		{
			return &DOS_frame_tile;
		}
		break;
		case WDAT_VREWARD:
		{
			return DOS_word_array(mod, KBEXE_OFFSET(exe_type, DOS_VREWARDS), 17);
		}
		break;
		case DAT_MENUCONTROLS:
		{
			byte *opt = malloc(sizeof(byte) * 16);
			memset(opt, 0, 16);
			memset(opt, 1, 5);
			if (mod->bpp == 2) opt[5] = 1;
			return opt;
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
			f = KB_fopen_with("LAND.ORG", "rb", mod);
			if (f == NULL) return NULL;
			n = KB_fread(world, sizeof(byte), len, f);
			KB_fclose(f);
			return world;
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

