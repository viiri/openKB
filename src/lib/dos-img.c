/*
 *  dos-img.c -- .4/.16/.256 file reader
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
#include "kbfile.h"
#include "kbdir.h"

#include "kbres.h"

#include "malloc.h"

#define MAX_IMG_FILES	36
#define HEADER_SIZE_IMG (MAX_IMG_FILES * 4 + 2)

struct imgGroup {

	struct imgHeader {

		word num_files;

		struct imgFile {

			word offset;
			word mask_offset;

		} files[MAX_IMG_FILES];

	} head;
	
	struct {

		byte bpp;	

		struct {

			word w;
			word h;

		} files[MAX_IMG_FILES];
	
	} cache;

	void *top;//top file
	int i;		//iterator
};

struct imgGroup* imgGroup_load(KB_File *f) {

	char buf[HEADER_SIZE_IMG];

	int i, j;

	struct imgGroup *grp;
	struct imgHeader *head;

	int n;

	n = KB_fread(buf, sizeof(char), HEADER_SIZE_IMG, f);

	if (n < 2 + 12) { KB_errlog("[imggrp] Can't read header! (%d bytes)\n", 2 + 12); return NULL; }

	grp = malloc(sizeof(struct imgGroup));

	if (grp == NULL) return NULL;

	head = &grp->head;

	/* Parse data */
	char *p = &buf[0];	

	/* Number of files */
	head->num_files = READ_WORD(p);

	if (head->num_files > MAX_IMG_FILES) {
		KB_errlog("[imggrp] Impossible number of internal files: %d\n", head->num_files);
		free(grp);
		return NULL;
	}

	/* Files entries */
	for (i = 0; i < head->num_files; i++ ) {
		head->files[i].offset = READ_WORD(p);
		head->files[i].mask_offset = READ_WORD(p);
		grp->cache.files[i].w = 0; /* no cache */
	}

	/* Save FILE handler, reset iterator */
	grp->top = f;
	grp->i = 0;
	f->ref_count++;

	/* Default BPP -- unknown */
	grp->cache.bpp = 0;

	/* ~~~~~~~~~~~~~~~ */
	return grp;
}

void imgGroup_unload(struct imgGroup* grp) {
	/* Close FILE handler */
	KB_File *top = grp->top;
	top->ref_count--;
	KB_fclose(grp->top);
	free(grp);
}

void imgGroup_read(struct imgGroup* grp, int first, int frames) {
	int i;
	/* Read some frames */
	for (i = first; i < first + frames; i++) {
		char buf[4];
		/* Seek to offset, read 4 bytes, seek to offset again */
		KB_fseek(grp->top, grp->head.files[i].offset, 0);
		KB_fread(buf, 1, 4, grp->top);

		grp->cache.files[i].w = KB_UNPACK_WORD(buf[0],buf[1]);
		grp->cache.files[i].h = KB_UNPACK_WORD(buf[2],buf[3]);

		/* if (i) grp->cache.files[i].len = grp->head.files[i].offset - grp->head.files[i-1].offset;
		else if (i + 1 < grp->head.num_files) grp->cache.files[i].len = grp->head.files[i + 1].offset - grp->head.files[i].offset;
		else KB_errlog("Can't reliably find image filesize at this point :/\n");
		grp->cache.files[i].len -= 4; */
	}
}

byte imgGroup_detect_bpp(struct imgGroup* grp) {
	KB_File *file = grp->top;

	word mask_pos = grp->head.files[0].mask_offset; 
	word pixel_area = grp->cache.files[0].w * grp->cache.files[0].h;
	word next_offset = 0;

	if (grp->cache.bpp != 0) return grp->cache.bpp;

	if (grp->head.num_files > 1)
		next_offset = grp->head.files[1].offset - 4;
	else {
		KB_fseek(file, 0, SEEK_END);
		next_offset = KB_ftell(file);
	}

	if (mask_pos &&	(next_offset - pixel_area / 8 == mask_pos)) 
		next_offset = mask_pos;

	/* BPP = 8 / (AREA / LEN) */
	grp->cache.bpp = 8 / (pixel_area /	(next_offset - grp->head.files[0].offset - 4));

	return grp->cache.bpp;
}

