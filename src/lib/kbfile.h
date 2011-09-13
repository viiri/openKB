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

#define KBFTYPE_FILE	0x00
#define KBFTYPE_INCC	0x10
#define KBFTYPE_INIMG	0x11

#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

//#include "kbdir.h"

typedef struct KB_File {

	int type;
	void *f;
	long int pos;
	long int len;
	
	void *d;
	int ref_count;

} KB_File;

/*
 * The API.
 * Top level functions. Actually call ones with suffixes, based on file->type.
 */
extern KB_File * KB_fopen ( const char * filename, const char * mode );
extern int KB_fseek ( KB_File * stream, long int offset, int origin );
extern long int KB_ftell ( KB_File * stream );
extern int KB_fread ( void * ptr, int size, int count, KB_File * stream );
extern int KB_fclose ( KB_File * stream );

/*
 * "KB_file" wrapper for real files.
 * a) Provides STDIO-like interface
 * b) Emulates STDIO interface on non-POSIX systems
 * c) Used internally by other file wrappers to read the actual, non-virtual files
 */
extern KB_File * KB_fopenF ( const char * filename, const char * mode );
extern int KB_fseekF ( KB_File * stream, long int offset, int origin );
extern long int KB_ftellF ( KB_File * stream);
extern int KB_freadF ( void * ptr, int size, int count, KB_File * stream );
extern int KB_fcloseF ( KB_File * stream );

#endif	/* _OPENKB_LIBKB_FILE */
