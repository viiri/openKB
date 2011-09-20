#include "SDL.h"

#include "kbfile.h"
#include "kbres.h"
#include "kbauto.h"
#include "kbstd.h"

#define VILLAIN_OFFSET 0x04B04C
#define CURSOR_OFFSET (0x062D6C - (16 * 8))
#define TROOP_OFFSET 0x000668EC

/* A tile is composed of several sub-tiles */
/* Each subtile is 8 x 8 */
#define SUBTILE_W	8
#define SUBTILE_H	8
/* Lay them in groups of 6 x 4 */
#define SUBGROUP_W	6
#define SUBGROUP_H	4
/* And get a tile (48 x 32) */
#define TILE_W	(SUBTILE_W * SUBGROUP_W)
#define TILE_H	(SUBTILE_H * SUBGROUP_H) 
/* In bytes, that takes */
#define SUBTILE_LEN ((SUBTILE_W * SUBTILE_H) / 2)
#define TILE_LEN (SUBTILE_LEN * (SUBGROUP_W * SUBGROUP_H))

SDL_Surface *MD_LoadIMGROW(const char *buf, int frames) {

	SDL_Rect dstrect = { 0, 0, SUBTILE_W, SUBTILE_H };

	int col, row, frame;
	int pos = 0;

	SDL_Surface *surf = SDL_CreateRGBSurface(SDL_SWSURFACE, TILE_W * frames, TILE_H, 8, 0xFF, 0xFF, 0xFF, 0x00);
	if (surf == NULL) return NULL;

	for (frame = 0; frame < frames; frame++)
	for (col = 0; col < SUBGROUP_W; col++)
	for (row = 0; row < SUBGROUP_H; row++) {

		dstrect.x = col * SUBTILE_W + frame * TILE_W;
		dstrect.y = row * SUBTILE_H;

		SDL_BlitXBPP(&buf[pos], surf, &dstrect, 4);
		put_ega_pal(surf);

		pos += SUBTILE_LEN;
	}

	SDL_Rect f = { 0, 0, surf->w, surf->h };
	SDL_ReplaceIndex(surf, &f, 0, 0xFF);
	SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);

	return surf;
}

void* MD_Resolve(KBmodule *mod, int id, int sub_id) {

	enum {

		UNKNOWN,
		ROW,	
	
	} method = UNKNOWN;

	int frames = 0;
	int offset = 0;

	switch (id) {
		case GR_LOGO:
		{
		}
		break;
		case GR_TROOP:	/* A troop */
		{
			method = ROW;
			offset = TROOP_OFFSET;
			frames = 4;
			offset += (TILE_LEN * frames) * sub_id;
		}
		break;
		case GR_VILLAIN:	/* Villain animated face */
		{
			method = ROW;
			offset = VILLAIN_OFFSET;
			frames = 4;
			offset += (TILE_LEN * frames) * sub_id;
		}
		break;		
		default: break;
	}

	switch (method) {
		case ROW:
		{
			char fullname[PATH_LEN];
			int span_len = TILE_LEN * frames;

			KB_File *f = KB_fopen(mod->slotA_name, "rb");
			KB_fseek(f, offset, 0);

			char buf[span_len];
			int n = KB_fread(buf, sizeof(char), span_len, f);

			KB_fclose(f);

			SDL_Surface *surf = MD_LoadIMGROW(&buf[0], frames);
			return surf;		
		}
		break;
		case UNKNOWN:
		default:
		break;
	}

	return NULL;
}
