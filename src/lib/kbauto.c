/*
 *  kbauto.c -- Module auto-discovery service
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
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <sys/stat.h> 

#include "SDL_image.h"

#include "kbstd.h"
#include "kbres.h"
#include "kbfile.h" //:/
#include "kbdir.h"

#include "kbconf.h"

#define KBTYPE_ROM		0x0F

#define KBTYPE_DSK  	0x01
#define KBTYPE_GROUP	0x02
#define KBTYPE_CMP  	0x03
#define KBTYPE_DIR  	0x07
#define KBTYPE_EXE  	0x08
#define KBTYPE_RAW_4	0x04
#define KBTYPE_RAW_16	0x05
#define KBTYPE_RAW_256	0x06


struct KBfileid {

	char filename[128];
	char file_ext[128];
	int filesize;

	char sign[128];
	int sign_len;
	int sign_off;	

	int companion;

	int kb_family;
	int kb_type;
};

struct KBfileid fingerprints[] = {
	{ 
		"free", "\0", 0,//TODO: make some file (COPYING?) inside the dir be the sign
		"\0", 0, 0,
		-1,
		KBFAMILY_GNU, KBTYPE_DIR,
	},
	{ 
		"416", "CC", 0,//340961, 
		"\0", 0, 0,
		1,
		KBFAMILY_DOS, KBTYPE_GROUP,
	},
	{ 
		"256", "CC", 0,//281550, 
		"\0", 0, 0, 
		0,
		KBFAMILY_DOS, KBTYPE_GROUP,
	},
	{
		"KB", "EXE", 0,//113718
		"Copyright (C) 1990-95 New World Computing, Inc", 46, 0x15EA5,
		2,
		KBFAMILY_DOS, KBTYPE_EXE,
	},	
	{
		"\0", "BIN", 0, 
		"SEGA", 4, 0x100, 
		-1,
		KBFAMILY_MD, KBTYPE_ROM,
	},
};

int fingerprints_num = sizeof(fingerprints) / sizeof(struct KBfileid);

void name_split(const char *name, char *base, char *ext) {
	int i = 0;
	int l = strlen(name);
	int dot = -1;
	
	for (i = l - 1; i > -1; i--) {
		if (name[i] == '.') {
			dot = i;
			break;
		}	
	}
	
	if (dot == -1) {
		strcpy(base, name);
		strcpy(ext, "");
	} else if (dot == 0) {
		strcpy(base, "");
		strcpy(ext, name + 1);
	} else {
		strcpy(base, name);
		base[dot] = '\0';
		strcpy(ext, name + dot +1);
	}
}

int verify_file(const char *name, const char *path) {

	int id = -1;
	int j;
	for (j = 0; j < fingerprints_num; j++) 
	{
		struct KBfileid *fp = &fingerprints[j];
		if (fp->filename[0] != '\0' || fp->file_ext[0] != '\0') {	/* name test */

			char base[NAME_MAX];
			char ext[NAME_MAX];

			name_split(name, base, ext);

			// todo: test basename and extension separately
			if (fp->filename[0] != '\0' && strcasecmp(fp->filename, base)) continue;
			if (fp->file_ext[0] != '\0' && strcasecmp(fp->file_ext, ext)) continue;

		}
		if (fp->kb_type == KBTYPE_DIR) { /* "is a directory" test */
			if (test_directory(path,0)) continue;
		}
		if (fp->filesize != 0) { /* exact size test */
			if (file_size(name) != fp->filesize) continue;
		}
		if (fp->sign_len != 0) { /* signature test */
			char buf[1024];
			int n, ok = 1;
			FILE *f;
			f = fopen(path, "rb");
			if (f == NULL) continue;
			fseek(f, fp->sign_off, 0);
			n = fread(buf, sizeof(char), fp->sign_len, f);
			fclose(f);
			if (n != fp->sign_len) continue;
			for (n = 0; n < fp->sign_len; n++) {
				if (fp->sign[n] != buf[n]) { ok = 0; break;	}
			}
			if (!ok) continue;
		}
		printf("-%s- [%d]\n", name, j);
		id = j;
	}
	return id;
}

void init_module(KBmodule *mod) {
	int i;
}

