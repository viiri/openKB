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

#include "SDL.h"

#include "lib/kbsys.h"
#include "lib/kbconf.h"

typedef struct KBenv {

	SDL_Surface *screen;
	SDL_Surface *font;

	KBconfig *conf;

	int width;
	int height;
	int bpp;

	int fullscreen;
	int filter;

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

	void *udata;

} KBwin;

extern void SDL_SBlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);

extern void RESOURCE_DefaultConfig(KBconfig* _conf);

extern void* KB_Resolve(int id, int sub_id);

extern SDL_Surface* KB_LoadIMG8(int id, int sub_id);

extern SDL_Surface *SDL_LoadRESOURCE(int id, int sub_id, int flip);

#endif /* _OPENKB_ENV */
