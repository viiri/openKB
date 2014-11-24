/*
 *  kbfile.h -- File I/O API (mimics STDIO fopen/fread/fseek/ftell/fclose)
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
 * Provides an opaque filehandler "KB_file", used similarly to
 * stdio's "FILE" , gzlib's "gzFile" or SDL's "RWops".
 * Internally, several actual data-sources might hide.
 */
/*
 * See also: kbdir.h - for the compatible Directory I/O API.  
 */
#ifndef _OPENKB_LIBKB_FILE
#define _OPENKB_LIBKB_FILE

#include <stdio.h> /* SEEK_SET and friends defines, 'stderr' variable */

#define MAX_KBFTYPE	4

#define KBFTYPE_BUF 	0x00
#define KBFTYPE_FILE	0x01
#define KBFTYPE_INCC	0x02
#define KBFTYPE_INIMG	0x03

#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#include "kbdir.h"

typedef struct KB_File {

	int type;
	void *d;
	long int pos;
	long int len;
	
	int ref_count;
	
	void *prev;

} KB_File;

typedef struct KB_FileDriver {
	
	int type;	

	KB_File*(*fopen_in)(const char * filename, const char * mode, KB_DIR * in);
	int 	(*fseek)(KB_File * stream, long int offset, int origin);
	long int(*ftell)(KB_File * stream);
	int 	(*fread)(void * ptr, int size, int count, KB_File * stream);
	int 	(*fclose)(KB_File * stream);

} KB_FileDriver;

/*
 * The API.
 * Top level functions. Do some housekeeping, and call KB_FileDriver
 * callbacks.
 */
#define KB_fopen(FILENAME, MODE) KB_fopen_in(FILENAME, MODE, NULL)

extern KB_File * KB_fopen_in ( const char * filename, const char * mode, KB_DIR * in );
extern int KB_fseek ( KB_File * stream, long int offset, int origin );
extern long int KB_ftell ( KB_File * stream );
extern int KB_fread ( void * ptr, int size, int count, KB_File * stream );
extern int KB_fclose ( KB_File * stream );

/* Bonus API */
extern char* KB_fgets(char *buf, int size, KB_File *stream);

/* Interfaces */
#define KB_FILE_IF_NAME_ADD(SUFFIX) \
extern KB_File * KB_fopen ## SUFFIX ## _in(const char * filename, const char * mode, KB_DIR * in); \
extern int KB_fseek ## SUFFIX  (KB_File * stream, long int offset, int origin); \
extern long int KB_ftell ## SUFFIX  (KB_File *stream); \
extern int KB_fread ## SUFFIX  (void * ptr, int size, int count, KB_File * stream); \
extern int KB_fclose ## SUFFIX  (KB_File * stream);
/*
 * Real file (suffix "F")
 * a) Provides STDIO-like interface
 * b) Emulates STDIO interface on non-POSIX systems
 * c) Used internally by other file wrappers to read the actual, non-virtual files
 */
KB_FILE_IF_NAME_ADD(F)
/*
 * CC Inline File ("CC") 
 */
KB_FILE_IF_NAME_ADD(CC)
/*
 * IMG Inline File ("IMG") 
 */
KB_FILE_IF_NAME_ADD(IMG)
/*
 * BUF Array Access ("BUF")
 */
KB_FILE_IF_NAME_ADD(BUF)
#undef KB_FILE_IF_NAME_ADD

#ifdef HAVE_LIBSDL
#include <SDL.h>

/* RWops interface to file */
extern SDL_RWops* KBRW_open( KB_File *f );
extern int KBRW_seek( SDL_RWops *ctx, int offset, int whence );
extern int KBRW_read( SDL_RWops *ctx, void *ptr, int size, int maxnum);
extern int KBRW_write( SDL_RWops *ctx, const void *ptr, int size, int num);
extern int KBRW_close( SDL_RWops *ctx);
#endif


#endif	/* _OPENKB_LIBKB_FILE */
