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

#define MAX_KBDTYPE	3

#define KBDTYPE_DIR 	0x00

#define KBDTYPE_GRPCC 	0x01
#define KBDTYPE_GRPIMG 	0x02

//#include "kbfile.h"

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

typedef struct KB_DIR KB_DIR;

struct KB_DIR {

	int type;
	void *d;
	long int pos;
	long int len;

	KB_Entry dit;
	
	int ref_count;
	
	KB_DIR *prev;
};

typedef struct KB_DirDriver {
	
	int type;	

	void*	(*loaddir)(const char *filename, KB_DIR *top, int *dirp);
	void	(*seekdir)(KB_DIR *dirp, long offset);
	long	(*telldir)(KB_DIR *dirp);
	KB_Entry*	(*readdir)(KB_DIR *dirp);
	int 	(*closedir)(KB_DIR *dirp);

} KB_DirDriver;

extern KB_DIR * KB_follow_path( const char * filename, int *n, int *e, KB_DIR *top );

/* API */
#define KB_opendir(FILENAME) KB_opendir_in(FILENAME, NULL)
extern KB_DIR * KB_opendir_in(const char *filename, KB_DIR *dirp);
extern void KB_seekdir(KB_DIR *dirp, long offset);
extern long KB_telldir(KB_DIR *dirp);
extern KB_Entry * KB_readdir(KB_DIR *dirp);
extern int KB_closedir(KB_DIR *dirp);

/* Interfaces */
#define KB_DIR_IF_NAME_ADD(SUFFIX) \
extern void* KB_loaddir ## SUFFIX (const char *filename, KB_DIR *top, int *dirp); \
extern void KB_seekdir ## SUFFIX  (KB_DIR *dirp, long offset); \
extern long KB_telldir ## SUFFIX  (KB_DIR *dirp); \
extern KB_Entry * KB_readdir ## SUFFIX  (KB_DIR *dirp); \
extern int KB_closedir ## SUFFIX  (KB_DIR *dirp);
/* Real Directory ("D") */
KB_DIR_IF_NAME_ADD(D);
/* CC Group File ("CC") */
KB_DIR_IF_NAME_ADD(CC);
/* IMG Group File ("IMG") */
KB_DIR_IF_NAME_ADD(IMG);
#undef KB_DIR_IF_NAME_ADD



#endif	/* _OPENKB_LIBKB_DIR */
