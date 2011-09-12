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

#include "kbres.h"
#include "kbauto.h"
#include "kbfile.h" //:/
#include "kbdir.h"
KBmodule *main_module = NULL;
KBmodule modules[MAX_MODULES];
int num_modules = 0;

#define KBFAMILY_GNU	0x0F
#define KBFAMILY_DOS	0x0D
#define KBFAMILY_MD 	0x02

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
		NULL, 0, 0,
		0,
		KBFAMILY_DOS, KBTYPE_GROUP,
	},
	{ 
		"256", "CC", 0,//281550, 
		NULL, 0, 0, 
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

void discover_modules(const char *path) {

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
            return 0;
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
            return 0;
    }
    printf("\nDirectory stream is now closed\n");

	/* ADD Modules */
	
	/* GNU */
	strcpy(modules[0].name, "Free");
	strcpy(modules[0].slotA_name, path);
	strcat(modules[0].slotA_name, "free/");
	modules[0].kb_family = KBFAMILY_GNU;
	num_modules++;

	/* Commodore64 */
	strcpy(modules[1].name, "Commodore64");
	strcpy(modules[1].slotA_name, ".D64#");
	num_modules++;

	/* Apple][ */
	strcpy(modules[2].name, "Apple][");
	strcpy(modules[2].slotA_name, ".dsl#");
	num_modules++;

	/* DOS */
	strcpy(modules[3].name, "DOS (Hercules)");
	strcpy(modules[3].slotA_name, "416.CC#");
	modules[3].slotA = NULL;
	modules[3].kb_family = KBFAMILY_DOS;
	modules[3].bpp = 1;
	num_modules++;

	strcpy(modules[4].name, "DOS (CGA)");
	strcpy(modules[4].slotA_name, path);
	strcat(modules[4].slotA_name, "416.CC#");
	modules[4].slotA = NULL;
	modules[4].kb_family = KBFAMILY_DOS;
	modules[4].bpp = 2;
	num_modules++;

	strcpy(modules[5].name, "DOS (EGA)");
	strcpy(modules[5].slotA_name, path);
	strcat(modules[5].slotA_name, "416.CC#");
	modules[5].kb_family = KBFAMILY_DOS;
	modules[5].bpp = 4;
	num_modules++;

	strcpy(modules[6].name, "DOS (VGA)");
	strcpy(modules[6].slotA_name, "416.CC#");
	strcpy(modules[6].slotB_name, "256.CC#");
	modules[6].kb_family = KBFAMILY_DOS;
	modules[6].bpp = 8;
	num_modules++;

	/* Amiga */
	strcpy(modules[7].name, "Amiga");
	strcpy(modules[7].slotA_name, ".ADF#");
	num_modules++;

	/* MegaDrive/Genesis */
	strcpy(modules[8].name, "MegaDrive");
	strcpy(modules[8].slotA_name, ".BIN#");
	num_modules++;

	/* MacOS */
	strcpy(modules[9].name, "MacOS (B&W)");
	strcpy(modules[9].slotA_name, ".BIN#");
	num_modules++;

	strcpy(modules[10].name, "MacOS (Color)");
	strcpy(modules[10].slotA_name, ".BIN#");
	num_modules++;


	/* List modules */
	for (i = 0; i < num_modules;i++) {
		printf("Module %d: %s\n", i, modules[i].name);
	}


	for (i = 0; i < n_result;i++) {
		printf("File %d: %s\n", i, result[i].filename);
	}

    return 1;
}

KB_File *KB_fopen_with(const char *filename, char *mode, KBmodule *mod) {
	int i;
	char buffer[1024];
	KB_File *f = NULL;

	char *buf_ptr[3] = {
		&mod->slotA_name,
		&mod->slotB_name,
		&mod->slotC_name,
	};

	KB_DIR *dir_ptr[3] = {
		&mod->slotA,
		&mod->slotB,
		&mod->slotC,
	};

	for (i = 0; i < 3; i++) {
		buffer[0] = '\0';
		strcpy(buffer, buf_ptr[i] );
		strcat(buffer, filename);

		f = KB_fopen_in(buffer, "rb", dir_ptr[i]);
		if (f) break;
	}
	return f;
}

void* DOS_Resolve(int id, int sub_id) {

	char *middle_name;
	char *suffix;
	char *ident;

	int row_start = 0;
	int row_frames = 0;

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

	char *troop_names[] = {
		"peas",
	};

	char *villain_names[] = {
		"peas",
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
			strcat(realname, middle_name);
			strcat(realname, suffix);
			strcat(realname, ident);

			f = KB_fopen_with(realname, "rb", mod);

			n = KB_fread(buf, sizeof(char), 0xFA00, f);

			switch (method) {
				case RAW_IMG:	surf = SDL_loadRAWIMG(&buf[0], n, mod->bpp);	break;
				case RAW_CH:	surf = SDL_loadRAWCH (&buf[0], n);	break;
				case ROW_IMG:	surf = SDL_loadROWIMG(f, row_start, row_frames, mod->bpp);	break;
				default: break;
			}

			KB_fclose(f);

			return (void*)surf;
		}
		break;
	}
	return NULL;

}

void* GNU_Resolve(int id, int sub_id) {

printf("GNU RESOLVE: %d, %d\n", id, sub_id);

	char *image_name = NULL;

	KBmodule *mod;

	mod = main_module;

	switch (id) {
		case GR_LOGO:
		{
			image_name = "kblogo.png";
		}
		break;
		default: break;
	}

	if (image_name) {
	
		char realname[1024];

		realname[0] = '\0';
		strcpy(realname, mod->slotA_name);
		strcat(realname, image_name);
		
		printf("FILE: %s\n", realname);
		
		SDL_Surface *surf = IMG_Load(realname);
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