void init_modules(KBconfig *conf) {
	int i;
}
void stop_module(KBmodule *mod) {
	int i;
}
void stop_modules(KBconfig *conf) {
	int i;
}

void wipe_module(KBmodule *mod) {
	mod->slotA_name[0] = '\0';
	mod->slotB_name[0] = '\0';
	mod->slotC_name[0] = '\0';
	mod->slotA = NULL;
	mod->slotB = NULL;
	mod->slotC = NULL;
}

void register_module(KBconfig *conf, KBmodule *mod) {

	if (conf->num_modules >= MAX_MODULES) {
		KB_stdlog("Unable to add module '%s' %s , limit of %d reached\n", mod->name, mod->slotA, conf->num_modules);
		return;
	}
	
	memcpy(&conf->modules[conf->num_modules++], mod, sizeof(KBmodule)); 

}

void register_modules(KBconfig *conf) {

}

void add_module_aux(KBconfig *conf, const char *name, int family, int bpp, const char *path, const char *slotA, const char *slotB, const char *slotC) {

	int i = conf->num_modules;

	if (i >= MAX_MODULES) {
		KB_stdlog("Unable to add module '%s' %s , limit of %d reached\n", name, slotA, i);
		return;
	}

	wipe_module(&conf->modules[i]);
	KB_strcpy(conf->modules[i].name, name);

	KB_dircpy(conf->modules[i].slotA_name, path);
	KB_dirsep(conf->modules[i].slotA_name);
	KB_strcat(conf->modules[i].slotA_name, slotA);
	if (slotB) {
		KB_dircpy(conf->modules[i].slotB_name, path);
		KB_dirsep(conf->modules[i].slotB_name);
		KB_strcat(conf->modules[i].slotB_name, slotB);
	}
	if (slotC) {
		KB_dircpy(conf->modules[i].slotC_name, path);
		KB_dirsep(conf->modules[i].slotC_name);
		KB_strcat(conf->modules[i].slotC_name, slotC);
	}
	conf->modules[i].slotA = NULL;
	conf->modules[i].slotB = NULL;
	conf->modules[i].slotC = NULL;

	conf->modules[i].kb_family = family;
	conf->modules[i].bpp = bpp;
	conf->num_modules++;

		KB_debuglog(1, "Module: %s\n", conf->modules[i].name);
		KB_debuglog(1, "SlotA: %s\n", conf->modules[i].slotA_name);
		KB_debuglog(0, "SlotB: %s\n", conf->modules[i].slotB_name);
		KB_debuglog(0, "SlotC: %s\n", conf->modules[i].slotC_name);
		KB_debuglog(-2, NULL, NULL);

}

/*
 * This function takes a look at the files presented in the directory
 *  and makes intelligent guesses about available modules.
 */ 