byte imgGroup_filename_to_bpp(const char *filename) {

	char buf[256];
	char ext[4];

	byte bpp = 0;

	name_split(filename, &buf, &ext);

	if (!strcasecmp(ext, "256")) bpp = 8;
	if (!strcasecmp(ext, "16")) bpp = 4;
	if (!strcasecmp(ext, "4")) bpp = 2;
	if (!strcasecmp(ext, "CH")) bpp = 1;

	return bpp;
}

/*
 * KB_DirDriver interface
 */
/* Open an IMG directory "filename" inside a directory "dirs" */
void* KB_loaddirIMG(const char *filename, KB_DIR *dirs, int *max)
{
	int i, j;

	struct imgGroup *grp;

	int n;

	KB_File *f = KB_fopen_in( filename, "rb", dirs );

	if (!f) { KB_errlog("Can't open %s\n", filename); return NULL; }

	grp = imgGroup_load(f);

	if (grp == NULL) {
		KB_fclose(f);
		return NULL;
	}

	/* Dir size */
	*max = grp->head.num_files;

	/* Attempt to parse filename to detect BPP */
	grp->cache.bpp = imgGroup_filename_to_bpp(filename);

	return grp;
}

void KB_seekdirIMG(KB_DIR *dirp, long loc) {
	struct imgGroup *grp = (struct imgGroup *)dirp->d;
	grp->i = loc;
}

long KB_telldirIMG(KB_DIR *dirp) {
	struct imgGroup *grp = (struct imgGroup *)dirp->d;
	return (long)grp->i;
}

struct KB_Entry * KB_readdirIMG(KB_DIR *dirp)
{
	KB_Entry *entry = &dirp->dit;
	struct imgGroup *grp = (struct imgGroup *)dirp->d;

	if (grp->i >= grp->head.num_files) return NULL;

	/* Serial Number */
	entry->d_ino = grp->i;//grp->head.files[grp->i].key;

	/* Name */
	sprintf(entry->d_name, "img.%d", grp->i, grp->head.files[grp->i].offset);

	/* [Read w & h] */
	imgGroup_read(grp, grp->i, 1);

	/* Extra data */
	entry->d_info.img.w = grp->cache.files[grp->i].w;
	entry->d_info.img.h = grp->cache.files[grp->i].h;
	entry->d_info.img.bpp = grp->cache.bpp;

	grp->i++;

	return entry;
}

int KB_closedirIMG(KB_DIR *dirp)
{
	struct imgGroup *grp = (struct imgGroup *)dirp->d;
	imgGroup_unload(grp);
	free(dirp);
	return 0;
}

/*
 * KB_FileDriver interface
 */
KB_File* KB_fopenIMG_in(const char * filename, const char * mode, KB_DIR *dirp)
{
	struct imgGroup *grp = (struct imgGroup *)dirp->d;
	int i = atoi(filename);

	KB_File *f;
	f = malloc(sizeof(KB_File));
	if (f == NULL) return NULL;

	/* Save pointer to imgGroup struct */
	f->d = (void*)grp;

	/* [Read w & h] */
	imgGroup_read(grp, i, 1);
	KB_fseek(grp->top, grp->head.files[i].offset, 0);

	/* Calculate area and byte length */
	dword area = grp->cache.files[i].w * grp->cache.files[i].h;
	f->len = (area) / (8 / grp->cache.bpp) + 4;

KB_debuglog(0, "[imgdir]Craving for image <%d> [%s]\n", i, filename);
KB_debuglog(0, "[imgdir] offset: %08x mask: %08x w: %d, h: %d	| len: %ld\n", 
	grp->head.files[i].offset,
	grp->head.files[i].mask_offset,
	grp->cache.files[i].w,
	grp->cache.files[i].h, f->len);

	return f;
}

int KB_freadIMG ( void * ptr, int size, int count, KB_File * stream )
{
	struct imgGroup *grp = (struct imgGroup *)stream->d;
	KB_File *real = (KB_File*)grp->top;

	int rcount = stream->len - stream->pos;

	if (count > rcount) count = rcount;

	rcount = KB_fread(ptr, size, count, real);

	stream->pos += rcount;

	return rcount;
}

