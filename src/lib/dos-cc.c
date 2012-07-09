/*
 *  dos-cc.c -- .CC file reader
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
#include "kbsys.h"
#include "kbstd.h"
#include "kbfile.h"

#define MAX_CC_FILES	133
#define HEADER_SIZE_CC 1122

word KB_ccHash(const char *filename) {
	word key = 0;
	byte next;	

	while ((next = *filename++)) 
	{
		next &= 0x7F;
		if (next >= 0x60) 
			next -= 0x20;

		/* Swap bytes */
		key = ((key >> 8) & 0x00FF) | ((key << 8) & 0xFF00);

		/* Rotate left by 1 bit */
		key = (key << 1) | ((key >> 15) & 0x0001);

		key += next;
	}

	return key;
}

struct ccGroup {

	struct ccHeader {	/* 1122 bytes header */

		word num_files;

		struct {	/* 8 bytes per file entry */

			word key;
			sword offset;
			sword size;

		} files[MAX_CC_FILES];

		char unknown[64];/* Could be more files, could be something else */

	} head;

	struct {	/* Meta-data contained else-where in file */

		struct {

			dword size;

		} files[MAX_CC_FILES];

	} cache;

	struct { 	/* Our own filename list, not part of .CC format */

		struct {

			char name[16];

		} files[MAX_CC_FILES];

	} list;

	KB_File *top;	//driver
	int i;  	//iterator
};

struct ccGroup* ccGroup_load(KB_File *f) {

	char buf[HEADER_SIZE_CC];

	int i, j;

	struct ccGroup *grp;
	struct ccHeader *head;

	int n;
	char *p;

	n = KB_fread(buf, sizeof(char), HEADER_SIZE_CC, f);

	if (n != HEADER_SIZE_CC) { perror("fread"); return NULL; }

	grp = malloc(sizeof(struct ccGroup));

	if (grp == NULL) {
		return NULL;
	}

	head = &grp->head;

	/* Parse data */
	p = &buf[0];

	/* Number of files */
	head->num_files = READ_WORD(p);

	if (head->num_files > MAX_CC_FILES) {
		free(grp);
		return NULL;
	}

	/* Files entries */
	for (i = 0; i < head->num_files; i++ ) {
		head->files[i].key = READ_WORD(p);
		head->files[i].offset = READ_SWORD(p);
		head->files[i].size = READ_SWORD(p);
		grp->cache.files[i].size = 0; // unknown full size
		grp->list.files[i].name[0] = '\0'; // no name yet
	}

	/* Save FILE handler, reset iterator */
	grp->top = f;
	grp->i = 0;
	f->ref_count++;

	return grp;
}

void ccGroup_unload(struct ccGroup* grp) {
	/* Close FILE handler */
	grp->top->ref_count--;
	KB_fclose(grp->top);
	free(grp);
}

void ccGroup_read(struct ccGroup* grp, int first, int frames) {
	int i;
	for (i = first; i < first + frames; i++) {
		/* Read out "uncompressed size" if we haven't already */
		if (!grp->cache.files[i].size) {
			char buf[4];

			/* Seek to offset, read 4 bytes */
			KB_fseek(grp->top, grp->head.files[i].offset, SEEK_SET);
			KB_fread(buf, 1, 4, grp->top);

			grp->cache.files[i].size = KB_UNPACK_DWORD(buf[0],buf[1],buf[2],buf[3]);
		}
	}
}

int ccGroup_append_list(struct ccGroup *grp, const char *filename) {

	char listfile[1024];
	int i;

	FILE *f;

	/* Clean list */
	for (i = 0; i < MAX_CC_FILES; i++) {
		grp->list.files[i].name[0] = '\0';
	}

	/* Prepare filename */
	KB_strcpy(listfile, filename);
	KB_strcat(listfile, (filename[strlen(filename)-1] == 'C' ? "L" : "l"));

	/* Try reading */
	f = fopen(listfile, "r");
	if (f != NULL) {
		char buf[1024] = { 0 };
		while (fgets(buf, 1024, f) != NULL) {
			int j;
			word key;
			for (j = 0; j < 16; j++) {
				if (buf[j] == '\n') buf[j] = '\0';
				if (!isascii(buf[j])) buf[j] = '\0';
			}
			key = KB_ccHash(buf);
			for (j = 0; j < grp->head.num_files; j++) {
				if (grp->head.files[j].key == key) {
					KB_strcpy(grp->list.files[j].name, buf);
					break;
				}
			}
		}
		fclose(f);
	} else {
		KB_stdlog("%s not found\n", listfile);
		return 1;
	}
	return 0;
}

