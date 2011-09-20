/*
 * proof-of-concept .4/.16/.256 file viewer
 * pretty useless actually, consider turning it into BMP writer
 *
 * Public Domain
 * 
 * THIS CODE IS DIRT
 */
#include "SDL/SDL.h" 
#include "SDL/SDL_image.h"

#include "math.h"

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

Uint8 cga_pallete_mask_ega[16] =
{
	0, // 00 // black // bin:00
	7, // 03 // white // bin:11
	3, // 01 // cyan // bin:01
	5, // 02 // magenta // bin:10
};

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

		Uint32 *p;
		int j;
		for (j = 0; j < 8; j++) {

			p = (Uint32*) dest->pixels + (dy+ty) * dest->w + (dx+tx);
			*p = ega_pallete_rgb[cga_pallete_mask_ega[c[j]]];

			dx ++;
			if (dx >= width) {
				dx = 0; dy++;
				if (dy >= height) return;
			}

		}

	}
}


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

SDL_Surface *show_font(char *filename, unsigned long off)
{
}

SDL_Surface *show_tiles(char *filename, unsigned long off)
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

	printf("Filesize: %d bytes [%04x]\n", flen, flen);

	char *buf = malloc(sizeof(char) * flen);
	//char buf[flen];
	if (buf == NULL) {
		fprintf(stderr, "Unable to allocate %d bytes\n", sizeof(char) * flen);
		return 0; 
	}

	n = fread ( buf, sizeof(char), flen, f);
	fclose(f);
	
	if (n != flen) {
		printf("Untable to read %d\n", flen);
		return 0;
	}

	/* Now, examine header */
	word num_masks = 0;
	word num_frames = 0;
	word frame_pos[256];
	word mask_pos[256];

	num_frames = (buf[0] & 0xFF) | buf[1] << 8;

	printf("### Entries: (%d total)\n", num_frames);

	int i;
	int j = 0;

	int max_width = 0;
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

		if (pos > flen || pos < last_pos) { 
			//printf("Warning: Illigal position: %ld\n", pos);
			//pos = 0;
		}

		printf("Entry %d, Offset: 0x%08x, Mask: 0x%08x, Size: %d x %d px\n", i, pos, mask, x_width, x_height);

		frame_pos[i] = pos;
		mask_pos[i] = mask;
		if (mask) num_masks++;

		k += 4;
		last_pos = pos;
	}


	{	
		//width = (buf[frame_pos[0]] & 0xFF) | buf[frame_pos[0] + 1] << 8;
		height = (buf[frame_pos[0] + 2] & 0xFF) | buf[frame_pos[0] + 3] << 8;
		height *= num_frames;
	}

	SDL_Surface *surf = SDL_CreateRGBSurface(SDL_SWSURFACE, max_width*2, height + num_frames*2, 32, 0, 0, 0, 0);	
	SDL_FillRect(surf, NULL, 0x2200FF);

	int sowi = 0;
	int mask_mode = 0;
	int sohe = 0;
	
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
		
		if (width < 0 || height < 0) { 
			width = sowi;
			height = sohe;
			continue;
		}

		if (render_mode == RENDER_CGA)
			put_2bpp(surf, &buf[offset], n - offset, width, height, 0, sohe);
		if (render_mode == RENDER_EGA)
			put_4bpp(surf, &buf[offset], n - offset, width, height, 0, sohe);
		if (render_mode == RENDER_VGA)
			put_8bpp(surf, &buf[offset], n - offset, width, height, 0, sohe);

		if (mask_pos[i])
			put_1bpp(surf, &buf[mask_pos[i]], n - mask_pos[i], width, height, width, sohe);

		sohe += height + 2;
		sowi = width;
	}

	return surf;
}

int redraw_frame(SDL_Surface *screen)
{

}
    
void fill_vgapal() {
	int i, j;
	for (i = 0; i < 256; i+=16) {
		for (j = 0; j < 16;j++) {
			vga_pallete_rgb[i+j] = ega_pallete_rgb[j];
		}
	}
}

int main( int argc, char* args[] ) 
{
	SDL_Surface *screen;
	SDL_Surface *tiles;
	
	char filename[1024];
	filename[0] = 0;

	int ai;
	int next_is_file = 1;
	for (ai = 1; ai < argc; ai++ ) {
//		printf("ARGS: %d, %s\n", ai, args[ai]);
		if (!strcasecmp(args[ai], "-m")) { next_is_file = 0; continue; }
		if (!next_is_file) {
			long off = atoi(args[ai]);
//			printf("OFFSET: %ld\n", off);
			render_mode = off;
			next_is_file = 1;
		} else {
//			printf("FILENAME: %s\n", args[ai]);
			strcpy(filename, args[ai]);
		}
//		printf("ARG %d -- %s\n", ai, args[ai]);
	} 

	if (filename[0] == 0) {
		printf("no filename specified!\n");
		exit(1);
	}
	
	int el = strlen(filename);
	
	/* .256 */
	if (el > 4 && filename[el-1] == '6' && filename[el-2] == '5' && filename[el-3] == '2')
	{
		render_mode = RENDER_VGA;
	}
	/* .16 */
	if (el > 3 && filename[el-1] == '6' && filename[el-2] == '1')
	{
		render_mode = RENDER_EGA;
	}
	/* .4 */
	if (el > 2 && filename[el-1] == '4')
	{
		render_mode = RENDER_CGA;
	}
	/* .CH */
	if (el > 3 && filename[el-1] == 'H' && filename[el-2] == 'C')
	{
		render_mode = RENDER_1BPP;
	}

	printf("Using render mode: %s\n", render_modes[render_mode]);

	fill_vgapal();

    SDL_Init( SDL_INIT_VIDEO );

    screen = SDL_SetVideoMode( 800, 1400, 32, SDL_SWSURFACE  ) ;

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

			if (render_mode == 0)
				tiles = show_font(filename, 0);
			else
	 			tiles = show_tiles(filename, 0);

			SDL_FillRect(screen, 0, NULL);

			SDL_BlitSurface( tiles, NULL , screen, NULL  );

	    	SDL_Flip( screen );

			SDL_FreeSurface(tiles);

			SDL_Delay(10);

			redraw = 0;

		}

		SDL_Delay(30);

	}

	SDL_Quit(); 

	return 0; 
}