int KB_fseekIMG(KB_File * stream, long int offset, int origin)
{
	printf("If you see this you are screwed!\n");
	return 0;
}

long int KB_ftellIMG(KB_File * stream)
{
	printf("If you see this you are screwed!\n");
	return 0;
}

int KB_fcloseIMG( KB_File * stream )
{
	free(stream);
	return 0;
}

/*
 * SDL flavor.
 */
#if HAVE_LIBSDL
void DOS_SetColors(SDL_Surface *surf, byte bpp) {

	switch (bpp) {
		case 1:
			put_mono_pal(surf);
			break;
		case 2:
			put_cga_pal(surf);
			break;
		case 4:
			put_ega_pal(surf);
			break;
		case 8:
		default:
			put_vga_pal(surf);
			break;
	}

}

void DOS_BlitRAWIMG(SDL_Surface *surf, SDL_Rect *destrect, const char *buf, byte bpp, word mask_pos) {

	SDL_BlitXBPP(buf, surf, destrect, bpp);
	if (mask_pos) {
		if (bpp != 8)	SDL_BlitMASK(&buf[mask_pos - 4], surf, destrect);
		else			SDL_ReplaceIndex(surf, destrect, buf[mask_pos - 4], 0xFF);
		SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);
	}

}

SDL_Surface *DOS_LoadRAWCH_BUF(char *buf, int len) 
{
	SDL_Surface *surf;
	SDL_Rect dest = { 0, 0, 8, 8 };

	int i, off = 0;

	word width = 8;
	word height = len / width / 2;

	surf = SDL_CreatePALSurface(width * 16, height);
	if (surf == NULL) return NULL;

	for (i = 0; i < 128; i++) {

		SDL_BlitXBPP(&buf[off], surf, &dest, 1);
		off += 8;

		dest.x += 8;
		if (dest.x >= surf->w) {
			dest.x = 0;
			dest.y += 8;
		}
	}

	DOS_SetColors(surf, 1);

	return surf;
}

SDL_Surface *DOS_LoadRAWIMG_BUF(char *buf, int len, byte bpp) 
{
	SDL_Surface *surf;
	SDL_Rect dest;

	word width = UNPACK_16_LE(buf[0], buf[1]);
	word height = UNPACK_16_LE(buf[2], buf[3]);

	word mask_pos = 4 + (width * height) / (8 / bpp);

	surf = SDL_CreatePALSurface(width, height);
	if (surf == NULL) return NULL;

	dest.x = 0;
	dest.y = 0;
	dest.w = width;
	dest.h = height;

	if (mask_pos >= len) mask_pos = 0;

	DOS_SetColors(surf, bpp);
	DOS_BlitRAWIMG(surf, &dest, &buf[4], bpp, mask_pos);

	return surf;
}

SDL_Surface *DOS_LoadIMGROW_DIR(KB_DIR *dirp, word first, word frames) 
{
	SDL_Surface *surf;
	struct imgGroup *grp = (struct imgGroup *)dirp->d;

	int i;
	byte bpp;
	word x = 0, y = 0;
	word width = 0, height = 0;
	word last = first + frames;

	/* Ensure requested frames are in bounds */
	if (first > grp->head.num_files - 1)
		first = grp->head.num_files - 1; 

	if (last > grp->head.num_files) 
		last = grp->head.num_files; 

	/* Enforce metadata to be present */
	imgGroup_read(grp, first, last - first);

	/* Find bpp */
	bpp = imgGroup_detect_bpp(grp);

	if (!bpp) return NULL;

	/* Find maximum size */
	for (i = first; i < last; i++) {
		/* Max width / Max height */
		width += grp->cache.files[i].w;
		if (height < grp->cache.files[i].w) 
			height = grp->cache.files[i].h;
	}

	/* Create new SDL surface */
	surf = SDL_CreatePALSurface(width, height);
	if (surf == NULL) return NULL;

	/* Blit the frames */
	for (i = first; i < last; i++) {
		int n;
		char buf[0xFA00];

		SDL_Rect dest;

		word base_offset = grp->head.files[i].offset;
		word mask_pos = grp->head.files[i].mask_offset;

		dest.x = x;
		dest.y = y;
		dest.w = grp->cache.files[i].w;
		dest.h = grp->cache.files[i].h;

		KB_fseek(grp->top, base_offset, 0);
		n = KB_fread(buf, sizeof(char), 0xFA00, grp->top);

		if (mask_pos) mask_pos -= base_offset;

		DOS_BlitRAWIMG(surf, &dest, &buf[4], bpp, mask_pos );
		DOS_SetColors(surf, bpp);		

		x += dest.w;
	}
	return surf;
}

