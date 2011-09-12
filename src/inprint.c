/* INLINE SDL PRINTER */
/* TODO: switch to vendor version */
#include "SDL.h"

static const struct {
  Uint32  	 width;
  Uint32  	 height;
  Uint32  	 pack; 
  Uint8 	 pixel_data[128 * 64 / 8 + 1];
} font_source = {
  128, 64, 8,
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\x08\x44\0\0\x04\x02\0\0\0\0\0 \0\x08\x14\x14\x3c\x2a\x0c\0\x02\x04\0\0\0\0\0 \0\x08\x14\x3e"
  "\x0a\x14\x12\x10\x02\x04\x2a\0\0\0\0\x10\0\x08\0\x14\x1c\x50\x5c\x10\x02\x04\x1c\0\0\x3e\0\x10\0\0\0\x3e"
  "\x28\xa8\x62\0\x02\x04\x2a\0\0\0\0\x08\0\x08\0\x14\x1e\x44\x3c\0\x02\x04\0\0\x08\0\x08\x08\0\0\0\0\0\0"
  "\0\0\x04\x02\0\0\x08\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x3c\x02"
  "\x7e\x3e\x42\x7e\x3c\x7e\x3c\x3c\0\0\x60\x06\0\x3c\x42\x02\x40\x40\x42\x02\x02\x40\x42\x42\x08\x08\x18"
  "\x18\x7e\x42\x5a\x02\x7e\x7c\x7e\x3e\x3e \x3c\x7c\0\0\x06\x60\0\x38\x42\x02\x02\x40\x40\x40\x42\x10\x42"
  "\x40\x08\x08\x18\x18\x7e\0\x3c\x02\x7e\x3e\x40\x3e\x3c\x08\x3c\x3c\0\x04\x60\x06\0\x08\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x3c\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x42\x3c\x3e\x7c\x3e"
  "\x7c\x7c\x7c\x42\x02\x40\x42\x02\x7c\x46\x3c\x5a\x42\x42\x02\x42\x02\x02\x02\x42\x02\x40\x22\x02\x92\x4a"
  "\x42\x4a\x7e\x3e\x02\x42\x3e\x3e\x72\x7e\x02\x40\x1e\x02\x92\x4a\x42\x7a\x42\x42\x02\x42\x02\x02\x42\x42"
  "\x02\x40\x22\x02\x92\x52\x42\x02\x42\x3e\x7c\x3e\x7c\x02\x7c\x42\x02\x3e\x42\x7c\x92\x62\x3c\x3c\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x18\x04\x18\0\0\x3e\x3c"
  "\x3e\x7c\xfe\x42\x42\x92\xc6\x82\x7e\x08\x04\x10\x08\0\x42\x42\x42\x02\x10\x42\x42\x92\x28\x82 \x08\x08"
  "\x10\x14\0\x3e\x42\x3e\x3c\x10\x42\x42\x92\x10\x7c\x18\x08\x08\x10\x22\0\x02\x62\x42\x40\x10\x42\x24\x92"
  "\x28\x10\x04\x08\x10\x10\0\0\x02\x7c\x42\x3e\x10\x3c\x18\x7c\xc6\x10\x7e\x08\x10\x10\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\x18 \x18\0\0\0\0\0\0\0\0\0\0\0\0\0\0 \0\0\0\0\0\x02\0 \0\x1c\0\x02\x04\x08\x02\x04\0\0\0\x02"
  "\0\x02\0 \0\x02\0\x02\0\0\x12\x04\0\0\0\x04\x3c\x1e\x3c\x3c\x1c\x0e\x1c\x1e\x04\x08\x0a\x04\x7c\x1c\x1c"
  "\0\x22\x22\x02\x22\x32\x02\x22\x22\x04\x08\x06\x04\x92\x22\x22\0\x22\x22\x02\x22\x02\x02\x22\x22\x04\x08"
  "\x0a\x04\x92\x22\x22\0\x7c\x1e\x3c\x3c\x1c\x02\x3c\x22\x04\x08\x12\x04\x92\x22\x1c\0\0\0\0\0\0\0 \0\0"
  "\x08\0\0\0\0\0\0\0\0\0\0\0\0\x1c\0\0\x06\0\0\0\0\0\0\0\0\0\x02\0\0\0\0\0\0\x10\x08\x04\0\x3c\0\0\0\0\x02"
  "\0\0\0\0\0\0\x08\x08\x08\0\x42\x1e\x3c\x3c\x1c\x0e\x22\x22\x41\x36\x22\x3e\x08\x08\x08\0\x5a\x22\x22\x02"
  "\x06\x02\x22\x22\x49\x08\x22\x10\x08\x08\x08\x4c\x4a\x22\x22\x02\x18\x02\x22\x14\x49\x08\x22\x08\x04\x08"
  "\x10\x32\x5a\x1e\x3c\x02\x0e\x1c\x1c\x08\x3e\x36\x3c\x3e\x08\x08\x08\0\x42\x02 \0\0\0\0\0\0\0 \0\x08\x08"
  "\x08\0\x3c\x02 \0\0\0\0\0\0\0\x1c\0\x08\x08\x08\0\0"
};

