/*
 *  env-sdl.c -- resource adapter for SDL
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
#include "SDL.h"

#include "lib/kbconf.h"
#include "lib/kbres.h"

#include "../vendor/vendor.h"
#include "env.h"

/*
 * Default config.
 * Yeah, a local global variable :/
 * NULL by default, to ensure clean segfaults if no default was provided.
 */
KBconfig *conf = NULL;

/*
 * Start/Stop the "environment"
 */
KBenv *KB_startENV(KBconfig *conf) {

	Uint32 width, height, flags;

	KBenv *nsys = malloc(sizeof(KBenv));

	if (!nsys) return NULL; 

    SDL_Init( SDL_INIT_VIDEO );

	width = 320;
	height = 200;
	flags = SDL_SWSURFACE;

	nsys->pan = 0;
	nsys->zoom = 1;

	if (conf->filter) {
		width = 640;
		height = 400;
		nsys->zoom = 2;
	}

	if (conf->fullscreen) {
		flags |= SDL_FULLSCREEN;
		if (height == 400) {
			nsys->pan = 40;
			height = 480;
		}
	}

    nsys->screen = SDL_SetVideoMode( width, height, 32, flags );

    nsys->conf = conf;

	nsys->font = NULL;
	
	RESOURCE_DefaultConfig(conf);

	prepare_inline_font();	// <-- inline font
	nsys->font_size.w = 8;
	nsys->font_size.h = 8;

	nsys->bg_color = 0;
	nsys->fg_color = 0xFFFFFF;
	nsys->ui_color = 0xFFFFFF;

	return nsys;
}

void KB_stopENV(KBenv *env) {

	if (env->font) SDL_FreeSurface(env->font);

	kill_inline_font();

	free(env);

	SDL_Quit();
}


inline void KB_loc(KBenv *env, word base_x, word base_y) {
	env->base_x = base_x;
	env->base_y = base_y;
	env->cursor_x = 0;
	env->cursor_y = 0;
	env->line_height = env->font_size.h;
}

inline void KB_curs(KBenv *env, word cursor_x, word cursor_y) {
	env->cursor_x = cursor_x;
	env->cursor_y = cursor_y;
}

inline void KB_getpos(KBenv *env, word *x, word *y) {
	*x = env->base_x + env->cursor_x * env->font_size.w;
	*y = env->base_y + env->cursor_y * env->line_height;
}

inline void KB_lh(KBenv *env, byte h) {
	env->line_height = (h == 0 ? env->font_size.h : h);
}

void KB_setfont(KBenv *env, SDL_Surface *surf) {
	infont(surf);
	env->font = surf;
	env->font_size.w = surf->w / 16;
	env->font_size.h = surf->h / 8;
	env->line_height = env->font_size.h;
}

void KB_setcolor(KBenv *env, Uint32* colors) /* Colors must be in 0x00RRGGBB format ! */
{
	SDL_Color pal[4];
	int i;
	for (i = 0; i < 4; i++) {
		pal[i].r = (Uint8)((colors[i] & 0x00FF0000) >> 16); 
		pal[i].g = (Uint8)((colors[i] & 0x0000FF00) >> 8);
		pal[i].b = (Uint8)((colors[i] & 0x000000FF));
	}
	SDL_SetColors(env->font, pal, 0, 4);
}

void KB_print(KBenv *env, const char *str) { 
	SDL_Rect dest = { 0 }, letter = { 0 };
	int i, len;

	dest.w = letter.w = env->font_size.w;
	dest.h = letter.h = env->font_size.h;

	len = strlen(str);
	for (i = 0; i < len; i++) {
		int id = (char)str[i];
		int row = id / 16;
		int col = id - (row * 16);
		if (str[i] == '\n') {
			env->cursor_x = 0;
			env->cursor_y++;
			continue;
		}
		letter.x = col * letter.w;
		letter.y = row * letter.h;
		KB_getpos(env, &dest.x, &dest.y);
		SDL_BlitSurface(env->font, &letter, env->screen, &dest);
		env->cursor_x++;
	}
}


void KB_printf(KBenv *env, const char *fmt, ...) { 
	char buf[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(buf, 255, fmt, argptr);
	KB_print(env, buf);
	va_end(argptr);
}

/*
 * SDL Operations.
 * Any SDL resource handler should have those or similar! :)
 * Warning: might not work for non-paletted surfaces, use with caution.
 */
#define SDL_CloneSurfaceX(SURFACE, SIZE) SDL_CreateRGBSurface(SURFACE->flags, SURFACE->w * SIZE, SURFACE->h * SIZE, SURFACE->format->BitsPerPixel, \
		SURFACE->format->Rmask, SURFACE->format->Gmask,	SURFACE->format->Bmask,	SURFACE->format->Amask)
