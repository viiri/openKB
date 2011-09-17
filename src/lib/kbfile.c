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

#define KB_FILE_IF_REP_ADD(SUFFIX) \
		&KB_fopen ## SUFFIX ## _in, \
		&KB_fseek ## SUFFIX , \
		&KB_ftell ## SUFFIX , \
		&KB_fread ## SUFFIX , \
		&KB_fclose ## SUFFIX

KB_FileDriver KB_FS[MAX_KBDTYPE] = {
	{
		KBDTYPE_DIR,
		KB_FILE_IF_REP_ADD(F),
	},
	{
		KBDTYPE_GRPCC,
		KB_FILE_IF_REP_ADD(CC),
	},
	{
		KBDTYPE_GRPIMG,
		KB_FILE_IF_REP_ADD(IMG),
	},
};

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

		d->prev = top;
		if (top) top->ref_count++;

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

	KB_File *f = (KB_FS[type].fopen_in)(filename, mode, top);

	/* Public view: */
	f->type = type;
	f->pos = 0;

	f->ref_count = 0;
	f->prev = (void*)top;
	if (top) top->ref_count++;

	return f;
}

int KB_fseek( KB_File * stream, long int offset, int origin )
{
	return (KB_FS[stream->type].fseek)(stream, offset, origin);
}

long int KB_ftell(KB_File * stream)
{
	return (KB_FS[stream->type].ftell)(stream);
}

int KB_fread ( void * ptr, int size, int count, KB_File * stream )
{
	return (KB_FS[stream->type].fread)(ptr, size, count, stream);
}

int KB_fclose( KB_File * stream )
{
	int ret = -1;
	KB_DIR *prev;

	if (stream->ref_count) {
		KB_errlog("Unable to close file with %d refrences.\n", stream->ref_count);
		return -1;
	}

	prev = (KB_DIR*)stream->prev;	

	ret = (KB_FS[stream->type].fclose)(stream);

	if (!ret && prev) {
		prev->ref_count--;
		if (prev->ref_count == 0)
			ret = KB_closedir(prev);
	}

	return ret;
}

/*
 * "Real file" wrapper
 * All functions have "F" suffix.
 */
KB_File * KB_fopenF_in( const char * filename, const char * mode, KB_DIR *wth )
{
	KB_File * stream;
	FILE *f;

	if (wth != NULL) {
		KB_errlog("Attempting to open real file '%s' in a virtual directory, which is impossible.\n", filename);
		return NULL;
	}

	f = fopen(filename, mode);
	if (f == NULL) {
		KB_errlog("Missing real file %s\n", filename);
		return NULL;
	}

	stream = malloc(sizeof(KB_File));
	if (stream == NULL) {
		fclose(f);
		return NULL;
	}

	/* Save FILE handler */
	stream->d = (void*)f;

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
	if (fseek(stream->d, offset, origin)) return 1;
	stream->pos = ftell(stream->d);
	return 0;
}

long int KB_ftellF(KB_File * stream)
{
	return stream->pos;
}

int KB_freadF ( void * ptr, int size, int count, KB_File * stream )
{
	return fread(ptr, size, count, stream->d);
}

int KB_fcloseF( KB_File * stream )
{
	fclose(stream->d);
	free(stream);
	return 0;
}