SDL_Surface *inline_font = NULL;
SDL_Surface *selected_font = NULL;
void prepare_inline_font(void)
{
	if (inline_font != NULL) { selected_font = inline_font; return; }

	inline_font = SDL_CreateRGBSurface(SDL_HWSURFACE, font_source.width, font_source.height, 8, 0xFF, 0xFF, 0xFF, 0x00);
	SDL_Surface *dst = inline_font;

	Uint8 *pix_ptr, tmp;

	int i, len, j;

	/* Cache pointer to pixels and array length */
	pix_ptr = (Uint8 *)dst->pixels;		
	len = dst->h * dst->w / font_source.pack;

	/* Lock, Copy, Unlock */
	SDL_LockSurface(dst);
	for (i = 0; i < len; i++) 
	{
		tmp = (Uint8)font_source.pixel_data[i];
		for (j = 0; j < font_source.pack; j++) 
		{
			Uint8 mask = (0x01 << j);
			pix_ptr[i * font_source.pack + j] = ((tmp & mask) ? 1 : 0);
		}
	}
	SDL_UnlockSurface(dst);

	selected_font = inline_font;
}
void kill_inline_font(void) { SDL_FreeSurface(inline_font); inline_font = NULL; }
void infont(SDL_Surface *font) 
{
	selected_font = font;
	if (font == NULL) prepare_inline_font();
}
void incolor(Uint32 fore, Uint32 back) /* Colors must be in 0x00RRGGBB format ! */
{
	SDL_Color pal[2];
	pal[0].r = (Uint8)((back & 0x00FF0000) >> 16); 
	pal[0].g = (Uint8)((back & 0x0000FF00) >> 8);
	pal[0].b = (Uint8)((back & 0x000000FF));
	pal[1].r = (Uint8)((fore & 0x00FF0000) >> 16); 
	pal[1].g = (Uint8)((fore & 0x0000FF00) >> 8);
	pal[1].b = (Uint8)((fore & 0x000000FF));
	SDL_SetColors(selected_font, pal, 0, 2);
}
void inprint(SDL_Surface *dst, const char *str, Uint32 x, Uint32 y)
{
	int i, len;
	len = strlen(str);
	for (i = 0; i < len; i++) {
		int id = (char)str[i];
		int row = id / 16;
		int col = id - (row * 16);

		SDL_Rect s_rect = { col * 8, row * 8, 8, 8}; 
		SDL_Rect d_rect = { x, y, 8, 8 };
		SDL_BlitSurface(selected_font, &s_rect, dst, &d_rect);
		x = x + 8;	
	}
}
#if 0
int main( int argc, char* args[] ) 
{
	SDL_Surface *screen; 

	Uint32 width = 640;
	Uint32 height = 480;
	Uint32 bpp = 32;
	Uint32 flags = 0;

	int i; for (i = 1; i < argc; i++) if (!strcasecmp("--fullscreen", args[i])) flags |= SDL_FULLSCREEN;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		exit(-1);
	}

	if ((screen = SDL_SetVideoMode(width, height, bpp, flags)) == 0) {
		fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
       	exit(-1);
	}

	prepare_inline_font();

	incolor(0xFF0000, 0x333333);

	inprint(screen, "Hello World!", 10, 10);

	SDL_Flip(screen);
	SDL_Delay(10000);

	SDL_Quit(); 
	return 0; 
}
#endif
