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

#include "malloc.h"

#define MAX_IMG_FILES	36
#define HEADER_SIZE_IMG (MAX_IMG_FILES * 4 + 2)

struct imgGroup {

	struct imgHeader {

		word num_files;

		struct imgFile {

			word offset;
			word mask_offset;

			word w;
			word h;

		} files[MAX_IMG_FILES];

	} head;

	byte bpp;

	KB_File *top;//top file
	int i;		//iterator
};

struct imgGroup * imgGroup_load(KB_File *f) {

	char buf[HEADER_SIZE_IMG];

	int i, j;

	struct imgGroup *grp;
	struct imgHeader *head;

	int n;

	n = KB_fread(buf, sizeof(char), HEADER_SIZE_IMG, f);

	if (n < 2 + 12) { fprintf(stderr, "Can't read header! (%d bytes)\n", 2 + 12); return NULL; }

	grp = malloc(sizeof(struct imgGroup));

	if (grp == NULL) return NULL;

	head = &grp->head;

	/* Parse data */
	char *p = &buf[0];	

	/* Number of files */
	head->num_files = READ_WORD(p);

	printf("[imggrp] NUM FILES: %d\n",  head->num_files);

	if (head->num_files > MAX_IMG_FILES) {
		free(grp);
		return NULL;
	}

	/* Files entries */
	for (i = 0; i < head->num_files; i++ ) {
		head->files[i].offset = READ_WORD(p);
		head->files[i].mask_offset = READ_WORD(p);
	}

	/* Save FILE handler, reset iterator */
	grp->top = f;
	grp->i = 0;

	/* Default BPP -- unknown */
	grp->bpp = 0;

	/* ~~~~~~~~~~~~~~~ */
	return grp;
}

void KB_seekdirIMG(KB_DIR *dirp, long loc) {

}

long KB_telldirIMG(KB_DIR *dirp) {

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

	/* Read w & h */
	/* Seek to offset, read 4 bytes, unpack width and height */
	char buf[4];
	KB_fseek(grp->top, grp->head.files[grp->i].offset, 0);
	KB_fread(buf, 1, 4, grp->top);
	grp->head.files[grp->i].w = KB_UNPACK_WORD(buf[0],buf[1]);
	grp->head.files[grp->i].h = KB_UNPACK_WORD(buf[2],buf[3]);

	/* Extra data */
	entry->d_info.img.w = grp->head.files[grp->i].w;
	entry->d_info.img.h = grp->head.files[grp->i].h;
	entry->d_info.img.bpp = grp->bpp;

	grp->i++;

	return entry;
}

byte filename_to_bpp(const char *filename) {

	int l = strlen(filename);
	
	printf("Convert: %s to bpp: %d\n", filename, 2);
	
	return 2;
}

/* Open an IMG directory "filename" inside a directory "dirs" */
KB_DIR * KB_opendirIMG_in(const char *filename, KB_DIR *dirs)
{
	KB_DIR * dirp;

	char buf[HEADER_SIZE_IMG];

	int i, j;

	struct imgGroup *grp;
	struct imgHeader *head;

	int n;

	KB_File *f = KB_fopen_in( filename, "rb", dirs );

	if (!f) { fprintf(stderr, "Can't open %s\n", filename); return NULL; }

	grp = imgGroup_load(f);

	if (grp == NULL) {
		KB_fclose(f);
		return NULL;
	}

	dirp = malloc(sizeof(KB_Entry));

	if (dirp == NULL) {
		free(grp);
		KB_fclose(f);
		return NULL;
	}

	/* Some public view */
	dirp->type = KBDTYPE_GRPIMG;
	dirp->d = (void*)grp;

	/* Keep table length in "len" */
	dirp->len = grp->head.num_files;

	/* BPP!!! */
	grp->bpp = filename_to_bpp(filename);
	printf("$$$ Setting Bpp: %d<%s>\n",grp->bpp, filename);

	/* ~~~~~~~~~~~~~~~ */
	return dirp;
}

KB_DIR * KB_opendirIMG(const char *filename) {
	return KB_opendirIMG_in(filename, NULL);
}

KB_File * KB_fopenIMG_in( const char * filename, const char * mode, KB_DIR *dirp )
{
	struct imgGroup *grp = (struct imgGroup *)dirp->d;
	int i = atoi(filename);

	char buf[4];
	dword w, h;

	KB_File *f;

	/* Public view: */
	f = malloc(sizeof(KB_File));
	f->type = KBFTYPE_INIMG;
	f->pos = 0;
	f->d = (void*)grp;

	/* Seek to offset, read 4 bytes, seek to offset again */
	KB_fseek(grp->top, grp->head.files[i].offset, 0);
	KB_fread(buf, 1, 4, grp->top);
	KB_fseek(grp->top, grp->head.files[i].offset, 0);

	grp->head.files[i].w = w = KB_UNPACK_WORD(buf[0],buf[1]);
	grp->head.files[i].h = h = KB_UNPACK_WORD(buf[2],buf[3]);

	f->len = (w * h) / (8 / grp->bpp) + 4;

//dbg
printf("[imgdir]Craving for image <%d> [%s]\n", i, filename);
printf("[imgdir] offset: %08x mask: %08x w: %d, h: %d	| len: %ld\n", 
	grp->head.files[i].offset,
	grp->head.files[i].mask_offset,
	grp->head.files[i].w,
	grp->head.files[i].h, f->len);

	return f;
}

