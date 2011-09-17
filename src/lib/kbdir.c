/*
 *  kbdir.c -- Directory I/O
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
#include "malloc.h"
#include "string.h" 
 
#include <stdio.h>

#include <sys/types.h> 
#include <dirent.h>

#include "kbdir.h"
#include "kbfile.h"

KB_DIR * KB_opendir(const char *filename)
{
	return KB_opendir_in(filename, NULL);
}

/* Open a directory [from within another directory] */
KB_DIR * KB_opendir_in(const char *filename, KB_DIR *top)
{
	int type = KBDTYPE_DIR;

	int n = 0, e = 0;

	KB_DIR *middle = KB_follow_path(filename, &n, &e, top);

	if (middle != NULL) 
	{
		return KB_opendir_in(&filename[n + 1], middle);
	}

	const char *ext = &filename[e + 1];

	if (!strcasecmp(ext, "cc")) {
		type = KBDTYPE_GRPCC;
	}
	if (!strcasecmp(ext, "4")) {
		type = KBDTYPE_GRPIMG;
	}

	switch (type) {
		case KBDTYPE_DIR:	return KB_opendirD( filename );
		case KBDTYPE_GRPCC:	return KB_opendirCC_in( filename, top );
		case KBDTYPE_GRPIMG:return KB_opendirIMG_in( filename, top );				
	}

	fprintf(stderr, "Error! Can't open directory '%s' of unknown type!\n", filename);
	return NULL;
}

struct KB_Entry * KB_readdir(KB_DIR *dirp)
{
	switch (dirp->type) {
		case KBDTYPE_DIR:	return KB_readdirD( dirp );
		case KBDTYPE_GRPCC:	return KB_readdirCC( dirp );
		case KBDTYPE_GRPIMG:	return KB_readdirIMG( dirp );				
	}
	return NULL;
}

#if 0
int KB_readdir_r(KB_DIR *dirp, struct KB_Entry *entry, struct KB_Entry **result)
{
	switch (dirp->type) {
		case KBDTYPE_DIR:	return KB_readdir_rD( dirp , entry, result );
	}
	return 0;
}
KB_File * KB_eopen(KB_DIR *dirp, KB_Entry *entry)
{
	switch (dirp->type) {
		case KBDTYPE_DIR:	return KB_eopenD( dirp , entry );
		case KBDTYPE_GRPCC:	return KB_eopenCC( dirp , entry );		
	}
	return NULL;
}
#endif

long KB_telldir(KB_DIR *dirp)
{
	switch (dirp->type) {
		case KBDTYPE_DIR:	return KB_telldirD( dirp );
		case KBDTYPE_GRPCC:	return KB_telldirCC( dirp );
		case KBDTYPE_GRPIMG:	return KB_telldirIMG( dirp );				
	}
	return 0;
}

void KB_seekdir(KB_DIR *dirp, int loc)
{
	switch (dirp->type) {
		case KBDTYPE_DIR:	 KB_seekdirD( dirp, loc );
		case KBDTYPE_GRPCC:	 KB_seekdirCC( dirp, loc );
		case KBDTYPE_GRPIMG: KB_seekdirIMG( dirp, loc );				
	}
	return;
}

void KB_rewinddir(KB_DIR *dirp)
{

}

int KB_closedir(KB_DIR *dirp)
{
	int ret = -1;
	KB_DIR *prev;

	if (dirp->ref_count) {
		KB_errlog("Unable to close directory which has %d refrences.\n", dirp->ref_count);
		return -1;
	}

	prev = dirp->prev; 

	switch (dirp->type) {
		case KBDTYPE_DIR:	 ret = KB_closedirD( dirp ); break;
		case KBDTYPE_GRPCC:	 ret = KB_closedirCC( dirp ); break;
		case KBDTYPE_GRPIMG: ret = KB_closedirIMG( dirp ); break;				
	}

	if (!ret && prev) {
		prev->ref_count--;
		if (prev->ref_count == 0)
			ret = KB_closedir(prev);
	}

	return ret;
}



/* "Real Directory" wrapper */
KB_DIR * KB_opendirD(const char *filename)
{
	KB_DIR * dirp;

	DIR *d = opendir(filename);
	if (d == NULL) return NULL;

	dirp = malloc(sizeof(KB_DIR));
	if (dirp == NULL) return NULL;
	
	dirp->prev = NULL;

	dirp->type = KBDTYPE_DIR;
	dirp->d = (void*)d;
	dirp->ref_count = 0;

	return dirp;
}

struct KB_Entry * KB_readdirD(KB_DIR *dirp)
{
	struct dirent *dit;
	KB_Entry *entry = &dirp->dit;
	DIR *d = (DIR*)dirp->d;

    dit = readdir(dirp->d);
    if (dit == NULL) return NULL;

	strcpy(entry->d_name, dit->d_name);
	entry->d_ino = dit->d_ino;

	return entry;
}

int KB_readdir_rD(KB_DIR *dirp, struct KB_Entry *entry, struct KB_Entry **result)
{

}

long KB_telldirD(KB_DIR *dirp)
{
	return telldir(dirp->d);
}

void KB_seekdirD(KB_DIR *dirp, long loc)
{
	seekdir(dirp->d, loc);
}

void KB_rewinddirD(KB_DIR *dirp)
{

}

int KB_closedirD(KB_DIR *dirp)
{
	closedir((DIR*)dirp->d);
	free(dirp);
}

int KB_dirfdD(KB_DIR *dirp)
{

}
#if 0
KB_File * KB_eopenD(KB_DIR *dirp, KB_Entry *entry)
{
	return NULL;
}
#endif