#define SDL_CloneSurfaceHW(SURFACE, H, W) SDL_CreateRGBSurface(SURFACE->flags, W, H, SURFACE->format->BitsPerPixel, \
		SURFACE->format->Rmask, SURFACE->format->Gmask,	SURFACE->format->Bmask,	SURFACE->format->Amask)
#define SDL_ClonePalette(DST, SRC) SDL_SetPalette((DST), SDL_LOGPAL | SDL_PHYSPAL, (SRC)->format->palette->colors, 0, (SRC)->format->palette->ncolors) 

void SDL_SizeX(SDL_Surface *surface, SDL_Surface *new_surface, Uint8 size) {
	Uint32 x, y;
	Uint8 bpp = surface->format->BytesPerPixel, cx, cy, p;
	Uint8 *source, *dest;

	source = (Uint8*)(surface->pixels);
	dest = (Uint8*)(new_surface->pixels);

	for(y = 0; y < surface->h; y++)
	for(x = 0; x < surface->w; x++)
		for (p = 0; p < bpp; p++)
			for (cy = 0; cy < size; cy++)
			for (cx = 0; cx < size; cx++)
			{
				dest[ (y * size + cy) * (new_surface->w * bpp) + ( (x * size + cx) * bpp) + p ] = 
				source[ y * (surface->w * bpp) + (x * bpp) + p ];
			}
}

void SDL_BlitSurfaceFLIP(SDL_Surface *surface, SDL_Rect *src, SDL_Surface *new_surface, SDL_Rect *dst) {
	Uint16 sx, sy, dx, dy;
	Uint8 bpp = surface->format->BytesPerPixel, p;
	Uint8 *source, *dest;

	source = (Uint8*)(surface->pixels);
	dest = (Uint8*)(new_surface->pixels);

	for(sy = src->y, dy = dst->y; sy < src->y + src->h; sy++, dy++)
	for(sx = src->x, dx = dst->x + dst->w - 1; sx < src->x + src->w; sx++, dx--)
		for (p = 0; p < bpp; p++)	
		{
			dest[ dy * (new_surface->w * bpp) + (dx * bpp) + p ] = 
			source[ sy * (surface->w * bpp) + (sx * bpp) + p ];
		}
}

/* 
 * Prepare matching surface and perform AdvancedMAME's scale2x implementation
 */
SDL_Surface* SDL_Scale2X_Surface(SDL_Surface *surface) 
{
	SDL_Surface* new_surface = NULL;

	new_surface = SDL_CloneSurfaceX(surface, 2);

	scale2x(surface, new_surface);

	return new_surface;
}

SDL_Surface* SDL_SizeX_Surface(SDL_Surface* surface, Uint8 size)
{
	SDL_Surface* new_surface = NULL;

	new_surface = SDL_CloneSurfaceX(surface, size);

	SDL_SizeX(surface, new_surface, size);

	return new_surface;
}

SDL_Surface *SDL_Flipped_Surface(SDL_Surface *surface) 
{
	SDL_Surface* new_surface = NULL;
	SDL_Rect src = { 0 };
	SDL_Rect dst = { 0 };

	new_surface = SDL_CloneSurfaceX(surface, 1);

	src.w = dst.w = new_surface->w;
	src.h = dst.h = new_surface->h;	

	SDL_BlitSurfaceFLIP(surface, &src, new_surface, &dst);

	return new_surface;
}


void tell_surface(SDL_Surface *surf) {

	printf("Surface <%p>\n", surf);
	printf("W: %d, H: %d, BPP: %d\n", surf->w, surf->h, surf->format->BitsPerPixel);
	int i = 0;
	for (i = 0; i < 16; i++) {
	SDL_Color *col = &surf->format->palette->colors[i];
	printf("COLOR 0: %02x %02x %02x\n", col->r, col->g, col->b);
	}
}

void SDL_SBlitTile(SDL_Surface *src, int id, SDL_Surface *dst, SDL_Rect *dstrect) {

}