/*
 * SDL_RWops interace.
 * //TODO: convert everything to RWops
 */
SDL_Surface* DOS_LoadIMGROW_RW(SDL_RWops *file, word first, word frames) 
{
	SDL_Surface *surf;
	struct imgGroup group;
	struct imgGroup *grp = &group;

	int i, n;
	byte bpp = 0;

	{
		char buf[HEADER_SIZE_IMG];
		struct imgHeader *head = &grp->head;

		SDL_RWseek(file, 0, 0);

		n = SDL_RWread(file, buf, sizeof(char), HEADER_SIZE_IMG);
		if (n < HEADER_SIZE_IMG) return NULL;

		/* Parse data */
		char *p = &buf[0];	

		/* Number of files */
		head->num_files = READ_WORD(p);
		if (head->num_files > MAX_IMG_FILES) return NULL;

		/* Files entries */
		for (i = 0; i < head->num_files; i++ ) {
			head->files[i].offset = READ_WORD(p);
			head->files[i].mask_offset = READ_WORD(p);
		}
	}

	word x = 0, y = 0;
	word width = 0, height = 0;
	word last = first + frames;

	/* Enforce frames */
	if (first > grp->head.num_files - 1)
		first = grp->head.num_files - 1;

	if (last > grp->head.num_files) 
		last = grp->head.num_files;

	/* Enforce metadata to be present */
	/* Read some frames */
	for (i = first; i < last; i++) 
	{
		char buf[4];
		/* Seek to offset, read 4 bytes */
		SDL_RWseek(file, grp->head.files[i].offset, 0);
		n = SDL_RWread(file, buf, 1, 4);

		grp->cache.files[i].w = KB_UNPACK_WORD(buf[0],buf[1]);
		grp->cache.files[i].h = KB_UNPACK_WORD(buf[2],buf[3]);

		/* Max width / Max height */
		width += grp->cache.files[i].w;
		if (height < grp->cache.files[i].w) 
			height = grp->cache.files[i].h;
	}

	/* Auto-detect bpp */
	{
		word mask_pos = grp->head.files[first].mask_offset; 
		word pixel_area = grp->cache.files[first].w * grp->cache.files[first].h;
		word next_offset = 0;

		if (first + 1 < grp->head.num_files)
			next_offset = grp->head.files[first+1].offset - 4;
		else {
			SDL_RWseek(file, 0, SEEK_END);
			next_offset = SDL_RWtell(file);
		}

		if (mask_pos &&	(next_offset - pixel_area / 8 == mask_pos)) 
			next_offset = mask_pos;
		/* BPP = 8 / (AREA / LEN) */
		bpp = 8 / (pixel_area /	(next_offset - grp->head.files[first].offset - 4));
	}

	if (bpp != 2 && bpp != 4 && bpp != 8) return NULL;

	/* Create new SDL surface */
	surf = SDL_CreatePALSurface(width, height);
	if (surf == NULL) return NULL;

	/* Blit the frames */
	for (i = first; i < last; i++) {
		int n;
		char buf[0xFA00];

		SDL_Rect dest;

		word base_offset = grp->head.files[i].offset;
		word mask_pos = grp->head.files[i].mask_offset;

		dest.x = x;
		dest.y = y;
		dest.w = grp->cache.files[i].w;
		dest.h = grp->cache.files[i].h;

		SDL_RWseek(file, base_offset, 0);
		n = SDL_RWread(file, buf, sizeof(char), 0xFA00);

		if (mask_pos) mask_pos -= base_offset;

		DOS_BlitRAWIMG(surf, &dest, &buf[4], bpp, mask_pos );
		DOS_SetColors(surf, bpp);

		x += dest.w;
	}
	return surf;
}

#endif
