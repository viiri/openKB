/*
 * vendor.h -- externs for bare third-party code 
 */

/* scale2x.c */
extern void scale2x(SDL_Surface *src, SDL_Surface *dst);

/* inprint.c */
extern void prepare_inline_font(void);
extern void kill_inline_font(void);
extern void infont(SDL_Surface *font);
extern void incolor(Uint32 fore, Uint32 back);
extern void inprint(SDL_Surface *dst, const char *str, Uint32 x, Uint32 y);

/* savepng.c */
#include "savepng.h"
