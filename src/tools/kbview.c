/*
 * proof-of-concept .4/.16/.256 file viewer and BMP writer
 *
 * Public Domain
 * 
 * THIS CODE IS DIRT
 */

/* NOTE!
 *
 * original version of this code (lurking in #if 0...#endif blocks) created
 * 32-bit RGB surfaces, and showed them onscreen. See "put_2bpp", "put_4bpp",
 * and "put_8bpp" functions.
 *
 * for conversion purposes, however, it is much more beneficial to keep the
 * palette information around, so a slightly more involved method is used.
 * (see "put_2bpp_8", "put_4bpp_8", "put_8bpp_8" functions)
 *
 * VGA images are: 256 colors, color 0xFF is color key.
 * EGA images are: 16 colors, color 0xFF is color key.
 * CGA images are: 4 colors, color 0xFF is color key.
 *
 * VGA images really *did* use color 0xFF as color key.
 * EGA and CGA images use separate mask channel,
 *  which *we* convert to index 0xFF (and assign proper color value).
 * SOME images do not have mask/color key set at all. We account for this.
 *
 * Converting back from such formats is straight-forward (see "kbimg2dos" tool).
 * In short, for CGA & EGA images, mask layer is extracted by peeking at the
 * 0xFF-colored pixels, and then those pixels are replaced with the original
 * color index. (We can find the original color by comparing color values).
 */

#include <SDL.h>
#include <SDL_image.h>
#include <math.h>

//#include "../../vendor/savepng.h"

typedef Uint16	word  ; /* 16-bit */
typedef Uint8	byte  ; /* 8-bit */

#define RENDER_1BPP 0
#define RENDER_CGA 1
#define RENDER_EGA 2
#define RENDER_VGA 3
char *render_modes[] = {
	"1bpp",
	"CGA",
	"EGA",
	"VGA",
};

int render_mode = 0; /* Important */




Uint8 cga_pallete_ega[16] =
{
	0, // 00 // black // bin:00
	3, // 01 // cyan // bin:01
	5, // 02 // magenta // bin:10
	7, // 03 // white // bin:11
};

Uint8 cga_pallete_mask_idx[16] =
{
	0,
	3,
};

Uint8 cga_pallete_mask_ega[16] =
{
	0, // 00 // black // bin:00
	7, // 03 // white // bin:11
	3, // 01 // cyan // bin:01
	5, // 02 // magenta // bin:10
};

Uint32 cga_pallete_rgb[4];

Uint32 vga_pallete_rgb[256];

