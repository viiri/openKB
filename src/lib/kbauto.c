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
#include "kbauto.h"
#include "kbfile.h" //:/
#include "kbdir.h"

#include "kbconf.h"

KBmodule *main_module = NULL;

#define KBTYPE_ROM		0x0F

#define KBTYPE_DSK  	0x01
#define KBTYPE_GROUP	0x02
#define KBTYPE_CMP  	0x03
#define KBTYPE_RAW_4	0x04
#define KBTYPE_RAW_16	0x05
#define KBTYPE_RAW_256	0x06



int FileSize( const char * szFileName ) 
{ 
  struct stat fileStat; 
  int err = stat( szFileName, &fileStat ); 
  if (0 != err) return 0; 
  return fileStat.st_size; 
}

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
		"416", "CC", 0,//340961, 
		"\0", 0, 0,
		0,
		KBFAMILY_DOS, KBTYPE_GROUP,
	},
	{ 
		"256", "CC", 0,//281550, 
		"\0", 0, 0, 
		-1,
		KBFAMILY_DOS, KBTYPE_GROUP,
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

//	printf("Must test: %s\n", path);
	int ok=-1;
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
		if (fp->filesize != 0) { /* exact size test */
			if (FileSize(name) != fp->filesize) continue;
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
				if (fp->sign[n] != buf[n]) { ok = 0; break; }
			}
			if (!ok) continue;
		}
		printf("-%s-\n", name);
		ok = j;		
	}
	return ok;
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
	
	memcpy(&conf->modules[conf->num_modules++], mod, sizeof(KBmodule)); 

}

void register_modules(KBconfig *conf) {

	

}

/*
 * Ideally, this function should take look at the files
 * presented in the directory and make intelligent guesses
 * about available modules.
 *
 * It does nothing of the sort...
 * TODO!!!
 */ 
void discover_modules(const char *path, KBconfig *conf) {

	struct KBfileid result[255]; 
	int	n_result = 0;

	printf("Performing module auto-discovery on path: %s\n", path);

    DIR             *dip;
    struct dirent   *dit;
 
    int             i = 0;

    /* DIR *opendir(const char *name);
     *
     * Open a directory stream to argv[1] and make sure
     * it's a readable and valid (directory) */
    if ((dip = opendir(path)) == NULL)
    {
            perror("opendir");
            return;
    }
 
    printf("Directory stream is now open\n");
 
    /*  struct dirent *readdir(DIR *dir);
     *
     * Read in the files from argv[1] and print */
    while ((dit = readdir(dip)) != NULL)
    {
            i++;
            char full_path[PATH_MAX];
            //printf("\n%s", dit->d_name);
            if (dit->d_name[0] == '.') continue;
            
            strcpy(full_path, path);
            strcat(full_path, dit->d_name);
            int ok = verify_file(dit->d_name, full_path);
            if (ok >= 0) {
            	strcpy(result[n_result].filename, dit->d_name);
            	n_result++;
            }
    }

    printf("\n\nreaddir() found a total of %i files\n", i);

    /* int closedir(DIR *dir);
     *
     * Close the stream to argv[1]. And check for errors. */
    if (closedir(dip) == -1)
    {
            perror("closedir");
            return;
    }
    printf("\nDirectory stream is now closed\n");


	/* Ignore everything we found and just dump those: */

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


void* DOS_Resolve(int id, int sub_id) {

	char *middle_name;
	char *suffix;
	char *ident;

	int row_start = 0;
	int row_frames = 4;

	int method = -1;
	#define RAW_IMG	0
	#define RAW_CH	1
	#define ROW_IMG	2

	char *bpp_names[] = {
		NULL, ".CH", ".4",//0, 1, 2
		NULL, ".16",	//3, 4
		NULL, NULL, 	//5,6
		NULL, ".256",	//7, 8
	};

	KBmodule *mod;

	mod = main_module;

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
		}
		break;
		case GR_FONT:
		{
			/* KB font */
			method = RAW_CH;
			middle_name = "KB";
			suffix = bpp_names[1];
			ident = "";
		}
		break;
		case GR_TROOP:	/* subId - troop index */
		{
			/* A troop (with animation) */
			method = ROW_IMG;
			middle_name = troop_names[sub_id];
			suffix = bpp_names[mod->bpp];
			ident = "";
		}
		break;
		case GR_TILE:	/* subId - tile index */
		{
			/* A tile */
			method = RAW_IMG;
			if (sub_id > 36) {
				sub_id -= 36;
				middle_name = "tilesetb";
			} else {
				middle_name = "tileseta";
			}
			suffix = bpp_names[mod->bpp];
			sprintf(ident, "%d", sub_id); 
		}
		break;
		case GR_TILESET:	/* subId - continent */
		{
			/* This one must be assembled */
		}
		break;
		case GR_VILLAIN:	/* subId - villain index */
		{
			/* A villain (with animation) */
			method = ROW_IMG;
			middle_name = villain_names[sub_id];
			suffix = bpp_names[mod->bpp];
			ident = "";
		}
		break;
		case GR_LOCATION:	/* subId - 0 home 1 town 2 - 6 dwelling */

		break;		
		case GR_UI:	/* subId - element index */

		break;
		default: break;
	}

	switch (method) {
		case ROW_IMG:
		case RAW_IMG:
		case RAW_CH:
		{
			SDL_Surface *surf = NULL;
			KB_File *f;
			char realname[1024];
			char buf[0xFA00];
			int n;

			realname[0] = '\0';
			KB_strcat(realname, middle_name);
			KB_strcat(realname, suffix);
			KB_strcat(realname, ident);

			switch (method) {
				case RAW_IMG:
				case RAW_CH:
				{
					printf("? DOS IMG FILE: %s\n", realname);
					f = KB_fopen_with(realname, "rb", mod);
					if (f == NULL) return NULL;
					n = KB_fread(&buf[0], sizeof(char), 0xFA00, f);
					KB_fclose(f);

					if (method == RAW_IMG)
						surf = SDL_loadRAWIMG(&buf[0], n, mod->bpp);
					else
						surf = SDL_loadRAWCH (&buf[0], n);
				}
				break;
				case ROW_IMG:	
				{
					KB_DIR *d = KB_opendir_with(realname, mod);

					if (d == NULL) { printf("Having trouble with dir %s\n", realname); return NULL; }				

					surf = SDL_loadROWIMG(d, row_start, row_frames, mod->bpp);	
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

void* GNU_Resolve(int id, int sub_id) {

	char *image_name = NULL;
	char *image_suffix = NULL;
	char *image_subid = "";
	int is_transparent = 1;

	KBmodule *mod;

	mod = main_module;

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

		printf("? FREE IMG FILE: %s\n", realname);

		SDL_Surface *surf = IMG_Load(realname);

		if (surf == NULL) printf("> FAILED TO OPEN, %s\n", IMG_GetError());

		if (surf && is_transparent)
			SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);

		return surf;
	}

	return NULL;
}

void* KB_Resolve(int id, int sub_id) {
	/* This could be a callback... */
	switch (main_module->kb_family) {
		case KBFAMILY_GNU: return GNU_Resolve(id, sub_id);
		case KBFAMILY_DOS: return DOS_Resolve(id, sub_id);
		default: break;
	}
	return NULL;
}