#define MBUFFER_SIZE 1024
#define MBUFFER_EDGE (MBUFFER_SIZE - 3) 

/*
 * Returns Number of bytes read, 0 on error 
 */
int KB_funLZW(char *result, unsigned int max, KB_File *f) {

	int i, n, j;

	unsigned int res_pos = 0;

	/*
	 * The data is kept in BIT-positioned "blocks".
	 * We use several variables to iterate it with some
	 * level of sanity:
	 */
	int pos = 0;	/* Position in bits */

	int byte_pos = 0; /* Position in bytes */
	int bit_pos = 0; /* Extra shift in bits */

	/*
	 * Each "block" can take from 9 to 12 bits of data.
	 * Last 8 bits are "the value" 
	 * First N bits are "the key"
	 */
	int step = 9;	/* Bits per step */

	/* Those masks help us get the relevant bits */ 
	word keyMask[4] = {
		0x01FF, 	// 0001 1111 
		0x03FF,		// 0011 1111
		0x07FF,		// 0111 1111
		0x0FFF,		// 1111 1111
	};

	/*
	 * If the "the key" is unset (00), the value is read as-is.
	 * Otherwise, the whole "block" (N+8 bytes) is treated
	 * as a dictionary key, and the dictionary is queried.
	 * The exceptions are the 0x0100 value, which means
	 * the dictionary must be cleared, and the 0x0101 value,
	 * which marks END OF FILE.
	 */

	/*
	 * The dictionary.
	 * Each entry consists of an index to a previous entry
	 * and a value, forming a tree.
	 */
	word dict_key[768 * 16] = { 0 };
	char dict_val[768 * 16];
	word dict_index = 0x0102; /* Start populating dictionary from 0x0102 */
	word dict_range = 0x0200; /* Allow that much entries before increasing step */

	/* Since data stored this way is backwards, we need a small queue */
	char queue[0xFF];
	int queued = 0;

	/* Read buffer */ 
	char mbuffer[MBUFFER_SIZE];

	/* Data */
	word next_index = 0;	/* block of data we currently examine */

	char last_char  = 0;	/* value from previous iteration */
	word last_index = 0;	/* block from previous iteration */

	sword big_index;	/* temp. variable to safely load and shift 3 bytes */
	word keep_index;	/* temp. variable to keep "next_index" before making it "last_index" */
	int reset_hack=0;	/* HACK -- delay dictionary reset to next iteration */ 

	/* Read first chunk of data */
	n = KB_fread(mbuffer, sizeof(char), MBUFFER_SIZE, f);

	/* Decompress */
	while (1)
	{
		/* We need a dictionary reset */
		if (reset_hack) 
		{
			step = 9;
			dict_range = 0x0200;
			dict_index = 0x0102;
		}

		/* Since "pos" is in bits, we get position in bytes + small offset in bits */
		byte_pos = pos / 8;
		bit_pos = pos % 8;

		pos += step;	/* And advance to the next chunk */

		/* Edge of buffer, read more data from file */
		if (byte_pos >= MBUFFER_EDGE)
		{
				int bytes_extra = MBUFFER_SIZE - byte_pos;//~= 3
				int bytes_left = MBUFFER_SIZE - bytes_extra;//~= 1021

				/* Copy leftovers */
				for (j = 0; j < bytes_extra; j++) mbuffer[j] = mbuffer[bytes_left + j];

				/* Read in the rest */				
				n = KB_fread(&mbuffer[bytes_extra], sizeof(char), bytes_left, f);

				/* Reset cursor */
				pos = bit_pos + step;	/* Add all unused bits */
				byte_pos = 0;
				/* On dictionary reset, use byte offset as bit offset*/
				if (reset_hack) bit_pos = bytes_extra;
		}
#ifdef DEBUG
KB_debuglog(0, "%04d\t0x%04X:%01X\t", pos, byte_pos, bit_pos);
#endif
		/* Read index from position "byte_pos", bit offset "bit_pos" */
		big_index = 
			((mbuffer[byte_pos+2] & 0x00FF) << 16) | 
			((mbuffer[byte_pos+1] & 0x00FF) << 8) | 
			(mbuffer[byte_pos] & 0x00FF);

		big_index >>= bit_pos;
		big_index &= 0x0000FFFF;

		next_index = big_index;
		next_index &= keyMask[ (step - 9) ];

		/* Apply the value as-is, continuing with dictionary reset, C) */
		if (reset_hack) 
		{
			/* Save index */
			last_index = next_index;
			/* Output char value */
			last_char = (next_index & 0x00FF);
			result[res_pos++] = last_char;
			/* We're done with the hack */
			reset_hack = 0;
			continue;
		}

		if (next_index == 0x0101)	/* End Of File */
		{
			/* DONE */
			break;
		}

		if (next_index == 0x0100) 	/* Reset dictionary */
		{
			/* Postpone it into next iteration */
			reset_hack = 1;
			/* Note: this hack avoids code duplication, but	makes the algorithm
			 * harder to follow. Basically, what happens, when the "reset" 
			 * command is being hit, is that 
			 * A) the dictionary is reset
			 * B) one more value is being read (this is the code duplication bit)
			 * C) the value is applied to output as-is
			 * D) we continue as normal
			 */
			continue;
		}

		/* Remember *real* "next_index" */
		keep_index = next_index;

		/* No dictionary entry to query, step back */
		if (next_index >= dict_index)
		{
			next_index = last_index;
			/* Queue 1 char */
			queue[queued++] = last_char;
		}

		/* Quering dictionary? */
		while (next_index > 0x00ff)
		{
			/* Queue 1 char */
			queue[queued++] = dict_val[next_index];
			/* Next query: */
			next_index = dict_key[next_index];
		}

		/* Queue 1 char */
		last_char = (next_index & 0x00FF);
		queue[queued++] = last_char;

		/* Ensure buffer overflow wouldn't happen */
		if (res_pos + queued > max) break;

		/* Unqueue */
		while (queued) 
		{
			result[res_pos++] = queue[--queued];
		}

		/* Save value to the dictionary */
		dict_key[dict_index] = last_index; /* "goto prev entry" */
		dict_val[dict_index] = last_char;  /* the value */
		dict_index++;

		/* Save *real* "next_index" */
		last_index = keep_index;

		/* Edge of dictionary, increase the bit-step, making range twice as large. */
		if (dict_index >= dict_range && step < 12) 
		{
			step += 1;
			dict_range *= 2;
		}
	}

	return res_pos;
}

