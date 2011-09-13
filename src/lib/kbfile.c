/*
 *  kbfile.c -- File I/O
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
#include "stdio.h"
#include "malloc.h"

#include "string.h"

#include "kbfile.h"
#include "kbdir.h"

KB_DIR * KB_follow_path( const char * filename, int *n, int *e, KB_DIR *top )
{
	int i = 0;
	int ext = 0;
	int rem = 0;
	int l = strlen(filename);
	for (i = 0; i < l; i++) {
		if (filename[i] == '/') {
			rem = i;
		}
		if (filename[i] == '.') {
			ext = i;
		}
		if (filename[i] == '#') {
			break;
		}
	}
	if (i != l) {
		*n = i;

		char buf[1024];
		strcpy(buf, filename);
		buf[i] = '\0';

		KB_DIR *d = KB_opendir_in( buf , top );

		return d;
	}

	*e = ext;
	return NULL;
}

KB_File* KB_fopen_in( const char * filename, const char * mode, KB_DIR *top )
{
	int type = KBFTYPE_FILE;

	int n = 0, e = 0;

	KB_DIR *middle = KB_follow_path(filename, &n, &e, top);


	if (middle != NULL) {
		return KB_fopen_in(&filename[n + 1], mode, middle);
	}

	if (top != NULL) type = top->type;

	switch (type) {
		case KBFTYPE_FILE:	return KB_fopenF( filename, mode );
		case KBFTYPE_INCC:	return KB_fopenCC_in( filename, mode, top );
		case KBFTYPE_INIMG:	return KB_fopenIMG_in( filename, mode, top );		
	}

	return NULL;
}

KB_File* KB_fopen( const char * filename, const char * mode )
{
	return KB_fopen_in(filename, mode, NULL);
}

int KB_fseek( KB_File * stream, long int offset, int origin )
{
	switch (stream->type) {
		case KBFTYPE_FILE:	return KB_fseekF( stream, offset, origin );
		case KBFTYPE_INCC:	return KB_fseekCC( stream, offset, origin );
		case KBFTYPE_INIMG:	return KB_fseekIMG( stream, offset, origin );
	}
	return 1;
}

long int KB_ftell(KB_File * stream)
{
	switch (stream->type) {
		case KBFTYPE_FILE:	return KB_ftellF( stream );
	}
	return 1;
}

int KB_fread ( void * ptr, int size, int count, KB_File * stream )
{
	switch (stream->type) {
		case KBFTYPE_FILE:	return KB_freadF( ptr, size, count, stream );
		case KBFTYPE_INCC:	return KB_freadCC( ptr, size, count, stream );
		case KBFTYPE_INIMG:	return KB_freadIMG( ptr, size, count, stream );
	}
	return 0;
}

int KB_fclose( KB_File * stream )
{
	switch (stream->type) {
		case KBFTYPE_FILE:	return KB_fcloseF( stream );
	}
	return 1;
}

/*
 * "Real file" wrapper
 * All functions have "F" suffix.
 */
KB_File * KB_fopenF( const char * filename, const char * mode )
{
	KB_File * stream;

	FILE *f = fopen(filename, mode);
	if (f == NULL) {
		KB_errlog("Missing real file %s\n", filename);
		return NULL;
	}

	stream = malloc(sizeof(KB_File));

	if (stream == NULL) return NULL;

	stream->type = KBFTYPE_FILE;
	stream->f = f;

	/* Yuck, better use stat */
	fseek(f, 0, SEEK_END);
	stream->len = ftell(f);

	fseek(f, 0, SEEK_SET);
	stream->pos = 0;

	return stream;	
}

int KB_fseekF(KB_File * stream, long int offset, int origin)
{
	if (stream->pos == offset) return 0; 
	if (!fseek( stream->f, offset, origin )) return 1;
	stream->pos = ftell(stream->f);
	return 0;
}

long int KB_ftellF(KB_File * stream)
{
	return stream->pos;
}

int KB_freadF ( void * ptr, int size, int count, KB_File * stream )
{
	return fread(ptr, size, count, stream->f);
}

int KB_fcloseF( KB_File * stream )
{
	fclose(stream->f);
	free(stream);
	return 0;
}
