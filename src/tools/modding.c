/* Public-domain (CC0) code.
 * See http://creativecommons.org/publicdomain/zero/1.0/
 */

/*
 * This file contains various functions to convert things back
 * into native King's Bounty formats.
 *
 * It is not part of openkb lib proper, because that functionality
 * wouldn't be ever required by the game itself.
 *
 * However, for tests and modding, it could prove useful.
 */
#include <malloc.h>
#include <string.h>

#ifdef HAVE_LIBSDL
#include <SDL.h>

typedef Uint32	word24; /* 24-bit, "one and a half" word */
typedef Uint16	word  ; /* 16-bit */
typedef Uint8	byte  ; /* 8-bit */

#else

#include <stdint.h>
typedef uint32_t	word24; /* 24-bit, "one and a half" word */
typedef uint16_t	word  ; /* 16-bit */
typedef uint8_t 	byte  ; /* 8-bit */

#endif

#define WRITE_BYTE(PTR, DATA) *PTR++ = DATA
#define WRITE_WORD(PTR, DATA) \
		PTR[0] = (DATA & 0xFF), \
		PTR[1] = ((DATA >> 8) & 0xFF)

#ifdef HAVE_LIBSDL
static SDL_Rect unSafeRect;

static SDL_Rect* SDL_EnsureRect(SDL_Surface *surf, SDL_Rect *rect) {
	if (rect == NULL) {
		rect =& unSafeRect;
		rect->x = 0;
		rect->y = 0;
		rect->w = surf->w;
		rect->h = surf->h;
	}
	return rect;
}

/* Create a base file for a palette image, consisting of 16 x 16 fat pixel blocks */
SDL_Surface *create_indexed_surface() {
	const int zoom = 16;
	int i, j; /* feel free to change surface resolution: */
	SDL_Surface *pal = SDL_CreateRGBSurface(0, 16*zoom, 16*zoom, 8, 0,0,0,0);
	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			SDL_Rect pixel = { i * zoom, j * zoom, zoom, zoom };
			SDL_FillRect(pal, &pixel, j *zoom + i);
		}
	}
	return pal;
}

/* Returns number of bytes written on success, -1 on error */
int DOS_Write1BPP(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect) {
	int i, x, y, bpp, b;
	char c;

	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	bpp = src->format->BytesPerPixel;
	if (bpp != 1) return -1; /* ensure it's a palette image */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	/* 8 bits contain 8 pixels */
	i = 0; c = 0; b = 0;
	for (y = src_rect->y; y < src_rect->y + src_rect->h; y++) {
		for (x = src_rect->x; x < src_rect->x + src_rect->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

			c |= ((*p << (7-b)) & (0x01 << (7-b)));// bin:00000000 <<

			b++;
			if (b > 7) {
				dst[i] = c;
				i++;
				b = 0;
				c = 0;
			}
		}
	}

	return i;
}

/* Returns number of bytes needed to store given rect in a surface or -1 */ 
int DOS_CalcMask(SDL_Surface *src, SDL_Rect *src_rect) {
	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	return (src_rect->w * src_rect->h) / 8; 
}

/* Returns number of bytes written on success, -1 on error */
int DOS_WriteMask(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect) {
	int i, x, y, bpp, b;
	char c;

	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	bpp = src->format->BytesPerPixel;
	if (bpp != 1) return -1; /* ensure it's a palette image */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	/* 8 bits contain 8 pixels */
	i = 0; c = 0; b = 0;
	for (y = src_rect->y; y < src_rect->y + src_rect->h; y++) {
		for (x = src_rect->x; x < src_rect->x + src_rect->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;
			char test;

			test = (*p == 255) ? 1 : 0;

			c |= ((test << (7-b)) & (0x01 << (7-b)));// bin:00000000 <<

			b++;
			if (b > 7) {
				dst[i] = c;
				i++;
				b = 0;
				c = 0;
			}
		}
	}

	return i;
}


int DOS_CalcCGA(SDL_Surface *src, SDL_Rect *src_rect) {
	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	return (src_rect->w * src_rect->h) / 4;
}

