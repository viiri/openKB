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
	
	int ref_count;

} KB_DIR;


extern KB_DIR * KB_follow_path( const char * filename, int *n, int *e, KB_DIR *top );

/* API */
extern KB_DIR * KB_opendir(const char *filename);
extern void KB_seekdir(KB_DIR *dirp, int offset);
extern long KB_telldir(KB_DIR *dirp);
extern KB_Entry * KB_readdir(KB_DIR *dirp);

extern KB_DIR * KB_opendir_in(const char *filename, KB_DIR *dirp);
extern KB_File * KB_fopen_in ( const char * filename, const char * mode, KB_DIR * in );

/* Real directory */
extern KB_DIR * KB_opendirD(const char *filename);
extern void KB_seekdirD(KB_DIR *dirp, long offset);
extern long KB_telldirD(KB_DIR *dirp);
extern KB_Entry * KB_readdirD(KB_DIR *dirp);


/* "CC" Group file */
extern KB_DIR * KB_opendirCC(const char *filename);
extern long KB_telldirCC(KB_DIR *dirp);
extern KB_Entry * KB_readdirCC(KB_DIR *dirp);
extern KB_DIR * KB_opendirCC_in(const char *filename, KB_DIR *dirp);
extern KB_File * KB_fopenCC_in( const char * filename, const char * mode, KB_DIR *dirp );

/* "IMG" Group file */
extern KB_DIR* KB_opendirIMG(const char *filename);
extern long KB_telldirIMG(KB_DIR *dirp);
extern KB_Entry* KB_readdirIMG(KB_DIR *dirp);
extern KB_DIR* KB_opendirIMG_in(const char *filename, KB_DIR *dirp);
extern KB_File* KB_fopenIMG_in ( const char * filename, const char * mode, KB_DIR * in );

#endif	/* _OPENKB_LIBKB_DIR */