Uint32 ega_pallete_rgb[16] = 
{
	0x000000, // 00 // dark black
	0x0000AA, // 01 // dark blue
	0x00AA00, // 02 // dark green
	0x00AAAA, // 03 // cyan	0xAA0000, // 04 // dark red
	0xAA00AA, // 05 // magenta
 	0xAA5500, // 06 // brown
	0xAAAAAA, // 07 // dark white / light gray 	0x555555, // 08 // dark gray / light black	0x5555FF, // 09 // light blue	0x55FF55, // 10 // light green	0x55FFFF, // 11 // light cyan	0xFF5555, // 12 // light red	0xFF55FF, // 13 // light magenta	0xFFFF55, // 14 // light yellow	0xFFFFFF, // 15 // bright white};

void apply_pal(SDL_Surface *dest, Uint32 *pal, int pal_max, int mask_index)
{
	SDL_Color colors[256];
	int i;
	for (i = 0; i < pal_max; i++)
	{
		colors[i].r = (pal[i] & 0xFF0000) >> 16;
		colors[i].g = (pal[i] & 0x00FF00) >> 8;
		colors[i].b = (pal[i] & 0x0000FF) >> 0;
	}

	colors[255].r = colors[mask_index].r;
	colors[255].g = colors[mask_index].g;
	colors[255].b = colors[mask_index].b;

	SDL_SetColors(dest, colors, 0, 256);
}

void apply_cga_pal(SDL_Surface *dest, int mask_index)
{
	apply_pal(dest, cga_pallete_rgb, 4, mask_index);
}
void apply_ega_pal(SDL_Surface *dest, int mask_index)
{
	apply_pal(dest, ega_pallete_rgb, 16, mask_index);
}
void apply_vga_pal(SDL_Surface *dest, int mask_index)
{
	apply_pal(dest, vga_pallete_rgb, 256, mask_index);
}

/* NOTE: sets of ( put_1bpp, put_2bpp, put_4bpp, put_8bpp ) and
 * ( put_2bpp_8, put_4bpp_8, put_8bpp_8 ) functions could be rewritten
 * as one small function each.
 *
 * I'm leaving them verbose for better readability, and potential
 * copy&paste-ability into other projects.
 *
 * If you're interested in such a condensed function, see
 * "SDL_BlitXBPP()" in src/tools/kbres.c of openkb.
 */

void put_1bpp(SDL_Surface *dest, char *src, unsigned long n, unsigned int width, unsigned int height, int tx, int ty)
{
	/* 8 bits contain 8 pixels o_O */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	for (i = 0; i < n; i += 1) {

		char test = src[i];
		char c[8];

		c[0] = ((test & 0x80) >> 7);// bin:10000000
		c[1] = ((test & 0x40) >> 6);// bin:01000000
		c[2] = ((test & 0x20) >> 5);// bin:00100000
		c[3] = ((test & 0x10) >> 4);// bin:00010000
		c[4] = ((test & 0x08) >> 3);// bin:00001000
		c[5] = ((test & 0x04) >> 2);// bin:00000100
		c[6] = ((test & 0x02) >> 1);// bin:00000010
		c[7] = ((test & 0x01) >> 0);// bin:00000001

		Uint8 *p;
		int j;
		for (j = 0; j < 8; j++) {

			p = (Uint8*) dest->pixels + (dy+ty) * dest->pitch + (dx+tx);
			*p = cga_pallete_mask_idx[c[j]];

			dx ++;
			if (dx >= width) {
				dx = 0; dy++;
				if (dy >= height) return;
			}

		}

	}
}

void put_2bpp_8(SDL_Surface *dest, char *src, unsigned long n, char *mask_src, unsigned long mask_n, SDL_Rect *dest_rect, int *mask_index)
{
	/* 8 bits contain 4 pixels */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	for (i = 0; i < n; i += 1) {

		char test = src[i];
		char mask;
		char c[4], m[4] = { 0 };

		c[0] = ((test & 0xC0) >> 6);// bin:11000000
		c[1] = ((test & 0x30) >> 4);// bin:00110000
		c[2] = ((test & 0x0C) >> 2);// bin:00001100
		c[3] = ((test & 0x03) >> 0);// bin:00000011

		if (mask_src) {
			int m_flip, m_pos;

			m_flip = i % 2;
			m_pos = i / 2;
			mask = mask_src[m_pos];

			if (!m_flip) {
				m[0] = ((mask & 0x80) >> 7);// bin:10000000
				m[1] = ((mask & 0x40) >> 6);// bin:01000000
				m[2] = ((mask & 0x20) >> 5);// bin:00100000
				m[3] = ((mask & 0x10) >> 4);// bin:00010000
			} else {
				m[0] = ((mask & 0x08) >> 3);// bin:00001000
				m[1] = ((mask & 0x04) >> 2);// bin:00000100
				m[2] = ((mask & 0x02) >> 1);// bin:00000010
				m[3] = ((mask & 0x01) >> 0);// bin:00000001
			}
		}

		Uint8 *p;
		int j;
		for (j = 0; j < 4; j++) {
			p = (Uint8*) dest->pixels + (dy + dest_rect->y) * dest->pitch + (dx + dest_rect->x);

			if (m[j]) {
				*mask_index = c[j];
				c[j] = 255;
			}

			*p = c[j];

			dx ++;
			if (dx >= dest_rect->w) {
				dx = 0; dy++;
				if (dy >= dest_rect->h) {
				/*
					printf("Still have %d bytes left!\n", n - i - 1);
					for (i += 1; i < n; i++) {
						printf("%02x ", (char) src[i] & 0xff );
					}
					printf("\n"); */
					return;
				}
			}

		}

	}
}

#if 0 /* 32-bit version of the above */
void put_2bpp(SDL_Surface *dest, char *src, unsigned long n, unsigned int width, unsigned int height, int tx, int ty)
{
	/* 8 bits contain 4 pixels */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	for (i = 0; i < n; i += 1) {

		char test = src[i];
		char c[4];

		c[0] = ((test & 0xC0) >> 6);// bin:11000000
		c[1] = ((test & 0x30) >> 4);// bin:00110000
		c[2] = ((test & 0x0C) >> 2);// bin:00001100 
		c[3] = ((test & 0x03) >> 0);// bin:00000011

		Uint32 *p;
		int j;
		for (j = 0; j < 4; j++) {
			p = (Uint32*) dest->pixels + (dy+ty) * dest->w + (dx+tx);
			*p = ega_pallete_rgb[cga_pallete_ega[c[j]]];

			dx ++;
			if (dx >= width) {
				dx = 0; dy++;
				if (dy >= height) return;
			}

		}

	}
}
#endif

void put_4bpp_8(SDL_Surface *dest, char *src, unsigned long n, char *mask_src, unsigned long mask_n, SDL_Rect *dest_rect, int *mask_index)
{
	/* 8 bits contain 2 pixels */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	for (i = 0; i < n; i += 1) {

		char test = src[i], mask;
		char c[2], m[2] = { 0 };
		
		c[0] = ((test & 0xF0) >> 4);// bin:11110000
		c[1] = ((test & 0x0F) >> 0);// bin:00001111

		if (mask_src != NULL) {
			int m_flip, m_pos;

			m_flip = i % 4;
			m_pos = i / 4;
			mask = mask_src[m_pos];

			if (m_flip == 0) {
				m[0] = ((mask & 0x80) >> 7);// bin:10000000
				m[1] = ((mask & 0x40) >> 6);// bin:01000000
			} else if (m_flip == 1) {
				m[0] = ((mask & 0x20) >> 5);// bin:00100000
				m[1] = ((mask & 0x10) >> 4);// bin:00010000
			} else if (m_flip == 2) {
				m[0] = ((mask & 0x08) >> 3);// bin:00001000
				m[1] = ((mask & 0x04) >> 2);// bin:00000100
			} else if (m_flip == 3) {
				m[0] = ((mask & 0x02) >> 1);// bin:00000010
				m[1] = ((mask & 0x01) >> 0);// bin:00000001
			}
		}

		Uint8 *p;
		int j;
		for (j = 0; j < 2; j++) {
			p = (Uint8*) dest->pixels + (dy + dest_rect->y) * dest->w + (dx + dest_rect->x);

			if (m[j]) {
				*mask_index = c[j];
				c[j] = 255;
			}

			*p = c[j];

			dx ++;
			if (dx >= dest_rect->w) {
				dx = 0; dy++;
				if (dy >= dest_rect->h) {
				/*
					printf("Still have %d bytes left!\n", n - i - 1);
					for (i += 1; i < n; i++) {
						printf("%02x ", (char) src[i] & 0xff );
					}
					printf("\n"); */
					return;
				}
			}
		}
	}

}

#if 0
void put_4bpp(SDL_Surface *dest, char *src, unsigned long n, int width, int height, int tx, int ty)
{
	/* 8 bits contain 2 pixels */
	unsigned long i;
	unsigned long dx = 0, dy = 0;
	for (i = 0; i < n; i += 1) {

		char test = src[i];
		char c[2];

		c[0] = ((test & 0xF0) >> 4);// bin:11110000
		c[1] = ((test & 0x0F) >> 0);// bin:00001111

		Uint32 *p;
		int j;
		for (j = 0; j < 2; j++) {
			p = (Uint32*) dest->pixels + (dy+ty) * dest->w + (dx+tx);
			*p = vga_pallete_rgb[c[j]];

			dx ++;
			if (dx >= width) {
				dx = 0; dy++;
				if (dy >= height) return;
			}
		}
	}

}
#endif

void put_8bpp_8(SDL_Surface *dest, char *src, unsigned long n, SDL_Rect *dest_rect)
{
	/* 8 bits contain 1 pixel */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	for (i = 0; i < n; i += 1) {

		char c[1];
		c[0] = src[i];

		Uint8 *p;
		int j;
		for (j = 0; j < 1; j++) {
			p = (Uint8*) dest->pixels + (dy + dest_rect->y) * dest->pitch + (dx + dest_rect->x);
			*p = c[j];

			dx ++;
			if (dx >= dest_rect->w) {
				dx = 0; dy++;
				if (dy >= dest_rect->h) {
				/*
					printf("Still have %d bytes left!\n", n - i - 1);
					for (i += 1; i < n; i++) {
						printf("%02x ", (char) src[i] & 0xff );
					}
					printf("\n");
				*/
					return;
				}
			}
		}

	}
}

#if 0
void put_8bpp(SDL_Surface *dest, char *src, unsigned long n, int width, int height, int tx, int ty)
{
	/* 8 bits contain 1 pixel */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	for (i = 0; i < n; i += 1) {

		char c[1];
		c[0] = src[i];

		Uint32 *p;
		int j;
		for (j = 0; j < 1; j++) {
			p = (Uint32*) dest->pixels + (dy+ty) * dest->w + (dx+tx);
			*p = vga_pallete_rgb[c[j]];

			dx ++;
			if (dx >= width) {
				dx = 0; dy++;
				if (dy >= height) return;
			}
		}

	}
}
#endif

SDL_Surface *show_font(char *filename, unsigned long off)
{
	int width = 128 * 8;
	int height = 8;
	int i;
	SDL_Rect dest = { 0 };

	SDL_Surface *surf;

	FILE * f;
	unsigned long n;
	int flen;

	char *buf;

	f = fopen(filename, "rb");

	fseek(f, 0, SEEK_END); // seek to end of file
	flen = ftell(f); // get current file pointer
	fseek(f, 32, SEEK_SET); // seek back to beginning of file

	//printf("Filesize: %d bytes [%04x]\n", flen, flen);

	buf = malloc(sizeof(char) * flen);
	//char buf[flen];
	if (buf == NULL) {
		fprintf(stderr, "Unable to allocate %d bytes\n", sizeof(char) * flen);
		return 0;
	}

	n = fread(buf, sizeof(char), flen, f);
	fclose(f);

	surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0);

	for (i = 0; i < 128; i++) {

		put_1bpp(surf, &buf[off], 8, 8, 8, dest.x, dest.y);
		off += 8;

		dest.x += 8;
		if (dest.x >= surf->w) {
			dest.x = 0;
			dest.y += 8;
		}
	}

	return surf;
}

