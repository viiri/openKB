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

#define KB_DIR_IF_REP_ADD(SUFFIX) \
		&KB_opendir ## SUFFIX ## _in, \
		&KB_seekdir ## SUFFIX , \
		&KB_telldir ## SUFFIX , \
		&KB_readdir ## SUFFIX , \
		&KB_closedir ## SUFFIX

KB_DirDriver KB_DIRS[MAX_KBDTYPE] = {
	{
		KBDTYPE_DIR,
		KB_DIR_IF_REP_ADD(D),
	},
	{
		KBDTYPE_GRPCC,
		KB_DIR_IF_REP_ADD(CC),
	},
	{
		KBDTYPE_GRPIMG,
		KB_DIR_IF_REP_ADD(IMG),
	},
};

/* Open a directory [from within another directory] */
KB_DIR * KB_opendir_in(const char *filename, KB_DIR *top)
{
	unsigned int type = KBDTYPE_DIR;

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

	return (KB_DIRS[type].opendir_in)(filename, top);
}

struct KB_Entry * KB_readdir(KB_DIR *dirp)
{
	return (KB_DIRS[dirp->type].readdir)(dirp);
}

long KB_telldir(KB_DIR *dirp)
{
	return (KB_DIRS[dirp->type].telldir)(dirp);
}

void KB_seekdir(KB_DIR *dirp, long loc)
{
	(void)(KB_DIRS[dirp->type].seekdir)(dirp, loc);
}

void KB_rewinddir(KB_DIR *dirp)
{
	KB_seekdir(dirp, 0);
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

	ret = (KB_DIRS[dirp->type].closedir)(dirp);

	if (!ret && prev) {
		prev->ref_count--;
		if (prev->ref_count == 0)
			ret = KB_closedir(prev);
	}

	return ret;
}



/* "Real Directory" wrapper */
KB_DIR * KB_opendirD_in(const char *filename, KB_DIR *wth)
{
	KB_DIR * dirp;
	
	if (wth != NULL) return NULL;

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

long KB_telldirD(KB_DIR *dirp)
{
	return telldir(dirp->d);
}

void KB_seekdirD(KB_DIR *dirp, long loc)
{
	seekdir(dirp->d, loc);
}

int KB_closedirD(KB_DIR *dirp)
{
	closedir((DIR*)dirp->d);
	free(dirp);
}
