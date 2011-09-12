/*
 *  kbdir.h -- Directory I/O API (mimics STDIO opendir/readdir/seekdir/closedir)
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
/*
 * Provides an opaque filehandler "KB_DIR", used similarly to
 * stdio's "DIR". readdir-like functions provider dirent-like
 * structure "KB_Entry".
 */
/*
 * See also: kbfile.h - for the compatible File I/O API.  
 */
#ifndef _OPENKB_LIBKB_DIR
#define _OPENKB_LIBKB_DIR

#define KBDTYPE_DIR 	0x00

#define KBDTYPE_GRPCC 	0x10
#define KBDTYPE_GRPIMG 	0x11

#include "kbfile.h"

typedef struct KB_Entry {

	int  d_ino;
	char d_name[1024];

	union {
		struct {
			int w;
			int h;
			int bpp;
		} img;
		struct {
			int cmpSize;
			int fullSize;
		} cmp;
	} d_info;

} KB_Entry;

typedef struct KB_DIR {

	int type;
	void *d;
	long int pos;
	long int len;

	KB_Entry dit;

} KB_DIR;

extern void KB_seekdir(KB_DIR *dirp, int offset);
extern long KB_telldir(KB_DIR *dirp);

/* Real directory */
extern struct KB_Entry * KB_readdirD(KB_DIR *dirp);
extern int KB_readdir_rD(KB_DIR *dirp, struct KB_Entry *entry, struct KB_Entry **result);
extern struct KB_File * KB_eopenD(KB_DIR *dirp, struct KB_Entry *entry);

extern long KB_telldirD(KB_DIR *dirp);

extern long KB_telldirCC(KB_DIR *dirp);
extern long KB_telldirIMG(KB_DIR *dirp);

/* "CC" Group file */
extern KB_DIR * KB_opendirCC(const char *filename);
extern struct KB_Entry * KB_readdirCC(KB_DIR *dirp);
extern struct KB_File * KB_eopenCC(struct KB_DIR *dirp, struct KB_Entry *entry);
extern KB_DIR * KB_opendirCC_in(const char *filename, KB_DIR *dirp);
extern KB_DIR * KB_opendir_in(const char *filename, KB_DIR *dirp);

/* "IMG" Group file */

#endif	/* _OPENKB_LIBKB_DIR */