int KB_freadIMG ( void * ptr, int size, int count, KB_File * stream )
{
	struct imgGroup *grp = (struct imgGroup *)stream->d;
	KB_File *real = (KB_File*)grp->top;

	int rcount = KB_fread(ptr, size, count, real);

	stream->pos += rcount;

	return rcount;
}

int KB_fseekIMG(KB_File * stream, long int offset, int origin)
{
	printf("If you see this you are screwed!\n");
	return 0;
}

#if HAVE_LIBSDL
void SDL_add_DOS_palette(SDL_Surface *surf, int bpp) {

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

void SDL_blitRAWIMG(SDL_Surface *surf, SDL_Rect *destrect, char *buf, int bpp, word offset, word mask_pos) {

	switch (bpp) {
		case 1:
			SDL_Blit1BPP(&buf[offset], surf, destrect);
			break;
		case 2:
			SDL_Blit2BPP(&buf[offset], surf, destrect);
			break;
		case 4:
			SDL_Blit4BPP(&buf[offset], surf, destrect);
			break;
		case 8:
		default:
			SDL_Blit8BPP(&buf[offset], surf, destrect);
			break;
	}

	if (mask_pos) {
		if (bpp != 8) 
			SDL_BlitMASK(&buf[mask_pos], surf, destrect);
		else
			SDL_ReplaceIndex(surf, &destrect, buf[mask_pos], 0xFF);
		SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);
	}

}

SDL_Surface *SDL_loadRAWCH(char *buf, int len) 
{
	SDL_Surface *surf;
	SDL_Rect dest;

	word width = 8;
	word height = len / width / 2;

	surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width * 16, height, 8, 0xFF, 0xFF, 0xFF, 0xFF);

	dest.x = 0;
	dest.y = 0;
	dest.w = 8;
	dest.h = 8;

	int off = 0;
	int i;
	for (i = 0; i < 128; i++) {

		SDL_Blit1BPP(&buf[off], surf, &dest);
		off += 8;

		dest.x += 8;
		if (dest.x >= surf->w) {
			dest.x = 0;
			dest.y += 8;
		}	
	}

	SDL_add_DOS_palette(surf, 1);

	return surf;
}

SDL_Surface *SDL_loadRAWIMG(char *buf, int len, int bpp) 
{
	SDL_Surface *surf;
	SDL_Rect dest;

	word width = UNPACK_16_LE(buf[0], buf[1]);
	word height = UNPACK_16_LE(buf[2], buf[3]);

	word mask_pos = 4 + (width * height) / (8 / bpp);

	surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 8, 0xFF, 0xFF, 0xFF, 0xFF);

	dest.x = 0;
	dest.y = 0;
	dest.w = width;
	dest.h = height;

	if (mask_pos >= len) mask_pos = 0;

	SDL_add_DOS_palette(surf, bpp);
	SDL_blitRAWIMG(surf, &dest, &buf[0], bpp, 4, mask_pos);

	return surf;
}

/* TODO: accept only dir :/ */
SDL_Surface *SDL_loadROWIMG(KB_DIR *dirp, word first, word frames, byte bpp) 
{
	SDL_Surface *surf;
	struct imgGroup *grp = (struct imgGroup *)dirp->d;

	int i;
	word width = 0, height = 0;
	word last = first + frames;

	/* Enforce frames, bpp */
	if (first > grp->head.num_files - 1)
		first = grp->head.num_files - 1; 

	if (last > grp->head.num_files) 
		last = grp->head.num_files; 

	if (!grp->bpp) grp->bpp = bpp;

	if (!bpp) return NULL;

	/* Read all frames */
	for (i = first; i < last; i++) {
		char buf[4];
		/* Seek to offset, read 4 bytes, seek to offset again */
		KB_fseek(grp->top, grp->head.files[i].offset, 0);
		KB_fread(buf, 1, 4, grp->top);
		//KB_fseek(grp->top, grp->head.files[i].offset, 0);
	
		grp->head.files[i].w =  KB_UNPACK_WORD(buf[0],buf[1]);
		grp->head.files[i].h =  KB_UNPACK_WORD(buf[2],buf[3]);

		/* Max width / Max height */
		width += grp->head.files[i].w;
		if (height < grp->head.files[i].w) 
			height = grp->head.files[i].h;
	}

	/* Create new SDL surface and tile-blit it */
	surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 8, 0xFF, 0xFF, 0xFF, 0xFF);

	word x = 0, y = 0;
	for (i = first; i < last; i++) {
		int n;
		char buf[0xFA00];

		SDL_Rect dest;

		word base_offset = grp->head.files[i].offset;
		word mask_pos = grp->head.files[i].mask_offset;

		dest.x = x;
		dest.y = y;
		dest.w = grp->head.files[i].w;
		dest.h = grp->head.files[i].h;

		KB_fseek(grp->top, base_offset, 0);
		n = KB_fread(buf, sizeof(char), 0xFA00, grp->top);

		if (mask_pos) mask_pos -= base_offset;

		SDL_blitRAWIMG(surf, &dest, &buf[0], bpp, 4, mask_pos);
		SDL_add_DOS_palette(surf, bpp);		

		x += dest.w;
		if (x >= 320) {
			x = 0;
			y += height;
		}
	}
	return surf;
}
#endif