/*
 * KB_DirDriver interface
 */

/* Open CC-directory "filename" in abstract directory "dirs" */
void* KB_loaddirCC(const char *filename, KB_DIR *dirs, int *max)
{
	int i, j;

	struct ccGroup *grp;

	KB_File *f = KB_fopen_in( filename, "rb", dirs );

	if (!f) { printf("Can't open %s\n", filename); return NULL; }

	grp = ccGroup_load(f);

	if (grp == NULL) {
		KB_fclose(f);
		return NULL;
	}

	/* Dir size */
	*max = grp->head.num_files;

	/* Attempt to load .CCL filename list */
	ccGroup_append_list(grp, filename);

	return grp;
}

void KB_seekdirCC(KB_DIR *dirp, long loc) {
	struct ccGroup *grp = (struct ccGroup *)dirp->d;
	grp->i = loc;
}

long KB_telldirCC(KB_DIR *dirp) {
	struct ccGroup *grp = (struct ccGroup *)dirp->d;
	return grp->i;
}

struct KB_Entry * KB_readdirCC(KB_DIR *dirp) 
{
	KB_Entry *entry = &dirp->dit;
	struct ccGroup *grp = (struct ccGroup *)dirp->d;

	if (grp->i >= grp->head.num_files) return NULL;

	entry->d_ino = grp->head.files[grp->i].key;

	/* Name */
	if (grp->list.files[grp->i].name[0] != '\0')
		strcpy(entry->d_name, grp->list.files[grp->i].name);
	else
		sprintf(entry->d_name, "file.%d-%04x", grp->i, grp->head.files[grp->i].key);

	/* Read out "uncompressed size" if we haven't already */
	ccGroup_read(grp, grp->i, 1);

