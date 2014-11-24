/*
 * proof-of-concept .4/.16/.256 file writer
 *
 * Public Domain
 * 
 * THIS CODE IS DIRT
 */
#include <SDL.h>
#include <SDL_image.h>

typedef Uint16	word  ; /* 16-bit */
typedef Uint8	byte  ; /* 8-bit */

#define WRITE_WORD(PTR, DATA) \
		*PTR++ = ((DATA) & 0xFF), \
		*PTR++ = (((DATA) >> 8) & 0xFF)

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

extern int DOS_Write1BPP(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_CalcMask(SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_WriteMask(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_CalcCGA(SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_WriteCGA(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_CalcEGA(SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_WriteEGA(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_CalcVGA(SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_WriteVGA(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect);
extern int DOS_WritePalette_BUF(char *dst, int dst_max, SDL_Color *pal, int num);

int render_mode = -1; /* Important */

inline void SDL_ClonePalette(SDL_Surface *dst, SDL_Surface *src)
{
	SDL_SetPalette(dst, SDL_LOGPAL | SDL_PHYSPAL, src->format->palette->colors, 0, src->format->palette->ncolors);
}

int find_255(SDL_Surface *src); /* forward-declare */

int count_mask_ref(SDL_Surface *src)
{
	if (render_mode != RENDER_VGA) return 0;
	if (find_255(src) == 0) return 0;
	/* Right in the center */
	return (src->w * src->h) / 2;
}

int count_mask(SDL_Surface *src)
{
	if (render_mode == RENDER_VGA) return 0;
	if (find_255(src) == 0) return 0;
	/* 8 pixels = 1 byte */
	return (src->w * src->h) / 8;
}

#if 0
int count_2bpp(SDL_Surface *src)
{
	/* 4 pixels = 1 byte */
	return (src->w * src->h) / 4;
}

int count_4bpp(SDL_Surface *src)
{
	/* 2 pixels = 1 byte */
	return (src->w * src->h) / 2;
}

int count_8bpp(SDL_Surface *src)
{
	/* 1 pixel = 1 byte */
	return (src->w * src->h);
}
#endif

int count_Xbpp(SDL_Surface *src)
{
#if 0
	if (render_mode == RENDER_VGA)  return count_8bpp(src);
	if (render_mode == RENDER_EGA)  return count_4bpp(src);
	if (render_mode == RENDER_CGA)  return count_2bpp(src);
#else
	if (render_mode == RENDER_VGA)  return DOS_CalcVGA(src, NULL);
	if (render_mode == RENDER_EGA)  return DOS_CalcEGA(src, NULL);
	if (render_mode == RENDER_CGA)  return DOS_CalcCGA(src, NULL);
#endif
	if (render_mode == RENDER_1BPP) return count_mask(src);
	fprintf(stderr, "Error: Undefined render mode, can't count bytes.\n");
	return -1;
}

#if 1
void reput_mask(SDL_Surface *src, char *dst, unsigned long n)
{
	DOS_WriteMask(dst, n, src, NULL);
}
#else
void reput_mask(SDL_Surface *src, char *dst, unsigned long n)
{
	/* 8 bits contain 8 pixels */
	int bpp = src->format->BytesPerPixel;

	int x, y;
	char c = 0;
	int i = 0, b = 0;
	for (y = 0; y < src->h; y++) {
		for (x = 0; x < src->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;
			char test;

			test = (*p == 255) ? 1 : 0;
			
			if (test) { printf("!\n"); }
		
			c |= ((test << (7-b)) & (0x01 << (7-b)));// bin:00000000

			b++;
			if (b > 7) {
				dst[i] = c;
				i++;
				b = 0;
				c = 0;
			}
		}
	}
}

void reput_1bpp(SDL_Surface *src, char *dst, unsigned long n)
{
	/* 8 bits contain 8 pixels */
	int bpp = src->format->BytesPerPixel;

	int b = 0;
	int x, y;
	char c = 0;
	int i = 0;
	for (y = 0; y < src->h; y++) {
		for (x = 0; x < src->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

			c |= ((*p << (7-b)) & (0x01 << (7-b)));// bin:00000000
			b++;
			if (b > 7) {
				dst[i] = c;
				i++;
				b = 0;
				c = 0;
			}

		}
	}
}

void reput_2bpp(SDL_Surface *src, char *dst, unsigned long n)
{
	/* 8 bits contain 2 pixels */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	int bpp = src->format->BytesPerPixel;

	int b = 0;
	int x, y;
	char c = 0;
	i = 0;
	for (y = 0; y < src->h; y++) {
		for (x = 0; x < src->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

			if (b == 0) {
				c |= ((*p << 6) & 0xC0);// bin:11000000	
			} else if (b == 1) {
				c |= ((*p << 4) & 0x30);// bin:00110000	
			} else if (b == 2) {
				c |= ((*p << 2) & 0x0C);// bin:00001100
			} else if (b == 3) {
				c |= ((*p << 0) & 0x03);// bin:00000011
			}
			b++;
			if (b > 3) {
				dst[i] = c;
				i++;
				b = 0;
				c = 0;
			}

		}
	}
}

void reput_4bpp(SDL_Surface *src, char *dst, unsigned long n)
{
	/* 8 bits contain 2 pixels */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	int bpp = src->format->BytesPerPixel;

	int b = 0;
	int x, y;
	char c = 0;
	i = 0;
	for (y = 0; y < src->h; y++) {
		for (x = 0; x < src->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

			if (!b) {
				c |= ((*p << 4) & 0xF0);// bin:11110000	
			} else {
				c |= ((*p << 0) & 0x0F);// bin:00001111	
			}
			b++;
			if (b > 1) {
				dst[i] = c;
				i++;
				b = 0;
				c = 0;
			}

		}
	}
}

void reput_8bpp(SDL_Surface *src, char *dst, unsigned long n)
{
	/* 8 bits contain 1 pixel */
	unsigned long i;
	unsigned long dx = 0, dy = 0;

	int bpp = src->format->BytesPerPixel;

	int x, y;
	char c;
	i = 0;
	for (y = 0; y < src->h; y++) {
		for (x = 0; x < src->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

			c = (char) (*p);

			dst[i] = c;
			i++;
		}
	}
}
#endif

void reput_Xbpp(SDL_Surface *src, char *dst, unsigned long n)
{
#if 0
	if (render_mode == RENDER_VGA)  reput_8bpp(src, dst, n);
	if (render_mode == RENDER_EGA)  reput_4bpp(src, dst, n);
	if (render_mode == RENDER_CGA)  reput_2bpp(src, dst, n);
	if (render_mode == RENDER_1BPP) reput_1bpp(src, dst, n);
#else
	if (render_mode == RENDER_VGA)  DOS_WriteVGA(dst, n, src, NULL);
	if (render_mode == RENDER_EGA)  DOS_WriteEGA(dst, n, src, NULL);
	if (render_mode == RENDER_CGA)  DOS_WriteCGA(dst, n, src, NULL);
	if (render_mode == RENDER_1BPP) DOS_Write1BPP(dst, n, src, NULL);
#endif	
}

int find_255(SDL_Surface *src)
{
	int x, y, bpp;
	bpp = src->format->BytesPerPixel;	
	for (y = 0; y < src->h; y++) {
		for (x = 0; x < src->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;
    		if (*p == 255) return 1;
		}
	}
	return 0;
}

int find_col(SDL_Surface *src)
{
	int i;
	SDL_Color *mcol;
	mcol = &src->format->palette->colors[255];
	for (i = 0; i < src->format->palette->ncolors; i++) {
		SDL_Color *col;
		col = &src->format->palette->colors[i];
		if (col->r == mcol->r && col->g == mcol->g && col->b == mcol->b)
			return i;
	}
	return -1;
}

void reindex(SDL_Surface *src)
{
	int x, y, bpp, new_index;
	bpp = src->format->BytesPerPixel;	
	new_index = find_col(src);
	if (new_index == -1) return;
	for (y = 0; y < src->h; y++) {
		for (x = 0; x < src->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

    		if (*p == 255)
    			*p = new_index;
		}
	}
}
    
void show_usage(char *progname) {
	printf("Usage: %s [-m|-e|-c|-v] OUTPUT INPUT [INPUT2 [INPUT3 [...]]] [/ FRAMES]\n", progname);
	printf(" OUTPUT: desired .4/.16/.256 filename\n");
	printf(" INPUT: source bmp/png/whathaveyou file\n");
	printf(" OPTIONS: set write mode; -m Mono; -e EGA; -c CGA; -v VGA\n");
	printf(" To auto-cut multi-frame bitmap, use the DIVISION SIGN, i.e.:\n");
	printf(" %s peas.256 peas.png / 4\n", progname);
	printf(" To assemble several frames into one file, just pass multiple filenames:\n");
	printf(" %s select.256 select-0.png select-1.png select-2.png\n", progname);
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

int main( int argc, char* args[] ) 
{
	//SDL_Surface *screen;
	//SDL_Surface *tiles;

	char *output = NULL;
	char *input[128];
	int num_inputs = 0;
	int auto_cut = 0;

	int ai;
	int next_is_file = 1;
	for (ai = 1; ai < argc; ai++ ) {
//		printf("ARGS: %d, %s\n", ai, args[ai]);
		if (!strcasecmp(args[ai], "-v")) { render_mode = RENDER_VGA; continue; }
		if (!strcasecmp(args[ai], "-e")) { render_mode = RENDER_EGA; continue; }
		if (!strcasecmp(args[ai], "-c")) { render_mode = RENDER_CGA; continue; }
		if (!strcasecmp(args[ai], "-m")) { render_mode = RENDER_1BPP; continue; }
		if (!strcasecmp(args[ai], "/")) { next_is_file = 0; continue; }
		if (!next_is_file) {
			auto_cut = atoi(args[ai]);
			next_is_file = 1;
		} else {
			if (output == NULL) {
				output = args[ai];
			} else {
				input[num_inputs++] = args[ai];
				if (num_inputs >= 127) {
					fprintf(stderr, "Can't handle more than 128 input files\n");
					exit(1);
				}
			}
		}
//		printf("ARG %d -- %s\n", ai, args[ai]);
	}

	if (!output || num_inputs < 1 || (num_inputs > 1 && auto_cut)) {
		show_usage(args[0]);
		exit(1);
	}

	if (render_mode == -1)	
		render_mode = filename_extension_to_render_mode(output);

	if (render_mode == -1) {
		fprintf(stderr, "Unable to detect format for %s\n", output);
		fprintf(stderr, "Use a switch or a .4/.16/.256 extension.\n", output);
		exit(1);
	}

	if (auto_cut) {
		num_inputs = auto_cut;
	}

	printf("Using render mode: %s, frames: %d\n", render_modes[render_mode], num_inputs);

	//Ok, let's do it.

	//Step 1.
	//Create N SDL_Surfaces, where N equals final number of frames
	SDL_Surface **frames;
	frames = malloc(sizeof(SDL_Surface) * num_inputs);
	if (frames == NULL) {
		fprintf(stderr, "Unable to allocate %ld bytes for input frames.\n", sizeof(SDL_Surface) * num_inputs);
		return -1;
	}

	int i;
	if (auto_cut) {
		SDL_Surface *big = IMG_Load(input[0]);

		if (!big) {
			fprintf(stderr, "Unable to load %s: %s\n", input[0], IMG_GetError());
			free(frames);
			return -1;
		}

		int w = big->w / auto_cut;
		num_inputs = auto_cut;
		for (i = 0; i < num_inputs; i++) 
		{
			frames[i] = SDL_CreateRGBSurface(SDL_SWSURFACE, w, big->h, 8, 0,0,0,0);
			SDL_Rect src = { i * w, 0, w, big->h };
			SDL_Rect dst = {     0, 0, w, big->h };
			SDL_ClonePalette(frames[i], big);
			SDL_BlitSurface(big, &src, frames[i], &dst);
		}

	} else {
		for (i = 0; i < num_inputs; i++) 
		{
			frames[i] = IMG_Load(input[i]);		
		}
	}

	//Step 2.
	//Start writing
#define MAX_IMG_FILES	36
#define HEADER_SIZE_IMG (MAX_IMG_FILES * 4 + 2)

	char header[HEADER_SIZE_IMG] = { 0 };
	char *hp = &header[0];

	FILE *f = fopen(output, "wb");
	if (!f) {
		fprintf(stderr, "Unable to open %s for writing!\n", output);
		exit(2);
	}

	WRITE_WORD(hp, num_inputs);

	int header_size = 4 * num_inputs + 2;
	int next_offset = header_size;
	for (i = 0; i < num_inputs; i++) {
		int img_size = count_Xbpp(frames[i]);
		int mask_size = count_mask(frames[i]);
		int mask_ref = count_mask_ref(frames[i]);

		WRITE_WORD(hp, next_offset);
		if (mask_size) {
			WRITE_WORD(hp, next_offset + img_size + 4);
		}
		else if (mask_ref) {
			WRITE_WORD(hp, next_offset + mask_ref + 4);
		}
		else {
			WRITE_WORD(hp, 0);
		}
		next_offset += img_size + mask_size + 8;
	}

	fseek(f, 0, SEEK_SET);
	fwrite(header, sizeof(char), header_size, f);

	for (i = 0; i < num_inputs; i++) {
		char *buf;
		char *mask_buf;
		char minibuf[4] = { 0 };
		char *mb;

		int size = count_Xbpp(frames[i]);
		int mask_size = count_mask(frames[i]);

		if (render_mode == RENDER_VGA) mask_size = 0;

		buf = malloc(sizeof(char) * size);
		mask_buf = mask_size ? malloc(sizeof(char) * mask_size) : NULL;

		if (mask_buf) {
			reput_mask(frames[i], mask_buf, mask_size);
			reindex(frames[i]);
		}

		reput_Xbpp(frames[i], buf, size);

		mb = &minibuf[0];
		WRITE_WORD(mb, frames[i]->w);
		WRITE_WORD(mb, frames[i]->h);
		fwrite(minibuf, sizeof(char), 4, f);

		fwrite(buf, sizeof(char), size, f);

		if (mask_size)

		fwrite(mask_buf, sizeof(char), mask_size, f);

		mb = &minibuf[0];
		WRITE_WORD(mb, 0);
		WRITE_WORD(mb, 0);
		fwrite(minibuf, sizeof(char), 4, f);

		free(buf);
		free(mask_buf);
	}

	fclose(f);
	
	return 0; 
}
