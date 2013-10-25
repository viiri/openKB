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

	char *path;
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
		"KB", "EXE", 79839 /* bytes */,
		"\0", 0, 0,
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

/* Returns fingerprint id or -1 */
int verify_file(const char *name, const char *path) {

	int id = -1;
	int j;
	for (j = 0; j < fingerprints_num; j++) 
	{
		struct KBfileid *fp = &fingerprints[j];
		if (fp->filename[0] != '\0' || fp->file_ext[0] != '\0') {	/* name test */

			char base[NAME_MAX];
			char ext[NAME_MAX];

			name_nsplit(name, base, NAME_MAX, ext, NAME_MAX);

			// todo: test basename and extension separately
			if (fp->filename[0] != '\0' && strcasecmp(fp->filename, base)) continue;
			if (fp->file_ext[0] != '\0' && strcasecmp(fp->file_ext, ext)) continue;

		}
		if (fp->kb_type == KBTYPE_DIR) { /* "is a directory" test */
			if (test_directory(path,0)) continue;
		}
		if (fp->filesize != 0) { /* exact size test */
			if (file_size(path) != fp->filesize) continue;
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

extern void DOS_Init(KBmodule *mod);
extern void DOS_Stop(KBmodule *mod);


void init_module(KBmodule *mod) {
	switch (mod->kb_family) {
//		case KBFAMILY_GNU: GNU_Init(mod); break;
		case KBFAMILY_DOS: DOS_Init(mod); break;
//		case KBFAMILY_MD:  MD_Init(mod); break;
		default: break;
	}
}
void init_modules(KBconfig *conf) {
	int i, l;
	i = conf->module;
	l = (conf->num_modules * conf->fallback) + ((i+1) * (1-conf->fallback));
	for (; i < l; i++) init_module(&conf->modules[i]);
}

void stop_module(KBmodule *mod) {
	switch (mod->kb_family) {
//		case KBFAMILY_GNU: GNU_Stop(mod); break;
		case KBFAMILY_DOS: DOS_Stop(mod); break;
//		case KBFAMILY_MD:  MD_Stop(mod); break;
		default: break;
	}
}
void stop_modules(KBconfig *conf) {
	int i, l;
	i = conf->module;
	l = (conf->num_modules * conf->fallback) + ((i+1) * (1-conf->fallback));
	for (; i < l; i++) stop_module(&conf->modules[i]);
}

void wipe_module(KBmodule *mod) {
	mod->slotA_name[0] = '\0';
	mod->slotB_name[0] = '\0';
	mod->slotC_name[0] = '\0';
	mod->slotA = NULL;
	mod->slotB = NULL;
	mod->slotC = NULL;
	mod->cache = NULL;
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
int fill_fingerprints(const char *path, KBconfig *conf, struct KBfileid *result, int n_result, int max_results, int *num_files) {
	KB_DIR *dir;
	KB_Entry *entry;

	int i = 0;

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
			result[n_result].path = KB_strdup(path);
			n_result++;
			if (n_result >= max_results) {
				KB_stdlog("Found over %d files, ignoring the rest :(\n", n_result);
				break;
			}
		}
		/* However, what if it's a subdirectory with more cool files? */
		else {
			if (entry->d_type == KBETYPE_DIR) {
				n_result = fill_fingerprints(full_path, conf, result, n_result, max_results, num_files);
			}
			continue;
		}

	}
	KB_closedir(dir);
	*num_files += i;
	return n_result;
}

void discover_modules(const char *path, KBconfig *conf) {

	#define MAX_RFILES 255

	struct KBfileid result[MAX_RFILES]; 
	int	n_result = 0;
	int	num_files = 0;
	int i;

	KB_stdlog("Performing module auto-discovery on path: %s\n", path);

	// KBfileid *result, int n_results, int max_results)

	n_result = fill_fingerprints(path, conf, &result[0], 0, MAX_RFILES, &num_files);

	#undef MAX_RFILES

	if (!n_result) {
		KB_stdlog("Found NO interesting files (out of %d), are you sure that is the correct directory?\n", num_files);
		return;
	}

	if (n_result) {
		KB_stdlog("Found %d interesting file(s), going to analyze them in more detail.\n", n_result);
	}

	int had_modules = conf->num_modules;

	/* "auto" discover Free modules */
	for (i = 0; i < n_result; i++) {
		struct KBfileid *id = &result[i];
		/* 'i'll take it' */
		if (id->kb_family == KBFAMILY_GNU) {
			add_module_aux(conf, "Free", KBFAMILY_GNU, 0, result[i].path, result[i].filename, NULL, NULL);
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
					add_module_aux(conf, "DOS (VGA)", KBFAMILY_DOS, 8, result[i].path,
						result[dosGRP1].filename, result[dosGRP2].filename, "");
				}

				/* But 2 are good enougth for a module too */ 
				add_module_aux(conf, "DOS (Hercules)", KBFAMILY_DOS, 1, result[i].path, result[dosGRP2].filename, "", NULL);
				add_module_aux(conf, "DOS (CGA)", KBFAMILY_DOS, 2, result[i].path, result[dosGRP2].filename, "", NULL);
				add_module_aux(conf, "DOS (EGA)", KBFAMILY_DOS, 4, result[i].path, result[dosGRP2].filename, "", NULL);
				
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
			add_module_aux(conf, "MegaDrive", KBFAMILY_MD, 0, result[i].path, result[i].filename, NULL, NULL);
		}
	}

	/* Free results */
	for (i = 0; i < n_result; i++) {
		free(result[i].path);
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

/* Case-insensitive version of KB_fopen_with */
KB_File *KB_fcaseopen_with(const char *filename, char *mode, KBmodule *mod) {
	int i;
	char buffer[PATH_MAX];
	int done, close = 0;
	KB_File *f = NULL;
	KB_DIR *dirp = NULL;
	KB_Entry *e;

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

		done = 0;
		close = 0;
		dirp = dir_ptr[i];//KB_opendir(path);

		if (dirp == NULL) {
			close = 1;
			KB_debuglog(0, "Dir not pre-opened '%s'\n", buf_ptr[i]);
			dirp = KB_opendir(buf_ptr[i]);
			if (dirp == NULL) continue;
		}

		while ((e = KB_readdir(dirp)) != NULL) {
			//KB_debuglog(0, "Matching: %s with exp %s\n", e->d_name, filename);
			if (!strcasecmp(filename, e->d_name)) {

				KB_dircpy(buffer, buf_ptr[i]);
				KB_dirsep(buffer);
				KB_strcat(buffer, e->d_name);

				f = KB_fopen_in(buffer, "rb", dir_ptr[i]);
				if (f) { done = 1; break; }
			}
		}
		//if (close) KB_closedir(dirp);
		if (done) break;
	}

	return f;
}

/* Case-insensitive verify that filename exists in path, put matched version into match  */
int match_file(const char *path, const char *filename, char *match) {
	int i, l;
	int found;
	KB_DIR *dirp;
	KB_Entry *e;

	found = 0;

	dirp = KB_opendir(path);
	if (dirp == NULL) return 0;

	l = strlen(filename) + 1;

	while ((e = KB_readdir(dirp)) != NULL) {
		//KB_debuglog(0, "Matching: %s with exp %s\n", e->d_name, filename);
		if (strcmp(filename, e->d_name)) {
			KB_strncpy(match, filename, l);
			found = 2;
			break;
		}
		if (!found && !strcasecmp(filename, e->d_name)) {
			KB_strncpy(match, e->d_name, l);
			found = 1;
		}
	}

	KB_closedir(dirp);
	return found;
}