void SDL_BlitTile(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {

}

void SDL_SBlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {

	int zoom = (conf->filter ? 2 : 1);

	SDL_Rect nsrcrect;
	nsrcrect.x = srcrect->x * zoom;
	nsrcrect.y = srcrect->y * zoom;
	nsrcrect.w = srcrect->w * zoom;
	nsrcrect.h = srcrect->h * zoom;

	SDL_Rect ndstrect;
	ndstrect.x = dstrect->x * zoom;
	ndstrect.y = dstrect->y * zoom;
	ndstrect.w = dstrect->w * zoom;
	ndstrect.h = dstrect->h * zoom;

	SDL_BlitSurface(src, &nsrcrect, dst, &ndstrect);
}


/* 
 * Set the the default config.
 * Must be set before any calls to *RESOURCE functions.
 * This is done to avoid passing the same config over and over
 * again in tight loops.
 */ 
void RESOURCE_DefaultConfig(KBconfig* _conf) { conf = _conf; }

/*
 * Load a graphical KB resource using config 'conf' (must be provided
 *  via the 'RESOURCE_DefaultConfig' function beforehand)
 *
 * Take conf->filter into account and zoom accordingly.
 *
 * If 'flip' is set, make the resulting surface twice as large, and
 *  blit a horizontally-flipped copy resulting space.
 * If 'flip' is 1, do that horizontally, vertically if 2.
 */
SDL_Surface *SDL_LoadRESOURCE(int id, int sub_id, int flip) {

	int zoom = (conf->filter ? 2 : 1);

	enum {
		PASS,
	} strategy;

	Uint32 w, h;

	word flip_range = 48;

	SDL_Surface *ret = NULL;

	SDL_Surface *surf = KB_LoadIMG8(id, sub_id);

	if (!surf) return NULL;

	w = surf->w * zoom;
	h = surf->h * zoom;

	if (flip == 2) w *= 2;
	if (flip == 1) h *= 2;

	ret = surf;

	if (w != surf->w || h != surf->h) {

		SDL_Surface *bigsurf = SDL_CloneSurfaceHW(surf, h, w);
		if (surf->format->palette) SDL_ClonePalette(bigsurf, surf);
		else {
			KB_stdlog("Warning: A %d-bpp image\n", surf->format->BitsPerPixel);
		}
		SDL_SetColorKey(bigsurf, SDL_SRCCOLORKEY, 0xFF);

		if (zoom > 1) { /* Zoom-copy */

			if (conf->filter == 1)	SDL_SizeX(surf, bigsurf, 2);
			if (conf->filter == 2)	scale2x(surf, bigsurf);

		} else { /* Flipping only, just copy over */

			SDL_SetColorKey(surf, 0, 0);
			SDL_FillRect(bigsurf, NULL, 0xFF00000);
			SDL_BlitSurface(surf, NULL, bigsurf, NULL); 

		}

		if (flip == 1) {
			SDL_Rect base_rect = { 0, 0 };
			SDL_Rect flip_rect = { 0, 0 };

			flip_rect.w = base_rect.w = surf->w * zoom;
			flip_rect.h = base_rect.h = surf->h * zoom;

			if (flip == 2) flip_rect.x += flip_rect.w;
			if (flip == 1) flip_rect.y += flip_rect.h;
			
			int i;
			flip_range *= zoom;
			SDL_Rect mbase;
			SDL_Rect mflip;
			mbase.h = mflip.h = base_rect.h;
			mbase.w = mflip.w = flip_range;
			mbase.y = base_rect.y;
			mbase.x = base_rect.x;// + base_rect.w - flip_range;
			mflip.y = flip_rect.y;
			mflip.x = flip_rect.x;
			for (i = 0; i < flip_rect.w/flip_range; i++) {
				SDL_BlitSurfaceFLIP(bigsurf, &mbase, bigsurf, &mflip);
				mbase.x += flip_range;
				mflip.x += flip_range;				
			}

			//SDL_BlitSurfaceFLIP(bigsurf, &base_rect, bigsurf, &flip_rect); 			
		} 	


		ret = bigsurf;
		SDL_FreeSurface(surf);
	}

	return ret;
}

SDL_Rect* RECT_LoadRESOURCE(int id, int sub_id) {
	int zoom = (conf->filter ? 2 : 1);
	SDL_Rect *src = (SDL_Rect *)KB_Resolve(id, sub_id);
	SDL_Rect *dst = malloc(sizeof(SDL_Rect));
	dst->x = src->x*zoom;//probably cheaper then memcpy 
	dst->y = src->y*zoom;
	dst->w = src->w*zoom;
	dst->h = src->h*zoom;
	return dst;
}

SDL_Surface* KB_LoadIMG8(int id, int sub_id) {

	SDL_Surface *surf = (SDL_Surface *)KB_Resolve(id, sub_id);
	return surf;
}

void* GNU_Resolve(KBmodule *mod, int id, int sub_id);
void* DOS_Resolve(KBmodule *mod, int id, int sub_id);
void* MD_Resolve(KBmodule *mod, int id, int sub_id);

void* KB_Resolve(int id, int sub_id) {
	int i, l;
	void *ret = NULL;
	i = conf->module;
	l = (conf->num_modules * conf->fallback) + ((i+1) * (1-conf->fallback));
	for (; i < l; i++) {
		KBmodule *mod = &conf->modules[i];
		/* This could be a callback... */
		switch (mod->kb_family) {
			case KBFAMILY_GNU: ret = GNU_Resolve(mod, id, sub_id); break;
			case KBFAMILY_DOS: ret = DOS_Resolve(mod, id, sub_id); break;
			case KBFAMILY_MD:  ret = MD_Resolve(mod, id, sub_id); break;
			default: break;
		}
		if (ret != NULL) break;
	}
	return ret;
}