SDL_Surface *show_tiles(char *filename, int show_mask, int _h, int _v, int padding)
{
	unsigned long flen;

	//char buf[level_bytes];
	FILE * f;
	unsigned long n;
	long height;
	long width;

	f = fopen(filename, "rb");

	fseek(f, 0, SEEK_END); // seek to end of file
	flen = ftell(f); // get current file pointer
	fseek(f, 0, SEEK_SET); // seek back to beginning of file	

	//printf("Filesize: %ld bytes [%04lx]\n", flen, flen);

	char *buf = malloc(sizeof(char) * flen);
	//char buf[flen];
	if (buf == NULL) {
		fprintf(stderr, "Unable to allocate %ld bytes\n", sizeof(char) * flen);
		return 0; 
	}

	n = fread ( buf, sizeof(char), flen, f);
	fclose(f);
	
	if (n != flen) {
		fprintf(stderr, "Untable to read %ld bytes\n", flen);
		return 0;
	}

	/* Now, examine header */
	word num_masks = 0;
	word num_frames = 0;
	word frame_pos[256];
	word frame_len[256];
	word mask_pos[256];
	word mask_len[256];

	num_frames = (buf[0] & 0xFF) | buf[1] << 8;

	printf("### Entries: (%d total)\n", num_frames);

	int i;
	int j = 0;

	int max_width = 0;
	int max_height = 0;
	int x_width = 0;
	int x_height = 0;

	unsigned long pos = 0;
	unsigned long mask = 0;
	int last_pos = 0;

	int k = 2;

	for (i = 0; i < num_frames; i++) {

		pos = ((buf[k + 1] << 8)) | (buf[k] & 0xFF);
		mask = ((buf[k + 3] << 8)) | (buf[k + 2] & 0xFF);
		
		pos &= 0xFFFF;		
		mask &= 0xFFFF;

		x_width = ((buf[pos + 1] << 8)) | (buf[pos+0] & 0xFF);
		x_height = ((buf[pos + 3] << 8)) | (buf[pos+2] & 0xFF);
		if (x_width > max_width) max_width = x_width;
		if (x_height > max_height) max_height = x_height;

		if (pos > flen || pos < last_pos) { 
			//printf("Warning: Illigal position: %ld\n", pos);
			//pos = 0;
		}

		printf("Entry %d, Offset: 0x%04lx, Mask: 0x%04lx, Size: %d x %d px\n", i, pos, mask, x_width, x_height);

		frame_pos[i] = pos;
		mask_pos[i] = mask;
		if (mask) num_masks++;

		k += 4;
		last_pos = pos;
	}

	if (render_mode == RENDER_VGA) {
		mask_len[0] = mask_len[1] = mask_len[2] = mask_len[3] = 0;
		for (i = 0; i < num_frames - 1; i++)
		{
			frame_len[i] = frame_pos[i + 1] - frame_pos[i];
		}
		frame_len[num_frames-1] = flen - frame_pos[num_frames-1];
	} else {
		for (i = 0; i < num_frames; i++)
		{
			frame_len[i] = mask_pos[i] - frame_pos[i];
		}
		for (i = 0; i < num_frames - 1; i++)
		{
			mask_len[i] = frame_pos[i + 1] - mask_pos[i];
		}
		mask_len[num_frames-1] = flen - mask_pos[num_frames-1];
	}

#if 0
	for (i = 0; i < num_frames; i++) {
		printf("Entry %d, Len: (%04x) %d, Mask len: (%04x) %d\n", i,
		frame_len[i], frame_len[i],
		mask_len[i], mask_len[i]);
	}
#endif

	{
		width = (buf[frame_pos[0]] & 0xFF) | buf[frame_pos[0] + 1] << 8;
		height = (buf[frame_pos[0] + 2] & 0xFF) | buf[frame_pos[0] + 3] << 8;
		width *= (num_frames * _h);
		height *= (num_frames * _v);
	}

	if (!num_masks) show_mask = 0;
	if (num_frames < 2) padding = 0;

	int total_width;
	int total_height;

	if (_h) {
		total_width = width + (num_frames * padding);
		total_height = max_height * (show_mask+1);
	} else {
		total_width = max_width * (show_mask+1);
		total_height = height + (num_frames * padding);
	}
		printf("Total width: %d, height: %d, show_mask: %d\n", total_width, total_height, show_mask);
	SDL_Surface *surf = SDL_CreateRGBSurface(SDL_SWSURFACE, total_width, total_height, 8, 0, 0, 0, 0);
	SDL_FillRect(surf, NULL, 0x2200FF);

	int mask_mode = 0;
	int mask_index = 255; /* palette index of (any) color which is masked */
	int x = 0;
	int y = 0;
	
	int offset;
	for (i = 0; i < num_frames; i++) {

		offset = frame_pos[i];

		//printf("Reading frame # %d, Offset %d \n", i, offset);

		if (offset == 0) { mask_mode = 1 - mask_mode; continue; }

		width = (buf[offset] & 0xFF) | (buf[offset + 1] << 8);
		offset += 2;

		height = (buf[offset] & 0xFF) | (buf[offset + 1] << 8);
		offset += 2;
		/* */
		//printf("Expected frame size: %d x %d\n", width, height);
		#if 0
		if (width < 0 || height < 0) { 
			width = x;
			height = y;
			continue;
		}
		#endif

		SDL_Rect dest_rect;
		dest_rect.w = width;
		dest_rect.h = height;
		dest_rect.x = x;
		dest_rect.y = y;

		if (render_mode == RENDER_CGA)
			put_2bpp_8(surf, &buf[offset], frame_len[i], mask_pos[i] == 0 ? NULL : &buf[mask_pos[i]], mask_len[i], &dest_rect, &mask_index);
		if (render_mode == RENDER_EGA)
			put_4bpp_8(surf, &buf[offset], frame_len[i], mask_pos[i] == 0 ? NULL : &buf[mask_pos[i]], mask_len[i], &dest_rect, &mask_index);
		if (render_mode == RENDER_VGA)
			put_8bpp_8(surf, &buf[offset], frame_len[i], &dest_rect);
	
		if (render_mode == RENDER_VGA && mask_pos[i]) {
			if (buf[mask_pos[i] - 1] != buf[offset])
			{
				printf("Warning! Mask offset pixel and top left pixel do not match...\n");
			}
			if ((unsigned char)buf[mask_pos[i] - 1] != 0xff)
			{
				printf("Warning! Color key is not 255... (%02x)\n", buf[mask_pos[i] - 1]);
			}
		}

		dest_rect.x = x * _h + width * _v;
		dest_rect.y = y * _v + height * _h;

		if (mask_pos[i] && show_mask)
			put_1bpp(surf, &buf[mask_pos[i]], n - mask_pos[i], width, height, dest_rect.x, dest_rect.y);

		y += (height + padding) * _v;
		x += (width + padding) * _h;
	}

	if (render_mode == RENDER_CGA) apply_cga_pal(surf, mask_index);
	if (render_mode == RENDER_EGA) apply_ega_pal(surf, mask_index);
	if (render_mode == RENDER_VGA) apply_vga_pal(surf, mask_index);

	return surf;
}