void discover_modules(const char *path, KBconfig *conf) {

	#define MAX_RFILES 255

	struct KBfileid result[MAX_RFILES]; 
	int	n_result = 0;

    KB_DIR *dir;
    KB_Entry *entry;

    int i = 0;

	KB_stdlog("Performing module auto-discovery on path: %s\n", path);

    if ((dir = KB_opendir(path)) == NULL) {
    	KB_errlog("[auto] Can't access path '%s'\n", path);
    	return;
    }

    while ((entry = KB_readdir(dir)) != NULL) {
		int ok;
        char full_path[PATH_LEN];    

		i++;

		if (entry->d_name[0] == '.') continue; /* Ignore hidden files and . .. */

		KB_dircpy(full_path, path);
		KB_dirsep(full_path);
		KB_strcat(full_path, entry->d_name);

		ok = verify_file(entry->d_name, full_path);

		if (ok >= 0) {
			KB_strcpy(result[n_result].filename, entry->d_name);
			if (fingerprints[ok].kb_type == KBTYPE_GROUP) KB_strcat(result[n_result].filename, "#");
			result[n_result].kb_family = fingerprints[ok].kb_family;
			result[n_result].kb_type = fingerprints[ok].kb_type;
			result[n_result].companion = fingerprints[ok].companion;
			n_result++;
			if (n_result >= MAX_RFILES) {
				KB_stdlog("Found over %d files, ignoring the rest :(\n", n_result);
				break;
			}
		}
    }
	KB_closedir(dir);

	if (!n_result) {
		KB_stdlog("Found NO interesting files (out of %d), are you sure that is the correct directory?\n", i);
		return;
	}

    if (n_result)
		KB_stdlog("Found %d interesting file(s), going to analyze them in more detail.\n", n_result);

	int had_modules = conf->num_modules;

	/* "auto" discover Free modules */
	for (i = 0; i < n_result; i++) {
		struct KBfileid *id = &result[i];
		/* 'i'll take it' */
		if (id->kb_family == KBFAMILY_GNU) {
			add_module_aux(conf, "Free", KBFAMILY_GNU, 0, path, result[i].filename, NULL, NULL);
		}
	}

	/* "auto" discover Dos modules */
	/* traverse files noting those inbetween */
	int dosGRP1 = -1;
	int dosGRP2 = -1;
	int dosEXE = -1;
	for (i = 0; i < n_result; i++) {
		struct KBfileid *id = &result[i];
		if (id->kb_family == KBFAMILY_DOS) {
		
			if (id->companion == 0) dosGRP1 = i;
			if (id->companion == 1) dosGRP2 = i;
			if (id->companion == 2) dosEXE = i;

			/* As soon as 2 are assembled, register as module */
			if (dosGRP2 != -1 && dosEXE != -1) {

				/* Better to have 3 */
				if (dosGRP1 != -1)	{		
					add_module_aux(conf, "DOS (VGA)", KBFAMILY_DOS, 8, path, 
						result[dosGRP1].filename, result[dosGRP2].filename, result[dosEXE].filename);
				}

				/* But 2 are good enougth for a module too */ 
				add_module_aux(conf, "DOS (Hercules)", KBFAMILY_DOS, 1, path, result[dosGRP2].filename, NULL, NULL);
				add_module_aux(conf, "DOS (CGA)", KBFAMILY_DOS, 2, path, result[dosGRP2].filename, NULL, NULL);
				add_module_aux(conf, "DOS (EGA)", KBFAMILY_DOS, 4, path, result[dosGRP2].filename, NULL, NULL);
				
				if (dosGRP1 != -1) {
					/* Reset groups */
					dosGRP1 = -1;
					dosGRP2 = -1;
				}
			}
			
		}
	}

	/* "auto" discover Sega modules */
	for (i = 0; i < n_result; i++) {
		struct KBfileid *id = &result[i];
		/* 'i'll take it' */
		if (id->kb_family == KBFAMILY_MD) {
			add_module_aux(conf, "MegaDrive", KBFAMILY_MD, 0, path, result[i].filename, NULL, NULL);
		}
	}


	/* Say something after all this. */
	if (conf->num_modules - had_modules > 0) {
		KB_stdlog("Discovered %d modules. Some might be incorrect.\n", conf->num_modules - had_modules);
	} else {
		KB_stdlog("Discovered NO modules out of %d interesting files :(\n", n_result);
	}

	return;

	/* Phew... Delete thos very very soon */

	/* GNU */
	wipe_module(&conf->modules[0]);
	KB_strcpy(conf->modules[0].name, "Free");
	KB_dircpy(conf->modules[0].slotA_name, path);
	KB_dirsep(conf->modules[0].slotA_name);
	KB_strcat(conf->modules[0].slotA_name, "free");
	KB_dirsep(conf->modules[0].slotA_name);
	conf->modules[0].kb_family = KBFAMILY_GNU;
	conf->num_modules++;

	/* Commodore64 */
	strcpy(conf->modules[1].name, "Commodore64");
	strcpy(conf->modules[1].slotA_name, ".D64#");
	conf->num_modules++;

	/* Apple][ */
	strcpy(conf->modules[2].name, "Apple][");
	strcpy(conf->modules[2].slotA_name, ".dsl#");
	conf->num_modules++;

	/* DOS */
	strcpy(conf->modules[3].name, "DOS (Hercules)");
	strcpy(conf->modules[3].slotA_name, "416.CC#");
	conf->modules[3].slotA = NULL;
	conf->modules[3].kb_family = KBFAMILY_DOS;
	conf->modules[3].bpp = 1;
	conf->num_modules++;

	wipe_module(&conf->modules[4]);
	KB_strcpy(conf->modules[4].name, "DOS (CGA)");
	KB_dircpy(conf->modules[4].slotA_name, path);
	KB_dirsep(conf->modules[4].slotA_name);
	KB_strcat(conf->modules[4].slotA_name, "416.CC#");
	conf->modules[4].slotA = NULL;
	conf->modules[4].kb_family = KBFAMILY_DOS;
	conf->modules[4].bpp = 2;
	conf->num_modules++;

	strcpy(conf->modules[5].name, "DOS (EGA)");
	strcpy(conf->modules[5].slotA_name, path);
	strcat(conf->modules[5].slotA_name, "416.CC#");
	conf->modules[5].kb_family = KBFAMILY_DOS;
	conf->modules[5].bpp = 4;
	conf->num_modules++;

	strcpy(conf->modules[6].name, "DOS (VGA)");
	strcpy(conf->modules[6].slotA_name, "416.CC#");
	strcpy(conf->modules[6].slotB_name, "256.CC#");
	conf->modules[6].kb_family = KBFAMILY_DOS;
	conf->modules[6].bpp = 8;
	conf->num_modules++;

	/* Amiga */
	strcpy(conf->modules[7].name, "Amiga");
	strcpy(conf->modules[7].slotA_name, ".ADF#");
	conf->num_modules++;

	/* MegaDrive/Genesis */
	strcpy(conf->modules[8].name, "MegaDrive");
	strcpy(conf->modules[8].slotA_name, ".BIN#");
	conf->num_modules++;

	/* MacOS */
	strcpy(conf->modules[9].name, "MacOS (B&W)");
	strcpy(conf->modules[9].slotA_name, ".BIN#");
	conf->num_modules++;

	strcpy(conf->modules[10].name, "MacOS (Color)");
	strcpy(conf->modules[10].slotA_name, ".BIN#");
	conf->num_modules++;

	/* List modules */
	for (i = 0; i < conf->num_modules;i++) {
		printf("Module %d: %s\n", i, conf->modules[i].name);
	}

	for (i = 0; i < n_result;i++) {
		printf("File %d: %s\n", i, result[i].filename);
	}

    return;
}