	/* Extra data */
	entry->d_info.cmp.cmpSize = grp->head.files[grp->i].size;
	entry->d_info.cmp.fullSize = grp->cache.files[grp->i].size;

	grp->i++;

	return entry;
}

int KB_closedirCC(KB_DIR *dirp) 
{
	struct ccGroup *grp = (struct ccGroup *)dirp->d;
	ccGroup_unload(grp);
	free(dirp);
	return 0;
}


/*
 * KB_FileDriver interface
 */
KB_File * KB_fopenCC_by(int i, KB_DIR *dirp)
{
	struct ccGroup *grp = (struct ccGroup *)dirp->d;

	KB_File *stream;
	char *data;
	int n;

	if (grp->top == NULL) return NULL;

	stream = malloc(sizeof(KB_File));

	if (stream == NULL) return NULL;

	/* Read out "uncompressed size" if we haven't already */
	ccGroup_read(grp, i, 1);

	/* Allocate that much bytes */
	stream->len = grp->cache.files[i].size;
	data = malloc(sizeof(char) * stream->len);

	if (data == NULL) { free(stream); return NULL; }

	/* Unpack LZw data */
	KB_fseek(grp->top, grp->head.files[i].offset + 4, 0);
	n = KB_funLZW(data, stream->len, grp->top);

	if (n < stream->len) {
		KB_errlog("Expected %d bytes, unlzw'ed %d\n", stream->len, n); 
		free(data); free(stream); return NULL; 
	}

	/* Save pointer */
	stream->d = (void*)data;
	stream->pos = 0;
#if 0
KB_debuglog(0, "[%02x][%02x][%02x][%02x]\n", data[0], data[1], data[2], data[3]);
#endif
	return stream;
}

/* Open file "filename" in CC directory "dirp" */
KB_File * KB_fopenCC_in( const char * filename, const char * mode, KB_DIR *dirp )
{
	if (dirp == NULL || dirp->type != KBDTYPE_GRPCC) {
		KB_errlog("Error! Unable to read CC file, incorrect CC directory %p.\n", filename, dirp);
		return NULL;
	}
	struct ccGroup *grp = (struct ccGroup *)dirp->d;
	word hash = KB_ccHash(filename);
	int i;

	for (i = 0; i < grp->head.num_files; i++) {
		if (grp->head.files[i].key == hash) {
			return KB_fopenCC_by( i, dirp );
		} 
	}

	/* HACK! If file extension ends in "%04x" hex, try to use _that_ as hash.
	 * Allows us to load files in "unknown.xxxx" format, i.e. files with no names. */
	int ext_len;
	const char *ext = strrchr(filename, '.');
	if (ext && (ext_len = strlen(ext)) >= 4) {
		hash = hex2dec(&ext[ext_len-4]);
		for (i = 0; i < grp->head.num_files; i++) {
			if (grp->head.files[i].key == hash) {
				return KB_fopenCC_by( i, dirp );
			}
		}
	}

	return NULL;
}

int KB_fseekCC(KB_File * stream, long int offset, int origin)
{
	int err = 0;
	long int roffset = offset;
	if (origin == SEEK_END) roffset = stream->len - offset;
	if (origin == SEEK_CUR) roffset = stream->pos + offset;
	if (roffset < 0) {
		roffset = 0;
		err = 1;
	}
	if (roffset > stream->len) {
		roffset = stream->len;
		err = 1;
	}
#if 0
	printf("Seeking inside stream: %ld, asked %ld, error %d\n", roffset, offset, err);
#endif
	stream->pos = roffset;
	return err;
}

long int KB_ftellCC(KB_File * stream)
{
	return stream->pos;
}

int KB_freadCC ( void * ptr, int size, int count, KB_File * stream )
{
	char *data = stream->d;
	/* Bytes left */
	int rcount = stream->len - stream->pos;
#if 0
	printf("Guy asked for %d bytes, not giving more then %d to him....\n", count, rcount);
#endif
	/* If he asked more than that */
	if (count > rcount) count = rcount;

	/* --read-- */
	memcpy(ptr, &data[stream->pos], count);

	/* Public view: */
	stream->pos += count;

	return count;
}

int KB_fcloseCC( KB_File * stream )
{
	char *data = stream->d;
	free(data);
	free(stream);
	return 0;
}