int redraw_frame(SDL_Surface *screen)
{

}

void fill_cgapal() {
	int i, j;
	for (i = 0; i < 4; i++) {
		cga_pallete_rgb[i] = ega_pallete_rgb[cga_pallete_ega[i]];
	}
}

void fill_vgapal(char *filename) {
	int i, j;
	for (i = 0; i < 256; i+=16) {
		for (j = 0; j < 16;j++) {
			vga_pallete_rgb[i+j] = ega_pallete_rgb[j];
		}
	}
	SDL_Surface *pal_file = IMG_Load(filename);
	if (pal_file == NULL || pal_file->format->palette == NULL) return;
	printf("VGA palette: %s\n", filename);
	for (i = 0; i < pal_file->format->palette->ncolors; i++) {
		SDL_Color *col = &(pal_file->format->palette->colors[i]);
		vga_pallete_rgb[i] = 
			((col->r << 16) & 0xFF0000) |
			((col->g << 8) & 0x00FF00) |
			((col->b << 0) & 0x0000FF) ;
	} 

}

int filename_extension_to_bitmap_type(const char *filename) {

	int el = strlen(filename);

	/* .bmp */
	if (el > 4 && 
		(filename[el-1] == 'p' || filename[el-1] == 'P') && 
		(filename[el-2] == 'm' || filename[el-2] == 'M') && 
		(filename[el-3] == 'b' || filename[el-3] == 'B') )
	{
		return 0;
	}
	/* .png */
	if (el > 4 && 
		(filename[el-1] == 'g' || filename[el-1] == 'G') && 
		(filename[el-2] == 'n' || filename[el-2] == 'N') && 
		(filename[el-3] == 'p' || filename[el-3] == 'P') )
	{
		return 1;
	}

	return -1;
}


