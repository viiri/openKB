/*
 * vendor.h -- externs for bare third-party code 
 */

#include "../src/config.h"

#ifdef HAVE_LIBSDL
#include <SDL.h>

/* scale2x.c */
extern void scale2x(SDL_Surface *src, SDL_Surface *dst);

/* inprint.c */
extern void prepare_inline_font(void);
extern void kill_inline_font(void);
extern void infont(SDL_Surface *font);
extern void incolor(Uint32 fore, Uint32 back);
extern void inprint(SDL_Surface *dst, const char *str, Uint32 x, Uint32 y);
extern SDL_Surface* get_inline_font(void);

/* savepng.c */
#include "savepng.h"

#endif

/* strlcat.c */
size_t strlcat(char *dst, const char *src, size_t siz);

/* strlcpy.c */
size_t strlcpy(char *dst, const char *src, size_t siz);