KB_DIR *KB_opendir_with(const char *filename, KBmodule *mod) {
	int i;
	char buffer[PATH_MAX];
	KB_DIR *d = NULL;

	char *buf_ptr[3] = {
		mod->slotA_name,
		mod->slotB_name,
		mod->slotC_name,
	};

	KB_DIR *dir_ptr[3] = {
		mod->slotA,
		mod->slotB,
		mod->slotC,
	};

	for (i = 0; i < 3; i++) {
		buffer[0] = '\0';
		strcpy(buffer, buf_ptr[i] );
		strcat(buffer, filename);

		d = KB_opendir_in(buffer, dir_ptr[i]);

		if (d) break;
	}
	return d;
}

KB_File *KB_fopen_with(const char *filename, char *mode, KBmodule *mod) {
	int i;
	char buffer[PATH_MAX];
	KB_File *f = NULL;

	char *buf_ptr[3] = {
		mod->slotA_name,
		mod->slotB_name,
		mod->slotC_name,
	};

	KB_DIR *dir_ptr[3] = {
		mod->slotA,
		mod->slotB,
		mod->slotC,
	};

	for (i = 0; i < 3; i++) {
		if (buf_ptr[i][0] == '\0') break;
		KB_dircpy(buffer, buf_ptr[i]);
		KB_dirsep(buffer);
		KB_strcat(buffer, filename);

		f = KB_fopen_in(buffer, "rb", dir_ptr[i]);
		if (f) break;
	}
	return f;
}

char *troop_names[] = {
    "peas","spri","mili","wolf","skel","zomb","gnom","orcs","arcr","elfs",
    "pike","noma","dwar","ghos","kght","ogre","brbn","trol","cavl","drui",
    "arcm","vamp","gian","demo","drag",
};

char *villain_names[] = {
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
			middle_name = troop_names[sub_id];
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
			middle_name = villain_names[sub_id];
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
		case GR_LOCATION:	/* subId - 0 home 1 town 2 - 6 dwelling */

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
		case GR_TROOP:
		{
			image_name = troop_names[sub_id];
			image_suffix = ".png";
		}
		break;
		case GR_COMTILES:
		{
			image_name = "comtiles";
			image_suffix = ".png";
			is_transparent = 0;
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