int filename_extension_to_render_mode(const char *filename) {

	int el = strlen(filename);

	/* .256 */
	if (el > 4 && filename[el-1] == '6' && filename[el-2] == '5' && filename[el-3] == '2')
	{
		return RENDER_VGA;
	}
	/* .16 */
	if (el > 3 && filename[el-1] == '6' && filename[el-2] == '1')
	{
		return RENDER_EGA;
	}
	/* .4 */
	if (el > 2 && filename[el-1] == '4')
	{
		return RENDER_CGA;
	}
	/* .CH */
	if (el > 3 && (filename[el-1] == 'H' || filename[el-1] == 'h') && (filename[el-2] == 'C' || filename[el-2] == 'c'))
	{
		return RENDER_1BPP;
	}

	return -1;
}

void display_surface(SDL_Surface *tiles, const char *caption)
{
	SDL_Surface *screen;

	SDL_Init( SDL_INIT_VIDEO );

	screen = SDL_SetVideoMode( tiles->w, tiles->h, 32, SDL_SWSURFACE );

	SDL_WM_SetCaption(caption, NULL);

	int done = 0;
	int redraw = 1;
	SDL_Event event;

	while (!done) {
		/* Wait till user kills or presses ESC */
		while (SDL_PollEvent(&event)) {
			if ((event.type == SDL_QUIT) ||
				(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
			 		done = 1;
		}

		if (redraw) {

			SDL_FillRect(screen, NULL, 0);

			SDL_BlitSurface( tiles, NULL , screen, NULL  );

	    	SDL_Flip( screen );

			SDL_FreeSurface(tiles);

			SDL_Delay(10);

			redraw = 0;

		}

		SDL_Delay(30);

	}

	SDL_Quit(); 
}

int main( int argc, char* args[] ) 
{
	SDL_Surface *tiles;

	char *input_file = NULL;
	char *output_file = NULL;
	char *palette_file = "palette.png";

	int output_mode = -1; /* -1=none, 0=bmp, 1=png */

	int vertical_align = 1;
	int horizontal_align = 0;
	int frame_padding = 2;
	int show_mask = 1;

	int ai;
	int next_is_file = 1;
	int next_type = 0;
	for (ai = 1; ai < argc; ai++ ) {
//		printf("ARGS: %d, %s\n", ai, args[ai]);
		if (!strcmp(args[ai], "-o")) { next_is_file = 1; next_type = 1; output_mode = 0; continue; }
		if (!strcmp(args[ai], "-p")) { next_is_file = 1; next_type = 2; continue; }
		if (!strcmp(args[ai], "-i")) { next_is_file = 1; next_type = 0; continue; }
		if (!strcmp(args[ai], "-V")) { horizontal_align = 0; vertical_align = 1; continue; }
		if (!strcmp(args[ai], "-H")) { horizontal_align = 1; vertical_align = 0; continue; }
		if (!strcmp(args[ai], "-M")) { show_mask = 0; continue; }
		if (!strcmp(args[ai], "-P")) { frame_padding = 0; continue; }
		if (!strcmp(args[ai], "-m")) { next_is_file = 0; continue; }
		if (!next_is_file) {
			long off = atoi(args[ai]);
//			printf("OFFSET: %ld\n", off);
			render_mode = off;
			next_is_file = 1;
			next_type = (input_file == NULL ? 0 : 1);
		} else {
//			printf("FILENAME: %s\n", args[ai]);
			if (next_type == 0)
			{
				input_file = args[ai];
			}
			if (next_type == 1)
			{
				output_file = args[ai];
			}
			if (next_type == 2)
			{
				palette_file = args[ai];
			}
		}
//		printf("ARG %d -- %s\n", ai, args[ai]);
	} 

	if (input_file == NULL) {
		printf("no filename specified!\n");
		exit(1);
	}

	if (output_mode > -1) {
		if (output_file == NULL) {
			output_file = malloc(sizeof(char) * (strlen(input_file) + 4));
			output_file[0] = 0;
			strcpy(output_file, input_file);
			strcat(output_file, ".bmp"); //.png
			output_mode = 0; //1
		} else {
			output_mode = filename_extension_to_bitmap_type(output_file);
		}
		printf("Output file: %s [%s]\n", output_file, output_mode ? "PNG" : "BMP");
		show_mask = 0;
		horizontal_align = 1;
		vertical_align = 0;
		frame_padding = 0;
	}

	//printf("Input file: %s\n", input_file);

	render_mode = filename_extension_to_render_mode(input_file);

	printf("Using render mode: %s\n", render_modes[render_mode]);

	fill_vgapal(palette_file);
	fill_cgapal();

	if (render_mode == 0)
		tiles = show_font(input_file, 0);
	else
		tiles = show_tiles(input_file, show_mask, horizontal_align, vertical_align, frame_padding);

	if (output_mode == -1)
		display_surface(tiles, input_file);
	else
		if (output_mode == 0) /* BMP */
			SDL_SaveBMP(tiles, output_file);
	//else
		//if (output_mode == 1) /* PNG */
			//SDL_SavePNG(tiles, output_file);

	return 0; 
}