/* Returns number of bytes written on success, -1 on error */
int DOS_WriteCGA(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect) {
	int i, x, y, bpp, b;
	char c;

	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	bpp = src->format->BytesPerPixel;
	if (bpp != 1) return -1; /* ensure it's a palette image */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	/* 8 bits contain 2 pixels */
	i = 0; c = 0; b = 0;
	for (y = src_rect->y; y < src_rect->y + src_rect->h; y++) {
		for (x = src_rect->x; x < src_rect->x + src_rect->w; x++) {
     		/* Here p is the address to the pixel we want to retrieve */
    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

			     if (b == 0)   	c |= ((*p << 6) & 0xC0);// bin:11000000	
			else if (b == 1)	c |= ((*p << 4) & 0x30);// bin:00110000	
			else if (b == 2)	c |= ((*p << 2) & 0x0C);// bin:00001100
			else if (b == 3)	c |= ((*p << 0) & 0x03);// bin:00000011

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

int DOS_CalcEGA(SDL_Surface *src, SDL_Rect *src_rect) {
	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	return (src_rect->w * src_rect->h) / 2;
}

/* Returns number of bytes written on success, -1 on error */
int DOS_WriteEGA(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect) {
	int i, x, y, bpp, b;
	char c;

	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	bpp = src->format->BytesPerPixel;
	if (bpp != 1) return -1; /* ensure it's a palette image */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	/* 8 bits contain 2 pixels */
	i = 0; c = 0; b = 0;
	for (y = src_rect->y; y < src_rect->y + src_rect->h; y++) {
		for (x = src_rect->x; x < src_rect->x + src_rect->w; x++) {

    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

			if (!b)	c |= ((*p << 4) & 0xF0);// bin:11110000	
			else	c |= ((*p << 0) & 0x0F);// bin:00001111	

			b++;
			if (b > 1) {
				dst[i] = c;
				i++;
				b = 0;
				c = 0;
			}

		}
	}
	return i;
}

int DOS_CalcVGA(SDL_Surface *src, SDL_Rect *src_rect) {
	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	return (src_rect->w * src_rect->h); 
}

/* Returns number of bytes written on success, -1 on error */
int DOS_WriteVGA(char *dst, int dst_max, SDL_Surface *src, SDL_Rect *src_rect) {
	int i, x, y, bpp;
	char c;

	if (src == NULL) return -1;	/* ensure valid SDL_Surface is passed */
	bpp = src->format->BytesPerPixel;
	if (bpp != 1) return -1; /* ensure it's a palette image */
	src_rect = SDL_EnsureRect(src, src_rect); /* ensure src_rect exists even if NULL was passed */

	/* 8 bits contain 1 pixel */
	i = 0;
	for (y = src_rect->y; y < src_rect->y + src_rect->h; y++) {
		for (x = src_rect->x; x < src_rect->x + src_rect->w; x++) {

    		Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;

			c = (*p);

			dst[i] = c;
			i++;
		}
	}

	return i;
}

/* Returns number of bytes written or -1 */
int DOS_WritePalette_BUF(char *dst, int dst_max, SDL_Color *pal, int num) {

	if (pal == NULL || num <= 0 || num > 255) {
		SDL_SetError("Passed NULL palette or invalid color count\n");
		return -1;
	}

	if (num * 3 >= dst_max) {
		SDL_SetError("%d colors will never fit into %d bytes\n", num, dst_max);
		return -1;
	}

	int i;
	char *p;

	p = &dst[0];
	for (i = 0; i < num; i++) {
		/* 8-bit RGB to 6-bit VGA: */
		byte vga_r = (pal[i].r * 63) / 255;
		byte vga_g = (pal[i].g * 63) / 255;
		byte vga_b = (pal[i].b * 63) / 255;
		/* Write */
		WRITE_BYTE(p, vga_r);
		WRITE_BYTE(p, vga_g);
		WRITE_BYTE(p, vga_b);
	}
	return i * 3;
}

#endif /* HAVE_LIBSDL */

#define DICT_START  	0x0102
#define DICT_BOUNDRY	0x0200
#define MAX_BITS	12
#define CMD_RESET	0x0100
#define CMD_END 	0x0101

#define MAX_DICT_SIZE (sizeof(dict_t) * (DICT_BOUNDRY * 128 + 1))

typedef struct {   
	word next[256];
} dict_t;

int DOS_LZW(char *dst, int dst_max, char *src, int src_len) {
	/*
	 * The data is kept in BIT-positioned "blocks".
	 * We use several variables to iterate it with some
	 * level of sanity:
	 */
	unsigned long total_bits = 0; /* Position in bits */

	int dst_len = 0; /* Position in bytes / Bytes written */
	int bit_pos = 0; /* Extra shift in bits */
	/*
	 * Each "block" can take from 9 to 12 bits of data.
	 * Last 8 bits are "the value"
	 * First N bits are "the key"
	 */
	int step = 9;	/* Bits per step */

	dict_t *dict;
	word    dict_index = 0x0102; /* Start populating dictionary from 0x0102 */
	word    dict_range = 0x0200; /* Allow that much entries before increasing step */

	word last_value;	/* value from previous iteration */
	byte next_value;	/* value we currently examine */
	int err = 0; /* Loop breaker */

	dict = malloc(MAX_DICT_SIZE);
	memset(dict, 0, MAX_DICT_SIZE);

	inline void write_bits(word x) {
		word24 big_index = 0;
//printf("[%d] Wrote value: %08x\n", dst_len, x);
		/* See if buffer is large enough */
		if (dst_len + 3 > dst_max) {
			err = 1;
			dst_len = 0;
			bit_pos = 0;
			/* ^ set everything to 0 */
			return;
		}

		big_index = x << bit_pos;

/* NOTE: this chunk should probably be rewritten with endianess in mind */
		byte a = big_index >> 16;
		byte b = big_index >> 8;
		byte c = big_index >> 0;
/* END NOTE */

		dst[dst_len+0] |= c;
		dst[dst_len+1] |= b;
		dst[dst_len+2] |= a;

		total_bits += step;

		dst_len = total_bits / 8;
		bit_pos = total_bits % 8;
	}

	write_bits(CMD_RESET);

	last_value = (byte) *(src++);
	src_len--;
	while (src_len-- > 0 && !err) {
		word query = last_value & 0xFFFF; /* cast down from word */
		byte next_value = *(src++);

		/* Query dictionary */
		if (dict[query].next[next_value]) {
			last_value = dict[query].next[next_value];
		} else {
			/* Not found, so we add it (and output as is) */
			write_bits(last_value);
			dict[query].next[next_value] = dict_index++;
			last_value = next_value;
		}

		/* When dictionary is overflown, */
		if (dict_index > dict_range) {

			/* Increase bit step */
			if (step < MAX_BITS) {

				dict_range *= 2;
				step++;

			} else { /* and if we can't */ 

				/* Reset dictionary */
				write_bits(CMD_RESET);

				step = 9;
				dict_range = DICT_BOUNDRY;
				dict_index = DICT_START;

				memset(dict, 0, MAX_DICT_SIZE);

			}
		}
	}

	write_bits(last_value);
	write_bits(CMD_END);

	dst_len += (bit_pos ? 1 : 0); 

	free(dict);
	return dst_len;
}

