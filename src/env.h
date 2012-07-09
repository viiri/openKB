/*
 *  env.h -- game "environment", global data, SDL-targeted
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
#ifndef _OPENKB_ENV
#define _OPENKB_ENV

#include <SDL.h>

#include "lib/kbsys.h"
#include "lib/kbconf.h"

#include "lib/kbsound.h"

typedef struct KBenv {

	SDL_Surface *screen;
	SDL_Surface *font;

	SDL_AudioSpec mixer;
	KBsound *sound;

	KBconfig *conf;

	word cursor_x;	/* Text position */
	word cursor_y;
	word base_x;	/* Pixel offset */
	word base_y;
	byte line_height; /* Line size in pixels */
	
	SDL_Rect font_size;

	Uint32 pan;
	int width;
	int height;
	int bpp;

	int fullscreen;
	int zoom;

} KBenv;

/* Global/main environment */
extern KBenv *sys;

/*
 * At some point (realtime / scale2x / etc), we'll need to 
 * abstract screen blits into separate "window" blits.
 *
 * Something like that could be used...
 */
typedef struct KBwin {

	SDL_Surface *cache;
	int need_redraw;

	int x;
	int y;

	int w;
	int h;

	int flags;

	void *gamestate;
	void *renderer;
	void *udata;

} KBwin;

extern KBenv *KB_startENV(KBconfig *conf);
extern void KB_stopENV(KBenv *env);

extern void KB_flip(KBenv *env);

extern void KB_printf(KBenv *env, const char *fmt, ...);
extern void KB_vprintf(KBenv *env, const char *fmt, va_list argptr);
extern SDL_Rect *KB_fontsize(KBenv *env);
extern void KB_setfont(KBenv *env, SDL_Surface *surf);
extern void KB_setcolor(KBenv *env, Uint32* colors); /* Colors must be in 0x00RRGGBB format ! */
inline void KB_loc(KBenv *env, word base_x, word base_y);
inline void KB_curs(KBenv *env, word cursor_x, word cursor_y);
inline void KB_lh(KBenv *env, byte h);
inline void KB_getpos(KBenv *env, word *x, word *y);

/* Short-hand aliases for above functions */
#define KB_ifont(ARGS...) KB_setfont(sys, ## ARGS)
#define KB_icolor(ARGS...) KB_setcolor(sys, ## ARGS)
#define KB_iloc(ARGS...) KB_loc(sys, ## ARGS)
#define KB_icurs(ARGS...) KB_curs(sys, ## ARGS)
#define KB_ilh(ARGS...) KB_lh(sys, ## ARGS)
#define KB_iprint(ARGS...) KB_print(sys, ## ARGS)
#define KB_iprintf(ARGS...) KB_printf(sys, ## ARGS)
#define KB_ivprintf(ARGS...) KB_vprintf(sys, ## ARGS)


extern void SDL_SBlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);

extern void RESOURCE_DefaultConfig(KBconfig* _conf);

extern void* KB_Resolve(int id, int sub_id);

extern SDL_Surface* KB_LoadIMG8(int id, int sub_id);

extern SDL_Surface *SDL_LoadRESOURCE(int id, int sub_id, int flip);

extern SDL_Rect* RECT_LoadRESOURCE(int id, int sub_id);

extern SDL_Surface* SDL_TakeSurface(int id, int sub_id, int flip);

extern void SDL_FreeCachedSurfaces();

extern char **STRL_LoadArray(int id, int sub_id);
extern void STRL_FreeArray(char **arr);

#endif /* _OPENKB_ENV */
