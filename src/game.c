/*
 *  game.c -- core gameplay routines
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
// For the meantime, use SDL directly, it if gets unwieldy, abstract it away 
#include <SDL.h>

#include "bounty.h"
#include "play.h"
#include "lib/kbconf.h"
#include "lib/kbres.h"
#include "lib/kbauto.h"
#include "lib/kbstd.h"

#include "../vendor/vendor.h" /* scale2x, inprint, etc */

#include "env.h"

#define MAX_HOTSPOTS	64

/* Global/main environment */
KBenv *sys = NULL;

struct {

	SDL_Rect *frames[6];

	SDL_Surface *border[6];

	SDL_Rect map;
	SDL_Rect status;
	SDL_Rect side;

	SDL_Rect *map_tile;
	SDL_Rect *side_tile;

	int boat_flip;
	int hero_flip;

} local = { NULL };

void update_frames();

void prepare_resources() {

	int i;

	for (i = 0; i < 5; i++) 
		local.frames[i] = RECT_LoadRESOURCE(RECT_UI, i);

	//local.border[0] = SDL_LoadRESOURCE(GR_UI, 0, 0);
	//local.border[1] = SDL_LoadRESOURCE(GR_UI, 1, 0);
	//local.border[2] = SDL_LoadRESOURCE(GR_UI, 2, 0);
	//local.border[3] = SDL_LoadRESOURCE(GR_UI, 3, 0);

	local.border[FRAME_MIDDLE] = SDL_LoadRESOURCE(GR_SELECT, 1, 0);

	local.map_tile = RECT_LoadRESOURCE(RECT_TILE, 0);
	local.side_tile = RECT_LoadRESOURCE(RECT_UITILE, 0);

	update_frames();
}

void free_resources() {

	int i;

	free(local.frames[0]);
	free(local.frames[1]);
	free(local.frames[2]);
	free(local.frames[3]);
	free(local.frames[4]);

	SDL_FreeSurface(local.border[FRAME_MIDDLE]);

	free(local.map_tile);
	free(local.side_tile);
	
	SDL_FreeCachedSurfaces();
	
}

void update_frames() {

	SDL_Surface *screen = sys->screen;
	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *left_frame = local.frames[FRAME_LEFT];
	SDL_Rect *right_frame = local.frames[FRAME_RIGHT];
	SDL_Rect *top_frame = local.frames[FRAME_TOP];
	SDL_Rect *bottom_frame = local.frames[FRAME_BOTTOM];
	SDL_Rect *bar = local.frames[FRAME_MIDDLE];
	
	SDL_Rect *purse = local.side_tile;

	local.status.x = left_frame->w;
	local.status.y = top_frame->h;
	local.status.w = screen->w - left_frame->w - right_frame->w;
	local.status.h = fs->h + sys->zoom;

	local.map.x = left_frame->w;
	local.map.y = top_frame->h + local.status.h + bar->h;
	local.map.w = screen->w - purse->w - right_frame->w - left_frame->w;
	local.map.h = screen->h - top_frame->h - bar->h - local.status.h - bottom_frame->h;

}


/* TOH */
KBgame* KB_loadDAT(const char* filename);
int KB_saveDAT(const char* filename, KBgame *game);

/*
 * Hotspot is a possible player action.
 *
 * NOTE: The 'coords'/'timer' union should be taken care of.
 * When hotspot acts as a timer, 'w' and 'h' read from SDL_Rect are zeroes,
 * safely bypassing mouseover tests (e.g. "mouseX >= foo && mouseX < foo + 0").
 * 
 */
typedef struct KBhotspot {

	union {
		SDL_Rect coords;
		struct {
			Uint32 resolution;
			Uint32 passed;
		};
	};

	Uint32	hot_key;
	byte	hot_mod;
	byte	flag;

} KBhotspot;

typedef struct KBgamestate {

	KBhotspot spots[MAX_HOTSPOTS];
	int max_spots;
	int hover;
	Uint32 last;

} KBgamestate;

#define KFLAG_ANYKEY	0x01
#define KFLAG_RETKEY	0x02

#define KFLAG_TIMER 	0x10
#define KFLAG_TIMEKEY	0x20

#define KFLAG_SOFTKEY 	(KFLAG_TIMER | KFLAG_TIMEKEY)

#define SDLK_SYN 0x16

#define SOFT_WAIT 150

#define _NON { 0 }
#define _TIME(INTERVAL) { INTERVAL, 0 }
#define _AREA(X,Y,W,H) { X, Y, W, H }

KBgamestate debug_menu = {
	{
		{	_NON, SDLK_LEFT, 0, KFLAG_RETKEY, },
		{	_NON, SDLK_RIGHT, 0, KFLAG_RETKEY, },
		{	_NON, SDLK_SPACE, 0, KFLAG_RETKEY, },
		0,
	},
	0
};

KBgamestate press_any_key = {
	{
		{	_AREA(0, 0, 1024, 768), 0xFF, 0, KFLAG_ANYKEY },
		0,
	},
	0
};

KBgamestate press_any_key_interactive = {
	{
		{	_AREA(0, 0, 1024, 768), 0xFF, 0, KFLAG_ANYKEY },
		{	_TIME(SOFT_WAIT), SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};

KBgamestate difficulty_selection = {
	{
		{	_NON, SDLK_UP, 0, KFLAG_RETKEY },
		{	_NON, SDLK_DOWN, 0, KFLAG_RETKEY },
		{	_NON, SDLK_RETURN, 0, KFLAG_RETKEY },
		0,
	},
	0
};

KBgamestate enter_string = {
	{
		{	_TIME(100), SDLK_BACKSPACE, 0, KFLAG_RETKEY | KFLAG_SOFTKEY },	
		{	_NON, 0xFF, 0, KFLAG_ANYKEY | KFLAG_RETKEY },
		{	_TIME(60), SDLK_SYN, 0, KFLAG_TIMER | KFLAG_RETKEY },
		0,
	},
	0
};

KBgamestate character_selection = {
	{
		{	_AREA(0, 0, 0, 0), SDLK_a, 0, 0      	},
		{	_AREA(0, 0, 0, 0), SDLK_b, 0, 0      	},
		{	_AREA(0, 0, 0, 0), SDLK_c, 0, 0      	},
		{	_AREA(0, 0, 0, 0), SDLK_d, 0, 0      	},
		{	_AREA(180, 8, 140, 8), SDLK_l, 0, 0   	},
		0,
	},
	0
};

KBgamestate module_selection = {
	{
		{	_NON, SDLK_UP, 0, 0		},
		{	_NON, SDLK_DOWN, 0, 0 	},
		{	_NON, SDLK_RETURN, 0, 0	},
		{	_AREA(0, 0, 0, 0), SDLK_1, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_2, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_3, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_4, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_5, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_6, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_7, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_8, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_9, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_0, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_MINUS, 0, 0	},
		0,
	},
	0
};

KBgamestate savegame_selection = {
	{
		{	_NON, SDLK_UP, 0, 0		},
		{	_NON, SDLK_DOWN, 0, 0 	},
		{	_NON, SDLK_RETURN, 0, 0	},
		{	_AREA(0, 0, 0, 0), SDLK_1, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_2, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_3, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_4, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_5, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_6, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_7, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_8, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_9, 0, 0		},		
		0,
	},
	0
};

#undef _NON
#undef _TIME
#undef _AREA

void render_game(KBenv *env, KBgame *game, KBgamestate *state, KBconfig *conf) {

}

/* Usefull shortcuts */
#define KB_ifont(ARGS...) KB_setfont(sys, ## ARGS)
#define KB_icolor(ARGS...) KB_setcolor(sys, ## ARGS)
#define KB_iloc(ARGS...) KB_loc(sys, ## ARGS)
#define KB_icurs(ARGS...) KB_curs(sys, ## ARGS)
#define KB_ilh(ARGS...) KB_lh(sys, ## ARGS)
#define KB_iprint(ARGS...) KB_print(sys, ## ARGS)
#define KB_iprintf(ARGS...) KB_printf(sys, ## ARGS)

void KB_imenu(KBgamestate *state, int id, int cols) {

	word x, y;

	KB_getpos(sys, &x, &y);

	state->spots[id].coords.x = x;
	state->spots[id].coords.y = y;
	state->spots[id].coords.w = sys->font_size.w * cols;
	state->spots[id].coords.h = sys->font_size.h;
}

/*
 * "inline" functions to adjust SDL_Rect around HOST.
 *
 * HOST could be another SDL_Rect or SDL_Surface, or anything
 * that has ->w and ->h members (thus the usage of macros).
 */
#define RECT_Size(RECT, HOST) \
		(RECT)->w = (HOST)->w, \
		(RECT)->h = (HOST)->h

#define RECT_Pos(RECT, HOST) \
		RECT->x = HOST->x, \
		RECT->y = HOST->y

#define RECT_AddPos(RECT, HOST) \
		RECT->x += HOST->x, \
		RECT->y += HOST->y

#define RECT_Center(RECT, HOST) \
		(RECT)->x = ((HOST)->w - (RECT)->w) / 2, \
		(RECT)->y = ((HOST)->h - (RECT)->h) / 2

#define RECT_Right(RECT, HOST) \
		RECT->x = (HOST->w - RECT->w)

#define RECT_Bottom(RECT, HOST) \
		(RECT)->y = ((HOST)->h - (RECT)->h)


/* In this one, ROWS and COLS of text are used */
#define RECT_Text(RECT, ROWS, COLS) \
		RECT->w = COLS * sys->font_size.w, \
		RECT->h = ROWS * sys->font_size.h

//
// TODO: get rid of those, call RECT_* directly
inline void SDL_CenterRect(SDL_Rect *rect, SDL_Surface *img, SDL_Surface *host) 
{
	RECT_Size(rect, img);
	RECT_Center(rect, host);
}

inline void SDL_CenterRectTxt(SDL_Rect *rect, int rows, int cols, SDL_Surface *host)
{
	SDL_Rect *size = &sys->font_size;
	rect->w = cols * size->w;
	rect->h = rows * size->h;
	RECT_Center(rect, host); 
}

int KB_reset(KBgamestate *state) {

	SDL_Event event;
	int i;

	/* Flush all events (Evil) */
	while (SDL_PollEvent(&event)) 
		;

	/* Reset all timers */
	for (i = 0; i < MAX_HOTSPOTS; i++) {
		if (state->spots[i].hot_key == 0) break;
		if (state->spots[i].flag & KFLAG_TIMER) {
			state->spots[i].coords.w = 0;
		}
	}
}

int KB_event(KBgamestate *state) {
	SDL_Event event;

	int eve = 0;
	int new_hover = -1;

	int click = -1;
	int mouse_x = -1;
	int mouse_y = -1;

	int i;

	static char kbd_state[512] = { 0 };

	Uint32 passed, now;

	/* If we don't know max number of hotspots, let's find out,
	 * ...because it surely beats testing for it 3 times below
	 * TODO? If KB_reset ever becomes mandatory, move it there... 
	 */ 
	if (state->max_spots == 0) {
		for (i = 0; i < MAX_HOTSPOTS; i++) 
			if (state->spots[i].hot_key == 0) break;
		state->max_spots = i;
	}

	/* Update current time */
	now = SDL_GetTicks();
	passed = now - state->last;
	state->last = now;

	/* Trigger "timed hotspots" (basicly, timers) */
	for (i = 0; i < state->max_spots; i++) {
		KBhotspot *sp = &state->spots[i]; 
		if (!(sp->flag & KFLAG_TIMER)) continue;

		sp->passed += passed;

		if (sp->passed >= sp->resolution) {
			sp->passed -= sp->passed;

			/* For "timed keys", also ensure the key is being pressed */
			if (sp->flag & KFLAG_TIMEKEY && !kbd_state[sp->hot_key]) continue;

			eve = i + 1; /* !!! */
			if (sp->flag & KFLAG_RETKEY) eve = sp->hot_key;
			break;
		}
	}

	if (!eve)
	while (SDL_PollEvent(&event)) {
		//switch (event.type) {
		//	case SDL_MOUSEMOTION:
		//}
		if (event.type == SDL_MOUSEMOTION) {
			mouse_x = event.motion.x;
			mouse_y = event.motion.y;
		}
		if ((event.type == SDL_QUIT) ||
			(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
			{
		 		eve = 0xFF;
		 		break;
			}

		if (event.type == SDL_KEYUP) {
			SDL_keysym *kbd = &event.key.keysym;
			kbd_state[kbd->scancode] = 0;
		}

		if (event.type == SDL_KEYDOWN) {
			SDL_keysym *kbd = &event.key.keysym;
			kbd_state[kbd->scancode] = 1;
			for (i = 0; i < state->max_spots; i++) {
				KBhotspot *sp = &state->spots[i];
				if ((sp->flag & KFLAG_ANYKEY) || 
					(sp->hot_key == kbd->sym && 
					( !sp->hot_mod || (sp->hot_mod & kbd->mod) )))
					{
						eve = i + 1; /* !!! */
						if (sp->flag & KFLAG_TIMEKEY)
						{
							sp->passed = 0;
						}
						if (sp->flag & KFLAG_RETKEY)
						{
							eve = kbd->sym; /* !!! */
							if ((kbd->mod & KMOD_SHIFT) && (eve < 128)) { //shift -- uppercase!
								if (eve >= SDLK_a && eve <= SDLK_z) eve -= 32;
								if (eve >= SDLK_0 && eve <= SDLK_9) eve -= 16;
							}
						}
					}
			}
			if (eve) break;
		}
		if (event.type == SDL_MOUSEBUTTONUP) {
			mouse_x = event.button.x;
			mouse_y = event.button.y;
			if (event.button.button == 1) {
				click = 1;
				break;
			}
		}
	}

	/* Mouse moved */
	if (mouse_x != -1 || mouse_y != -1) {
		int zoom = 1;
		SDL_Rect *r;
		mouse_x /= zoom;
		mouse_y /= zoom;
		for (i = 0; i < state->max_spots; i++) {
			r = &state->spots[i].coords;
			if (mouse_x >= r->x && mouse_x < (r->x+r->w)
			 && mouse_y >= r->y && mouse_y < (r->y+r->h)) 
			{
				new_hover = i;
			}
		}
	}

	/* Over a hotspot */
	if (new_hover != -1) {
		state->hover = new_hover;
		if (click != -1) {
			eve = new_hover + 1; /* !!! */
			if (state->spots[new_hover].flag & KFLAG_RETKEY)
				eve = state->spots[new_hover].hot_key;
		}
	}

	return eve;
}

void SDL_TextRect(SDL_Surface *dest, SDL_Rect *r, Uint32 fore, Uint32 back) {

	int i, j;

	SDL_Rect *fs = &sys->font_size;

	/* Choose colors, Fill Rect */
	SDL_FillRect(dest, r, back);	
	incolor(fore, back);

	/* Top and bottom (horizontal fill) */
	for (i = r->x ; i + fs->w < r->x + r->w; i += fs->w) {
		inprint(dest, "\x0E", i, r->y);
		inprint(dest, "\x0F", i, r->y + r->h - fs->h);
	}
	/* Left and right (vertical fill) */
	for (j = r->y ; j + fs->h < r->y + r->h; j += fs->h) {
		inprint(dest, "\x14", r->x, j);
		inprint(dest, "\x15", i, j);
	}
	/* Corners */
	inprint(dest, "\x10", r->x, r->y);/* Top-left */ 
	inprint(dest, "\x11", i, r->y);/* Top-right */
	inprint(dest, "\x12", r->x, r->y + r->h - fs->h);/* Bottom-left */
	inprint(dest, "\x13", i, r->y + r->h - fs->h);/* Bottom-right */
}

/* Enter name */
char *text_input(int max_len, int numbers_only, int x, int y) {
	static char entered_name[11];

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	SDL_Rect menu;
	SDL_Rect *fs = &sys->font_size;

	entered_name[0] = '\0';
	int curs = 0;

	const char *twirl = "\x1D" "\x05" "\x1F" "\x1C" ; /* stands for: | / - \ */  
	byte twirl_pos = 0;

	while (!done) {

		key = KB_event(&enter_string);

		if (key == 0xFF) { curs = 0; done = -1; }

		if (key == SDLK_RETURN) {
			done = 1;
		}
		else
		if (key == SDLK_BACKSPACE) {
			if (curs) {
				entered_name[curs] = ' ';
				curs--;
				entered_name[curs] = ' ';
				redraw = 1;
			}
		}
		else
		if (key == SDLK_SYN) {
			twirl_pos++;
			if (twirl_pos > 3) twirl_pos = 0;
			redraw = 1;
		} else
		if (key) {
			if (key < 128 && isascii(key) && 
				 ( !numbers_only ||
				   ( isdigit(key) ) 
				 )
			   ) {
				if (curs < max_len) {
					entered_name[curs] = (char)key;
					curs++;
					entered_name[curs] = '\0';
					redraw = 1;
				}
			}
		}

		if (redraw) {

			KB_iloc(x, y);
			KB_iprintf("%s", entered_name);
			KB_iloc(x + curs * fs->w, y);
			KB_iprintf("%c", twirl[twirl_pos]);

	    	SDL_Flip( screen );

			redraw = 0;
		}
	}

	inprint(screen, " ", x + curs * 8, y);
	entered_name[curs] = '\0';

	return (done == -1 ? NULL : &entered_name[0]);	
}

/* Wait for a keypress */
inline void KB_Pause() { 
 	while (!KB_event(&press_any_key)) SDL_Delay(10);
}

/* Change active colors */
void KB_MessageColor(Uint32 bg, Uint32 fg, Uint32 frame) {
	sys->bg_color = bg;
	sys->fg_color = fg;
	sys->ui_color = frame;
	incolor(fg, bg);
}

/* Display a message. Wait for a key to discard. */
SDL_Rect* KB_MessageBox(const char *str, int wait) {

	SDL_Surface *screen = sys->screen;
	Uint32 bg = sys->bg_color;
	Uint32 fg = sys->fg_color;
	Uint32 ui = sys->ui_color;

	SDL_Rect *fs = &sys->font_size;

	int i, max_w = 28;

	static SDL_Rect rect;

	if (wait > 1) { 
		wait = 0;
		max_w = 30;
	}

	/* Faux-pass to get max dimensions */
	int h = 0, w = 0; i = 0;
	do {
		w++;
		if (str[i] == '\n' || str[i] == '\0' || (str[i] == ' ' && w > max_w - 3)) {
			w = 0;
			h++;
		}
		i++;
	} while (str[i - 1] != '\0');

	/* Keep in rect */
	rect.w = max_w * fs->w;
	rect.h = h * fs->h;

	/* To the center of the screen */
	rect.x = (screen->w - rect.w) / 2;
	rect.y = (screen->h - rect.h) / 2;

	/* A little bit up */
	rect.y -= fs->h;//*2 !!

	/* A nice frame */
	rect.x -= fs->w;rect.y -= fs->h;rect.w += fs->w*2;rect.h += (fs->h*2);
	SDL_TextRect(screen, &rect, ui, bg);
	rect.x += fs->w;rect.y += fs->h;rect.w -= fs->w*2;rect.h -= (fs->h*2);
	SDL_FillRect(screen, &rect, 0xFF0000);

	/* Restore color */
	incolor(fg, bg);

	/* True-pass */
	i = 0; h = 0; w = 0;
	char buffer[80];
	do {
		buffer[w++] = str[i];
		if (str[i] == '\n' || str[i] == '\0' || (str[i] == ' ' && w > max_w - 3)) {
			buffer[w] = '\0';
			buffer[w-1] = '\0';
			inprint(screen, buffer, rect.x, rect.y + h * fs->h);
			w = 0;
			buffer[0] = '\0';
			h++;
		}
		i++;
	} while (str[i - 1] != '\0');

	SDL_Flip(screen);

	if (wait) KB_Pause();

	return &rect;
}

SDL_Rect* KB_BottomFrame() {
	SDL_Surface *screen = sys->screen;

	Uint32 bg = sys->bg_color;
	Uint32 fg = sys->fg_color;
	Uint32 ui = sys->ui_color;

	SDL_Rect *fs = &sys->font_size;
	
	SDL_Rect *left_frame = local.frames[FRAME_LEFT];
	SDL_Rect *bottom_frame = local.frames[FRAME_BOTTOM];

	SDL_Rect border;
	static SDL_Rect text;

	/* Make a 30 x 8 border (+1 character on each side) */ 
	RECT_Text((&border), 8, 30);
	border.h += fs->h / 2; /* Plus some extra pixels, because 'bottom box' is uneven */
	border.x = left_frame->w;
	border.y = screen->h - bottom_frame->h - border.h;

	/* Actual text is 28 x 6 (meaning 28 cols per 6 rows, btw) */
	RECT_Pos((&text), (&border));
	text.y += fs->h;
	text.x += fs->w;
	RECT_Text((&text), 6, 28);

	/* A nice frame */
	SDL_FillRect(screen, &border, 0xFF0000);
	SDL_TextRect(screen, &border, ui, bg);
	SDL_FillRect(screen, &text, 0x00FF00);

	return &text;
}

SDL_Rect* KB_BottomBox(const char *header, const char *str, int wait) {

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *text = KB_BottomFrame();	

	/* Header (few pixels up) */	
	KB_iloc(text->x, text->y - fs->h/4 - fs->h/8);
	if (header) KB_iprint(header);

	/* Message */
	KB_iloc(text->x, text->y + fs->h/4);
	if (header) KB_iprint("\n\n");	
	KB_iprint(str);

	SDL_Flip(sys->screen);

	if (wait) KB_Pause();

	return text;
}

SDL_Rect* KB_TopBox(const char *str) {

	Uint32 *colors = KB_Resolve(COL_TEXT, 0);	

	/* Clean line */
	SDL_FillRect(sys->screen, &local.status, colors[0]);

	/* Print string */
	KB_iloc(local.status.x, local.status.y + 1);
	KB_iprint(str);

	return &local.status;
}

/* "create game" screen (pick name and difficulty) */
KBgame *create_game(int pclass) {

	KBgame *game = NULL;

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	SDL_Rect menu;
	
	int cols = 30;//w
	int rows = 12;//h

	SDL_Rect *fs = &sys->font_size;

	SDL_CenterRectTxt(&menu, rows, cols, screen);

	int has_name = 0;
	char *name;

	int sel = 1;

	while (!done) {

		key = KB_event(&difficulty_selection);

		if (key == 0xFF) done = 1;

		if (key == SDLK_UP) {
			sel--;
			if (sel < 0) sel = 0;
			redraw = 1;
		}

		if (key == SDLK_DOWN) {
			sel++;
			if (sel > 3) sel = 3;
			redraw = 1;
		}

		if (key == SDLK_RETURN) {
			game = spawn_game(name, pclass, sel);
			done = 1;
		}

		if (redraw) {

			SDL_TextRect(screen, &menu, 0xFFFFFF, 0x000000);

			KB_iloc(menu.x + fs->w, menu.y + fs->h);	
			KB_iprintf(" %-9s Name: ", classes[pclass][0].title);

			if (has_name) KB_iprint(name);

			KB_iloc(menu.x + fs->w, menu.y + fs->h * 3);
			KB_iprint("   Difficulty   Days  Score\n");
			KB_iprint("\n");
			KB_iprint("   Easy         900    x.5 \n");
			KB_iprint("   Normal       600     x1 \n");
			KB_iprint("   Hard         400     x2 \n");
			KB_iprint("   Impossible?  200     x4 \n");

			if (has_name) {
				KB_iloc(menu.x + fs->w, menu.y + fs->h * 10);
				KB_iprint("\x18\x19 to select   Ent to Accept");		

				int i;
				KB_iloc(menu.x + fs->w, menu.y + fs->h * 5);
				for (i = 0; i < 4; i++) 
					KB_iprint((sel == i ? ">\n" : " \n"));
			}

	    	SDL_Flip( screen );

			redraw = 0;
		}

		if (!has_name) {
			name = text_input(10, 0, menu.x + fs->w * 18, menu.y + fs->h);
			if (name == NULL || name[0] == '\0') done = 1;
			else has_name = 1;
			redraw = 1;
		}

	}

	return game;
}

/* load game screen (pick savefile) */
KBgame *load_game() {
	SDL_Surface *screen = sys->screen;
	KBconfig *conf = sys->conf;
	
	KBgame *game = NULL;

	int done = 0;
	int redraw = 1;
	int key = 0;

	int sel = 0;

	SDL_Rect menu;

	char filename[10][16];
	char fullname[10][16];
	byte cache[10] = { 0 };
	byte rank[10];
	byte pclass[10];
	int num_files = 0;

	SDL_Rect *fs = &sys->font_size;

	KB_DIR *d = KB_opendir(conf->save_dir);
	KB_Entry *e;
    while ((e = KB_readdir(d)) != NULL) {
		if (e->d_name[0] == '.') continue;
		
		char base[255];
		char ext[255];
		
		name_split(e->d_name, base, ext);

		if (strcasecmp("DAT", ext)) continue;
		strcpy(filename[num_files], base);
		strcpy(fullname[num_files], e->d_name);
		num_files++;
    }
	KB_closedir(d);

	if (num_files == 0) {
		KB_MessageBox("This disk has no characters on it. Try creating a new\ncharacter or copy one from another disk.", 0);
		return NULL;		
	}

	/* Prepare menu */
	int i, l = 0;
	for (i = 0; i < num_files; i++) {
		int mini_l = strlen(filename[i]);
		if (mini_l > l) l = mini_l;
	}

	l = 14;

	/* Size */
	menu.w = l * fs->w + fs->w * 2;
	menu.h = num_files * fs->h + fs->h * 6;

	/* To the center of the screen */
	menu.x = (screen->w - menu.w) / 2;
	//menu.y = (screen->h - menu.h) / 2;

	/* A little bit up */
	menu.y = fs->h + 4 * fs->h;

	/* Update mouse hot-spots */
	for (i = 0; i < num_files; i++) {
		savegame_selection.spots[i + 3].coords.x = menu.x + fs->w * 2;
		savegame_selection.spots[i + 3].coords.y = menu.y + fs->h * 4 + i * fs->h;
		savegame_selection.spots[i + 3].coords.w = menu.w - fs->w;
		savegame_selection.spots[i + 3].coords.h = fs->h;
	}

	while (!done) {

		key = KB_event( &savegame_selection );
		if (savegame_selection.hover - 3 != sel) {
			sel = savegame_selection.hover - 3;
			redraw = 1;
		}

		if (key == 0xFF) done = 1;

		if (key == 1) { sel--; redraw = 1; }
		if (key == 2) { sel++; redraw = 1; }
		if (key >= 3 && key != 0xFF) {
			char buffer[PATH_LEN];
			KB_dircpy(buffer, conf->save_dir);
			KB_dirsep(buffer);
			KB_strcat(buffer, fullname[sel]);

			game = KB_loadDAT(buffer);
			if (game == NULL) KB_errlog("Unable to load game %s\n", buffer);
			done = 1;
		}
		//if (key == 3) { done = 1; }
		//if (key >= 4) { done = 1; }

		if (sel < 0) sel = 0;
		if (sel > num_files - 1) sel = num_files - 1;
		savegame_selection.hover = sel + 3;

		if (redraw) {

			int i;

			SDL_TextRect(screen, &menu, 0xFFFFFF, 0x000000);

			incolor(0xFFFFFF, 0x000000);
			inprint(screen, " Select game:", menu.x + fs->w, menu.y + fs->w - fs->w/2);
			inprint(screen, "   Overlord  ", menu.x + fs->w, menu.y + fs->w*2);
			inprint(screen, "             ", menu.x + fs->w, menu.y + fs->w*4 - fs->w/2);

			for (i = 0; i < num_files; i++) {
				incolor(0xFFFFFF, 0x000000);
				char buf[4];sprintf(buf, "%d.", i + 1);
				inprint(screen, buf, menu.x + fs->w*2, fs->h * i + menu.y + fs->h*4);

				if (i == sel) incolor(0x000000, 0xFFFFFF);
				inprint(screen, filename[i], menu.x + fs->w * 5, fs->h * i + menu.y + fs->h*4);
			}

	incolor(0xFFFFFF, 0x000000);
	inprint(screen, " 'ESC' to exit \x18\x19 Return to Select  ", local.status.x, local.status.y);

	    	SDL_Flip( screen );

			redraw = 0;
		}

	}

	return game;
}

void show_credits() {

	SDL_Rect *fs = &sys->font_size;

	SDL_Surface *userpic = SDL_LoadRESOURCE(GR_SELECT, 2, 0);

	SDL_Rect *max;

	KB_MessageColor(0x0000AA, 0xFFFFFF, 0xFFFF55);

	char *credits = KB_Resolve(STRL_CREDITS, 0);
	if (credits == NULL) return;
	int i, j = 0, n = 10;
	char *credit = credits;
	for (i = 0; i < n; ) {
		if (*credit == '\0') {
			i++;
			*credit = '\n';
		}
		credit++;
	}
	
	max = KB_MessageBox(credits, 2);

	SDL_Rect pos = { 0 };

	RECT_Size((&pos), userpic);
	RECT_Right((&pos), max);
	RECT_AddPos((&pos), max);	

	pos.y += (fs->h * 2);

	SDL_BlitSurface(userpic, NULL, sys->screen, &pos);

	SDL_Flip(sys->screen); 

	SDL_FreeSurface(userpic);

	KB_Pause();
}

KBgame *select_game(KBconfig *conf) {

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	int credits = 1;

	KBgame *game = NULL;

	Uint32 *colors = KB_Resolve(COL_TEXT, 0);

	SDL_Surface *title = SDL_LoadRESOURCE(GR_SELECT, 0, 0);

	while (!done) {

		key = KB_event(&character_selection);

		if (redraw) {

			SDL_Rect pos;

			SDL_CenterRect(&pos, title, screen);

			SDL_BlitSurface( title, NULL , screen, &pos );

			if (!credits) {
				KB_iloc(local.status.x, local.status.y);
				KB_icolor(colors);
				KB_iprint("Select Char A-D or L-Load saved game");
			}

	    	SDL_Flip( screen );

			redraw = 0;
		}

		if (credits) {
			show_credits();
			credits = 0;
			redraw = 1;
		}

		/* New game (class 1-4) */
		if (key > 0 && key < 5) 
		{
			game = create_game(key - 1);
			if (game) done = 1;
			redraw = 1;
		}

		/* Pressed 'L' */
		if (key == 5) {
			game = load_game();
			if (game) done = 2;
			redraw = 1;
		}

		/* Escape */
		if (key == 0xFF) done = 1;
	}

	if (game) {
		if (done == 1) KB_MessageBox("%s the %s,\n\nA new game is being created. Please wait while I perform godlike actions to make this game playable.",  0);
		if (done == 2) KB_MessageBox("%s the %s,\n\nPlease wait while I prepare a suitable environment for your bountying enjoyment!",  0);
	}

	SDL_FreeSurface(title);

	return game;
}

int select_module() {
	SDL_Surface *screen = sys->screen;
	KBconfig *conf = sys->conf;

	int done = 0;
	int redraw = 1;
	int key = 0;

	int sel = 0;

	SDL_Rect menu;

	/* Just for fun, do some error-checking */
	if (conf->num_modules < 1) {
		KB_errlog("No modules found! You must either enable auto-discovery, either provide explicit file paths via config file.\n"); 	
		return -1;
	}

	/* Prepare menu */
	int i, l = 0;
	for (i = 0; i < conf->num_modules; i++) {
		int mini_l = strlen(conf->modules[i].name);
		if (mini_l > l) l = mini_l;
	}

	/* Size */
	menu.w = l * 8 + 16;
	menu.h = conf->num_modules * 8 + 16;

	/* To the center of the screen */
	menu.x = (screen->w - menu.w) / 2;
	menu.y = (screen->h - menu.h) / 2;

	/* A little bit up */
	menu.y -= 8;

	/* Update mouse hot-spots */
	for (i = 0; i < conf->num_modules; i++) {
		module_selection.spots[i + 3].coords.x = menu.x + 8;
		module_selection.spots[i + 3].coords.y = menu.y + 8 + i * 8;
		module_selection.spots[i + 3].coords.w = menu.w - 8;
		module_selection.spots[i + 3].coords.h = 8;
	}

	while (!done) {

		key = KB_event( &module_selection );
		if (module_selection.hover - 3 != sel) {
			sel = module_selection.hover - 3;
			redraw = 1;
		}

		if (key == 0xFF) { /* Sudden Escape */
			return -1;
		}
		if (key == 1) { sel--; redraw = 1; }
		if (key == 2) { sel++; redraw = 1; }
		if (key == 3) { done = 1; }
		if (key >= 4) { done = 1; }

		if (sel < 0) sel = 0;
		if (sel > conf->num_modules - 1) sel = conf->num_modules - 1;
		module_selection.hover = sel + 3;

		if (redraw) {

			int i;

			SDL_FillRect( screen, NULL, 0x333333 );

			SDL_TextRect(screen, &menu, 0, 0xFFFFFF);

			for (i = 0; i < conf->num_modules; i++) {
				if (i == sel)	incolor(0xFFFFFF, 0x000000);
				else			incolor(0x000000, 0xFFFFFF);

				inprint(screen, conf->modules[i].name, menu.x + 8, 8 * i + menu.y + 8);
			}

	    	SDL_Flip( screen );

			redraw = 0;
		}

	}

	printf("SELECTED MODULE: %d - { %s } \n", sel, conf->modules[sel].name);

	conf->module = sel;
	return sel;
}

void display_logo() {

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	while (!done) {

		key = KB_event(&press_any_key);

		if (key) done = 1;

		if (redraw) {

			SDL_Rect pos;

			SDL_Surface *title = SDL_LoadRESOURCE(GR_LOGO, 0, 0);

			SDL_CenterRect(&pos, title, screen);

			SDL_FillRect( screen , NULL, 0xFF3366);

			SDL_BlitSurface( title, NULL , screen, &pos );

	    	SDL_Flip( screen );

			SDL_FreeSurface(title);

			SDL_Delay(10);

			redraw = 0;
		}

	}
 
}

void display_title() {

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	while (!done) {

		key = KB_event(&press_any_key);

		if (key) done = 1;

		if (redraw) {

			SDL_Rect pos;

			SDL_Surface *title = SDL_LoadRESOURCE(GR_TITLE, 0, 0);

			SDL_CenterRect(&pos, title, screen);

			SDL_FillRect( screen , NULL, 0x000000);

			SDL_BlitSurface( title, NULL , screen, &pos );

	    	SDL_Flip( screen );

			SDL_FreeSurface(title);

			redraw = 0;
		}

	}

}
#define TILE_W 48

void display_debug() {

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	int troop_id = 0;
	int troop_frame = 0;
	SDL_Rect src = {0, 0, TILE_W, 0 };
	
	int heads = 0;
	
	RESOURCE_DefaultConfig(sys->conf);

	while (!done) {

		key = KB_event(&debug_menu);

		if (key == 0xFF) done = 1;
		if (key == SDLK_LEFT) troop_id--;
		if (troop_id < 0) troop_id = 0;
		if (key == SDLK_RIGHT) troop_id++;
		if (key == SDLK_SPACE) heads = 1 - heads;
		if (troop_id > MAX_TROOPS - 1) troop_id = MAX_TROOPS - 1;

		if (redraw) {

			SDL_Rect pos;
			SDL_Rect pos2 = { TILE_W, 0, TILE_W, 0 };

			SDL_Surface *title = SDL_LoadRESOURCE(GR_TROOP, troop_id, 0);
			SDL_Surface *font = SDL_LoadRESOURCE(GR_FONT, 0, 0);
			SDL_Surface *peasant = KB_LoadIMG8(GR_TROOP, troop_id);
			//sys->conf->module++;
			int gr = GR_TROOP;
			if (heads) gr = GR_VILLAIN;
			SDL_Surface *peasant2 = SDL_LoadRESOURCE(gr, troop_id, 1);
			//sys->conf->module--;

		src.h = peasant->h;
		pos2.h = peasant2->h/2;

			SDL_CenterRect(&pos, peasant2, screen);

			SDL_FillRect( screen , NULL, 0x4664B4);

			{
				SDL_Surface *grass = SDL_LoadRESOURCE(GR_COMTILES, 0, 0);
				int gw = grass->w / 15;
				SDL_Rect gsrc = { 0, 0, gw, grass->h };
				SDL_Rect gdst = { 0, 0, gw, grass->h };
				int gi, gj;
				for (gj = 0; gj < 10; gj++) {
				for (gi = 0; gi < 10; gi++) {
				
					gdst.x = gw * gi;
					gdst.y = grass->h * gj;
					SDL_BlitSurface(grass, &gsrc, screen, &gdst);
				} }
				SDL_FreeSurface(grass);
			}


			SDL_BlitSurface( peasant2, NULL , screen, &pos );

			//SDL_BlitSurface( font, NULL , screen, &pos );

			SDL_Rect right = { 0, 0, peasant->w, peasant2->h/2 };

			src.x = troop_frame * TILE_W;
			src.h = peasant2->h/2;
			troop_frame++;
			if (troop_frame > 3) troop_frame = 0; 			

			SDL_BlitSurface( peasant, &src , screen, NULL );
			
			src.y += peasant2->h/2;
			SDL_SBlitSurface( peasant2, &src , screen, &pos2 );
			src.y -= peasant2->h/2;

	    	SDL_Flip( screen );

			SDL_FreeSurface(title);
			SDL_FreeSurface(font);
			SDL_FreeSurface(peasant);

			SDL_Delay(300);

			redraw = 1;
		}

	}
 
}

void KB_BlitMap(SDL_Surface *dest, SDL_Surface *tileset, SDL_Rect *viewport) {

	KBconfig *conf = sys->conf;


}


KBgamestate yes_no_question = {
	{
		{	{ 0 }, SDLK_y, 0, 0      	},
		{	{ 0 }, SDLK_n, 0, 0      	},
		0
	},
	0
};

KBgamestate five_choices = {
	{
		{	{ 0 }, SDLK_a, 0, 0      	},
		{	{ 0 }, SDLK_b, 0, 0      	},
		{	{ 0 }, SDLK_c, 0, 0      	},
		{	{ 0 }, SDLK_d, 0, 0      	},
		{	{ 0 }, SDLK_e, 0, 0      	},

		{	{ 60 }, SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};


void draw_location(int loc_id, int troop_id, int frame) {
	SDL_Surface *bg = SDL_LoadRESOURCE(GR_LOCATION, loc_id, 0);
	SDL_Surface *troop = SDL_LoadRESOURCE(GR_TROOP, troop_id, 1);

	SDL_Rect pos;
	SDL_Rect tpos;
	SDL_Rect troop_frame = { 0, 0, troop->w / 4, troop->h };

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *left_frame = RECT_LoadRESOURCE(RECT_UI, 1);
	SDL_Rect *top_frame = RECT_LoadRESOURCE(RECT_UI, 0);
	SDL_Rect *bar_frame  = RECT_LoadRESOURCE(RECT_UI, 4);

	RECT_Size(&pos, bg); 
	pos.x = left_frame->w;
	pos.y = top_frame->h + bar_frame->h + fs->h + sys->zoom;

	troop_frame.x = frame * troop_frame.w;

	tpos.x = troop_frame.w * 1;
	RECT_Size(&tpos, &troop_frame);
	RECT_Bottom(&tpos, bg);
	RECT_AddPos((&tpos), (&pos));
	tpos.y -= sys->zoom;

	SDL_BlitSurface(bg, NULL, sys->screen, &pos);

	SDL_BlitSurface(troop, &troop_frame, sys->screen, &tpos);

	SDL_FreeSurface (bg);
	free(left_frame);
	free(top_frame);
	free(bar_frame);
}

void view_puzzle(KBgame *game) {

	SDL_Surface *artifacts = SDL_LoadRESOURCE(GR_VIEW, 0, 0);
	
	SDL_Surface *tile = SDL_LoadRESOURCE(GR_TILE, 0, 0);
	
	SDL_Surface *tileset = SDL_LoadRESOURCE(GR_TILESET, 0, 0);

	SDL_Surface *faces[MAX_VILLAINS];


	/*---*/
	SDL_Rect *fs = &sys->font_size;
	SDL_Rect pos;
	SDL_Rect *left_frame = RECT_LoadRESOURCE(RECT_UI, 1);
	SDL_Rect *top_frame = RECT_LoadRESOURCE(RECT_UI, 0);
	SDL_Rect *bar_frame  = RECT_LoadRESOURCE(RECT_UI, 4);

	//RECT_Size(&pos, bg); 
	pos.x = left_frame->w;
	pos.y = top_frame->h + bar_frame->h + fs->h + sys->zoom;
	/*---*/

	int i;	
	
	for (i = 0; i < MAX_VILLAINS; i++) {

		faces[i] = SDL_LoadRESOURCE(GR_VILLAIN, i, 0);

	}

	SDL_Surface *screen = sys->screen;

	int j;
	
	int border_x = game->scepter_x - 3;
	int border_y = game->scepter_y - 3;

	int frame = 0;
	int done = 0;
	int redraw = 1;
	while (!done) {

		int key = KB_event(&press_any_key_interactive);
		
		if (key == 0xFF) done = 1;
		
		if (key == 2) {
			frame++;
			if (frame > 3) frame = 0;
			redraw = 1;
		}

		if (redraw) {
			
			for (j = 0; j < 5; j ++) {	
				for (i = 0; i < 5; i ++) {

					int id = puzzle_map[j][i];

					SDL_Rect dst = { pos.x + i * tile->w, pos.y + j * tile->h, tile->w, tile->h };

					if (id < 0) {

						int artifact_id = -id - 1;
						
						if (game->artifact_found[artifact_id]) {
						
						
						} else {

							SDL_Rect src = { artifact_id * tile->w, 0, tile->w, tile->h };
	
							SDL_BlitSurface( artifacts, &src, screen, &dst);

						}

					} else {


						if (game->villain_caught[id] ) {
						
						} else {
							SDL_Rect src = { frame * tile->w, 0, tile->w, tile->h };
	
							SDL_BlitSurface( faces[id], &src, screen, &dst);
						}
					}
				}
			}

			SDL_Flip(sys->screen);
			redraw = 0;
		}
	}

	

	KB_Pause();
}

KBgamestate minimap_toggle = {
	{
		{	{ 0 }, SDLK_SPACE, 0, 0      	},
		0
	},
	0
};

void view_minimap(KBgame *game) {
	SDL_Rect border;

	SDL_Surface *screen = sys->screen;

	SDL_Rect *fs = &sys->font_size;

	RECT_Text((&border), 19, 20);
	RECT_Center(&border, sys->screen);

	border.y += fs->h;
	border.x -= fs->w * 3;

	Uint32 *colors = KB_Resolve(COL_TEXT, 0);

	Uint32 *map_colors = KB_Resolve(COL_MINIMAP, 0);

	SDL_Surface *tile = SDL_LoadRESOURCE(GR_PURSE, 0, 0);

	SDL_TextRect(sys->screen, &border, colors[0], colors[1]);

	SDL_Rect map;

	SDL_Rect pixel;

	pixel.w = sys->zoom * 2;
	pixel.h = sys->zoom * 2;

	map.x = border.x + fs->w*2;
	map.y = border.y + fs->h*2 - fs->h/2;
	map.w = LEVEL_W * pixel.w;
	map.h = LEVEL_H * pixel.h;

	SDL_FillRect(sys->screen, &map, 0x112233);

	KB_iloc(border.x + fs->w, border.y + fs->h/2);
	KB_iprintf("   %s", continent_names[game->continent]);

	KB_iloc(border.x + fs->w, border.y + map.h + (fs->h*2) - fs->h/2);
	KB_iprintf("X=%d Position Y=%d", game->x, game->y);

	int done = 0;
	int redraw = 1;
	int orb = 0;
	int have_orb = 1;
	while (!done) {
	
		int key = KB_event(&minimap_toggle);

		if (key == 0xFF) done = 1;
		else if (key) { /* Toggle orb */
			if (game->orb_found[game->continent]) {
				orb = 1 - orb;
				redraw = 1;
			}
		}

		if (redraw) {

			int i;
			int j;

			if (!game->orb_found[game->continent])
				KB_TopBox("        Press 'ESC' to exit");
			else if (!orb)
				KB_TopBox("  'ESC' to exit / 'SPC' whole map");
			else
				KB_TopBox("  'ESC' to exit / 'SPC' your map");

			for (j = 0; j < LEVEL_H; j++) {
				for (i = 0; i < LEVEL_W; i++) {
		
					Uint32 color = 0;
		
					pixel.x = map.x + (i * pixel.w);
					pixel.y = map.y + ((LEVEL_H - j - 1) * pixel.h);
		
					if (orb || game->fog[game->continent][j][i]) { 
						byte tile = game->map[game->continent][j][i];
						color = map_colors[tile];
					}
					SDL_FillRect(screen, &pixel, color);
				}
				/* DOS aestetics: */
				/**/SDL_Flip(sys->screen);
				/**/SDL_Delay(10);
			}

			SDL_Flip(sys->screen);
			redraw = 0;
		}

	}
}

void view_contract(KBgame *game) {

	SDL_Rect border;

	SDL_Surface *screen = sys->screen;

	SDL_Rect *fs = &sys->font_size;

	RECT_Text((&border), 16, 36);
	RECT_Center(&border, sys->screen);
	
	border.y += fs->h/2;

	Uint32 *colors = KB_Resolve(COL_TEXT, 0);

	SDL_Surface *tile = SDL_LoadRESOURCE(GR_PURSE, 0, 0);

	SDL_TextRect(sys->screen, &border, colors[0], colors[1]);

	SDL_Rect hdst = { border.x + fs->w, border.y + fs->h, tile->w, tile->h };

	if (game->contract == 0xFF) {

		SDL_Surface *sidebar = SDL_LoadRESOURCE(GR_UI, 0, 0);

		KB_iloc(border.x + fs->w, border.y + fs->h);
		KB_iprint("\n\n\n\n\n\n      You have no Contract!");

		/* And a face */
		/* Empty Contract */
		SDL_Rect hsrc = { 8 * tile->w, 0, tile->w, sidebar->h };
		SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);

		SDL_Flip(sys->screen);
		
		KB_Pause();

		SDL_FreeSurface(sidebar);

	} else {

		byte villain_id = game->contract;

		SDL_Surface *face = SDL_LoadRESOURCE(GR_VILLAIN, villain_id, 0);

		int j, continent = -1, castle = -1;

		int desc_line = villain_id * 14;
		
		byte line_offsets[14] = {
			7,
			7,
			7,
			7,
			10,
			0,
		};

		/* Find his castle */
		for (j = 0; j < MAX_CASTLES; j++) {
			if (game->castle_owner[j] == KBCASTLE_PLAYER
			 || game->castle_owner[j] == KBCASTLE_MONSTERS) continue;
			continent = game->continent;
			if (game->castle_owner[j] & KBCASTLE_KNOWN)
				castle = j;
			else
				break; /* No point in continuing from here, castle has been found */
		}

		/* Print all 14 lines */
		KB_iloc(border.x + fs->w, border.y + fs->h);
		for (j = 0; j < 14; j++) {
			KB_icurs( line_offsets[j], j);
			char *text = KB_Resolve(STR_VDESC, desc_line + j);
			KB_iprintf("%s\n", text);
		}

		/* Print known info (continent and castle of residence) */
		KB_icurs(18, 3);
		if (continent == -1)
			KB_iprint("Unknown");
		else
			KB_iprint(continent_names[continent]);
		KB_icurs(18, 4);
		if (castle == -1)
			KB_iprint("Unknown");
		else
			KB_iprint(castle_names[castle]);

		int done = 0;
		int frame = 0;
		int redraw = 1;
		while (!done) {

			int key = KB_event(&press_any_key_interactive);

			if (key == 2) {
				frame++;
				if (frame > 3) {
					frame = 0;
				}
				redraw = 1;
			} else if (key) done = 1;

			if (redraw) {

				/* Blit face */
				SDL_Rect hsrc = { frame * tile->w, 0, tile->w, tile->h };
				SDL_BlitSurface( face, &hsrc, screen, &hdst);

				SDL_Flip(sys->screen);
				redraw = 0;
			}
		}

		SDL_FreeSurface(face);
	}


	SDL_FreeSurface(tile);
}

void view_character(KBgame *game) {

	SDL_Surface *portrait = SDL_TakeSurface(GR_PORTRAIT, game->class, 0);

	SDL_Surface *items = SDL_TakeSurface(GR_VIEW, 0, 0);
	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *left_frame = local.frames[FRAME_LEFT];
	SDL_Rect *top_frame = local.frames[FRAME_TOP];
	SDL_Rect *bar_frame  = local.frames[FRAME_MIDDLE];
	SDL_Rect *right_frame  = local.frames[FRAME_RIGHT];

	SDL_Rect pos = { left_frame->w, top_frame->h + bar_frame->h + fs->h + sys->zoom, sys->screen->w - left_frame->w - right_frame->w, 0 }; 

	SDL_Rect dest = { pos.x, pos.y, portrait->w, portrait->h }; 

	SDL_BlitSurface(portrait, NULL, sys->screen, &dest );

	SDL_Rect box = { pos.x + portrait->w, pos.y, pos.w - portrait->w, portrait->h };	

	SDL_FillRect(sys->screen, &box, 0x000000);

	SDL_Rect stats = { pos.x + portrait->w + fs->w / 8 , pos.y + fs->h / 4 + fs->h / 8, pos.w - portrait->w, 2 };
	
	SDL_Rect line = { pos.x + portrait->w , pos.y + fs->h / 2, pos.w - portrait->w, fs->h / 8 };

	KB_iloc(stats.x, stats.y);
	KB_ilh(fs->h + sys->zoom);
	KB_iprintf("%s the %s\n", game->name, classes[game->class][game->rank].title);
	KB_iprintf("Leadership         %5d\n", game->leadership);
	line.y = sys->cursor_y * fs->h + sys->base_y + fs->h / 8;
	SDL_FillRect(sys->screen, &line, 0xFFFFFF);

	KB_iloc(stats.x, stats.y + (fs->h + sys->zoom) * 2 + (fs->h/8));
	KB_iprintf("Commission/Week    %5d\n", player_commission(game));
	KB_iprintf("Gold               %5d\n", game->gold);
	line.y = sys->cursor_y * fs->h + sys->base_y;
	SDL_FillRect(sys->screen, &line, 0xFFFFFF);

	KB_iloc(stats.x, stats.y + (fs->h + sys->zoom) * 4 + (fs->h/8));
	KB_iprintf("Spell power        %5d\n", game->spell_power);
	KB_iprintf("Max # of spells    %5d\n", game->max_spells);
	line.y = sys->cursor_y * fs->h + sys->base_y;
	SDL_FillRect(sys->screen, &line, 0xFFFFFF);

	KB_iloc(stats.x, stats.y + (fs->h + sys->zoom) * 6 + (fs->h/8));
	KB_iprintf("Villains caught    %5d\n", player_captured(game));
	KB_iprintf("Artifacts found    %5d\n", player_num_artifacts(game));
	line.y = sys->cursor_y * fs->h + sys->base_y;
	SDL_FillRect(sys->screen, &line, 0xFFFFFF);

	KB_iloc(stats.x, stats.y + (fs->h + sys->zoom) * 8 + (fs->h/8));
	KB_iprintf("Castles garrisoned %5d\n", player_castles(game));
	KB_iprintf("Followers killed   %5d\n", game->followers_killed);
	KB_iprintf("Current score      %5d\n", player_score(game));

	KB_TopBox("        Press 'ESC' to exit");

	/* Draw artifacts (and maps) */
	int i;

#define BELT	4
#define MAP_BELT 2

#define EMPTY_SLOT (MAX_ARTIFACTS + MAX_CONTINENTS)
#define EMPTY_MAP (MAX_ARTIFACTS + MAX_CONTINENTS + 1)

	SDL_Rect inventory = { pos.x, pos.y + portrait->h, pos.w, items->h * 2 };	

	SDL_Rect item = { 0, 0, pos.w / 6, items->h };

	SDL_FillRect(sys->screen, &inventory, 0xFFFF00) ;

	int x = 0, y = 0;
	for (i = 0; i < MAX_ARTIFACTS; i++) {

		SDL_Rect item_src = { item.w * i, 0, item.w, items->h };
		SDL_Rect item_dst = { inventory.x + x * item.w, inventory.y + y * item.h, item.w, items->h };

		if (!game->artifact_found[i]) item_src.x = item.w * EMPTY_SLOT; 

		SDL_BlitSurface( items, &item_src, sys->screen, &item_dst);

		x++;
		if (x >= BELT) {
			y++;
			x = 0;
		}
	}

	x = 0; y = 0;
	inventory.x += ( BELT * item.w );
	for (i = 0; i < MAX_CONTINENTS; i++) {

		SDL_Rect item_src = { item.w * (i+ MAX_ARTIFACTS), 0, item.w, items->h };
		SDL_Rect item_dst = { inventory.x + x * item.w, inventory.y + y * item.h, item.w, items->h };

		if (!game->continent_found[i]) item_src.x = item.w * EMPTY_MAP;

		SDL_BlitSurface( items, &item_src, sys->screen, &item_dst);

		x++;
		if (x >= MAP_BELT) {
			y++;
			x = 0;
		}
	}

	SDL_Flip(sys->screen);

	while (KB_event(&press_any_key) != 0xFF) 
		{	}


}

void view_army(KBgame *game) {

	SDL_Surface *tile;

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *left_frame = local.frames[FRAME_LEFT];
	SDL_Rect *top_frame = local.frames[FRAME_TOP];
	SDL_Rect *bar_frame  = local.frames[FRAME_MIDDLE];
	SDL_Rect *right_frame  = local.frames[FRAME_RIGHT];

	SDL_Rect pos;

	//RECT_Size(&pos, bg);
	pos.x = left_frame->w;
	pos.y = top_frame->h + bar_frame->h + fs->h + sys->zoom;
	
	pos.w = sys->screen->w - left_frame->w - right_frame->w;

	byte troop_morale[5] = { 0 };

	int i, j;
	for (i = 0; i < 5; i++) {
		byte morale = MORALE_HIGH;
		if (game->player_numbers[i] == 0) break;
		byte troop_id = game->player_troops[i];
		byte groupI = troops[ troop_id ].morale_group;
		for (j = 0; j < 5; j++) {
			if (game->player_numbers[j] == 0) break;
			//if (i == j) continue;
			byte ctroop_id = game->player_troops[j];
			byte groupJ = troops[ ctroop_id ].morale_group;
			byte nm = morale_chart[groupI][groupJ];
			if (nm < morale) morale = nm;
		}
		troop_morale[i] = morale;
	}
	tile = SDL_TakeSurface(GR_TILE, 0, 0);

	int done = 0;
	int redraw = 1;
	int frame = 0;
	while (!done) {
		int key = KB_event(&press_any_key_interactive);	
		if (key == 0xFF) done = key;

		if (key == 2) {
			frame++;
			if (frame > 3) frame = 0;
			redraw = 1;
		}

		if (redraw) {
			for (i = 0; i < 5; i++) {
				SDL_Rect dest = { pos.x, pos.y + i * tile->h, tile->w, tile->h };
				SDL_Rect frm =  { frame * tile->w, 0, tile->w, tile->h };
				SDL_BlitSurface(tile, NULL, sys->screen, &dest);
				
				SDL_Rect tbox =  { pos.x + tile->w, pos.y + i * tile->h, pos.w - tile->w, tile->h };
				SDL_Rect tline =  { pos.x + tile->w, pos.y + (i+1) * tile->h - 2, pos.w - tile->w  - fs->w/2, 2 };
				
				SDL_FillRect(sys->screen, &tbox, 0);
				if (i < 4)
				SDL_FillRect(sys->screen, &tline, 0xFFFFFF);		

				if (game->player_numbers[i] == 0) continue;
				
				byte troop_id = game->player_troops[i];
				SDL_Surface *troop = SDL_TakeSurface(GR_TROOP, troop_id, 1);

				SDL_BlitSurface(troop, &frm, sys->screen, &dest);

				KB_iloc(tbox.x, tbox.y + fs->h / 2);
				KB_ilh(fs->h + 4);
				KB_iprintf(" %-3d %s\n", game->player_numbers[i], troops[ troop_id ].name);
				KB_iprintf(" SL:%2d MV:%2d\n", troops[ troop_id ].skill_level, troops[ troop_id ].move_rate);

				if (army_leadership(game, troop_id) <= 0)
					KB_iprint(" Out of control!");
				else
					KB_iprintf(" Morale:%s\n", morale_names[ troop_morale[i] ]);

				KB_iloc(tbox.x + fs->w * 16, tbox.y + fs->h / 2);
				KB_ilh(fs->h + 4);
				KB_iprintf("HitPts:%d\n", troops[ troop_id ].hit_points * game->player_numbers[i]);
				KB_iprintf("Damage:%d-%d\n", troops[ troop_id ].melee_min * game->player_numbers[i], troops[ troop_id ].melee_max * game->player_numbers[i]);
				KB_iprintf("G-Cost:%d\n", troops[ troop_id ].recruit_cost / 10 * game->player_numbers[i]);
			}

			KB_TopBox("        Press 'ESC' to exit");

			SDL_Flip(sys->screen);
			redraw = 0;
		}
	}

}


void dismiss_army(KBgame *game) {

	int i, max = 0;

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *text = KB_BottomBox("Dismiss which army", "", 0);

	KB_iloc(text->x, text->y + fs->h + fs->h / 2);
	KB_ilh(fs->h + fs->h / 8);

	for (i = 0; i < 5; i++) {
		if (game->player_troops[i] == 0xFF || game->player_numbers[i] == 0) break;

		KB_imenu(&five_choices, i, 16);
		KB_iprintf("  %c) %3d %s\n", 'A'+i, game->player_numbers[i], troops[ game->player_troops[i] ].name); 
	}
	max = i + 1;

	const char *twirl = "\x1D" "\x05" "\x1F" "\x1C" ; /* stands for: | / - \ */  
	byte twirl_pos = 0;

	int done = 0;
	int redraw = 1;
	while (!done) {

		int key = KB_event(&five_choices);

		if (redraw) {
			KB_iloc(text->x + fs->w * 19, text->y - fs->h / 4);
			KB_iprintf("%c", twirl[twirl_pos]);

			SDL_Flip(sys->screen);
			redraw = 0;
		}

		if (key && key <= max) {
			if (max == 1) {
				printf("If you Dismiss your last\narmy, you will be sent back to the King in disgrace.\n");
				printf("Dismiss last army (y/n)?/");
			}
		
			dismiss_troop(game, key - 1);
			done = 1;
		}

		if (key == 6) {
			twirl_pos++;
			if (twirl_pos > 3) twirl_pos = 0;
			redraw = 1;
		}		

		if (key == 0xFF) done = 1;
	}

}

KBgamestate throne_room_or_barracks = {
	{
		{	{ 0, 0, 0, 0 }, SDLK_a, 0, 0      	},
		{	{ 0, 0, 0, 0 }, SDLK_b, 0, 0      	},

		{	{ SOFT_WAIT }, SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};

void audience_with_king(KBgame *game) {
	char message[128];

	int captured = 0;
	int needed = 0;

	int i;

	for (i = 0; i < MAX_VILLAINS; i++)
		if (game->villain_caught[i]) captured++;

	needed = classes[game->class][game->rank + 1].villains_needed - captured;

	KB_BottomBox(NULL, 
		"Trumpets announce your\n"
		"arrival with regal fanfare.\n\n"
		"King Maximus rises from his\n"
		"throne to greet you and\n"
		"proclaims:           (space)", 1);

	sprintf(message, "\n\n"
		"My dear %s,\n\n"
		"I can aid you better after\n"
		"you've captured %d more\n"
		"villains.",
	game->name, needed);

	KB_BottomBox(message, "", 1);
}

void recruit_soldiers(KBgame *game) {

	int home_troops[5];
	int off = 0;

	int i;
	for (i = 0; i < MAX_TROOPS; i++) {
		if (troops[i].dwells == DWELLING_CASTLE)
			home_troops[off++] = i;
	}

	int max = 9999;

	const char *twirl = "\x1D" "\x05" "\x1F" "\x1C" ; /* stands for: | / - \ */
	byte twirl_pos = 0;

	byte whom = 0;

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *text = KB_BottomFrame();

	/* Status bar */
	KB_iloc(0, 0);
	KB_iprint("Press 'ESC' to exit");

	int done = 0;
	int redraw = 1;
	while (!done) {

		if (redraw) {
			text = KB_BottomFrame();
		
			/** Left side **/
			/* Header (few pixels up) */
			KB_iloc(text->x, text->y - fs->h/4);
			KB_iprint("Recruit Soldiers\n");

			/* Message */
			KB_iloc(text->x, text->y + fs->h/4);
			KB_ilh(fs->h + fs->h/8);
			KB_iprint("\n");
			for (i = 0; i < 5; i++) {
				KB_imenu(&five_choices, i, 13);
				KB_iprintf("%c) %-11s", 'A' + i, troops[home_troops[i]].name);
				KB_iprintf("%d\n", troops[home_troops[i]].recruit_cost);
			}	

			/** Right side **/
			/* Header (few pixels up) */
			KB_iloc(text->x + fs->w * 20, text->y - fs->h/4);
			KB_iprintf("GP=%dK\n", game->gold / 1000);

			/* Message */
			KB_iloc(text->x + fs->w * 20, text->y + fs->h/4);
			KB_ilh(fs->h + fs->h/8);
			KB_iprint("\n");
			KB_iprint("\n");
			KB_iprint("(A-C) ");
			if (whom == 0) {
				KB_iprintf("%c\n", twirl[twirl_pos]);
				KB_iprint("        \n        \n        ");
			} else {
				KB_iprintf("%c\n", 'A' + whom - 1);
				KB_iprintf("Max=%d\n", max);
				KB_iprint("How Many\n");
				KB_iprint("        ");
			}

			SDL_Flip(sys->screen);
			redraw = 0;
		}

		if (!whom) {

			int key = KB_event(&five_choices);

			if (key == 0xFF) { done = 1; }

			if (key && key < 6) {

				whom = key;
				redraw = 1;

				/* Calculate "MAX YOU CAN HANDLE" number based on leadership (and troop hp?) */
				max = army_leadership(game, home_troops[whom-1]) / troops [ home_troops[whom-1] ].hit_points;
			}
			if (key == 6) {
				twirl_pos++;
				if (twirl_pos > 3) twirl_pos = 0;
				redraw = 1;
			}

		} else {
			char *enter = text_input(6, 1, text->x + fs->w * 20, text->y + fs->h/4 + (fs->h + fs->h/8) * 5);

			if (enter == NULL) { whom = 0; }
			else {
				int number = atoi(enter);
				if (number > max) number = max;

				/* BUY troop */
				buy_troop(game, home_troops[whom-1], number);

				/* Calculate new "MAX YOU CAN HANDLE" number based on leadership (and troop hp?) */
				max = army_leadership(game, home_troops[whom-1]) / troops [ home_troops[whom-1] ].hit_points;
			}

			redraw = 1;
		}

	}
}

void visit_home_castle(KBgame *game) {

	int id = MAX_CASTLES;
	//printf("Visiting castle %d at %d , %d , %d x %d <%p>\n", id, game->x, game->y, bg->w, bg->h, bg);

	int home_troops[5];
	int off = 0;

	int i;
	for (i = 0; i < MAX_TROOPS; i++) {
		if (troops[i].dwells == DWELLING_CASTLE)
			home_troops[off++] = i;
	}

	SDL_Rect *fs = &sys->font_size;

	/* Status bar */
	KB_iloc(0, 0);
	KB_iprint("Press 'ESC' to exit");

	int random_troop = home_troops[ rand() % 5 ];

	int done = 0;
	int frame = 0;
	int redraw = 1;
	int redraw_menu = 1;
	while (!done) {

		int key = KB_event(&throne_room_or_barracks);

		if (key == 0xFF) done = 1;
		if (key == 1) {
			recruit_soldiers(game);
			redraw_menu = 1;
		}
		if (key == 2) {
			audience_with_king(game);
			redraw_menu = 1;
		}
		if (key == 3) { frame++; redraw = 1; }
		if (frame > 3) frame = 0;

		if (redraw || redraw_menu) {

			if (redraw_menu) {
				SDL_Rect *text = KB_BottomFrame();

				/* Header (few pixels up) */	
				KB_iloc(text->x, text->y - fs->h/4);
				KB_iprintf("Castle %s\n", castle_names[id]);

				/* Menu */
				KB_iloc(text->x, text->y );
				KB_ilh(fs->h + fs->h/8);
				KB_iprint("\n\n");
				KB_imenu(&throne_room_or_barracks, 0, 25);
				KB_iprint("A) Recruit Soldiers      \n");
				KB_imenu(&throne_room_or_barracks, 1, 25);
				KB_iprint("B) Audience with the King\n");
			}

			if (redraw) {
				/* Background */
				draw_location(0, random_troop, frame);
			}

			SDL_Flip(sys->screen);
			redraw = 0;
			redraw_menu = 0;
		}
	}
}

void visit_own_castle(KBgame *game, int castle_id) {

}

int lay_siege(KBgame *game, int castle_id) {

	int id = castle_id;

	//printf("Visiting castle %d at %d , %d , %d x %d <%p>\n", id, game->x, game->y, bg->w, bg->h, bg);

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *text = KB_BottomFrame();	

	/* Header (few pixels up) */	
	KB_iloc(text->x, text->y - fs->h/4 - fs->h/8);
	//KB_iprint(header);
	KB_iprintf("Castle %s\n", castle_names[id]);

	/* Message */
	KB_iloc(text->x, text->y + fs->h/4);
	KB_iprint("\n\n");
	if (game->castle_owner[id] == 0x7F) {
		KB_iprint("Various groups of monsters\noccupy this castle.");
	} else {
		char *name = KB_Resolve(STR_VNAME, game->castle_owner[id] & 0x1F);
		KB_iprint(name);
		KB_iprint(" and\narmy occupy this castle.");
		KB_iprint("\n\n\n");
	}
		KB_iloc(text->x, text->y + fs->h * 6);
	KB_iprint("            Lay Siege (y/n)?");	
	
	//KB_BottomBox("Castle %s", "A) Recruit Soldiers\nB) Audience with the King\nC) \nD)\nE)",0);

	SDL_Flip(sys->screen);

	int done = 0;
	while (!done) {
		int key = KB_event(&yes_no_question);	
		if (key) done = key;
	}

	return done - 1;
}


void visit_castle(KBgame *game) {

	enum {
		not_found,
		home,
		your,
		enemy,
	} ctype = not_found;

	int id = 0;
	int i, j;

	if (HOME_CONTINENT == game->continent
	&&	HOME_X == game->x && HOME_Y == game->y) {
		id = MAX_CASTLES;
		ctype = home;
	}
	else
	for (i = 0; i < MAX_CASTLES; i++) {
		if (castle_coords[i][0] == game->continent
		&&	castle_coords[i][1] == game->x
		&&	castle_coords[i][2] == game->y) {
			id = i;
			ctype = (game->castle_owner[id] == 0xFF ? your : enemy);
		}
	}

	if (ctype == not_found) {
		KB_errlog("Can't find castle at continent %d - X=%d Y=%d\n",game->continent,game->x,game->y);
		return; 
	}
	if (ctype == home) {
		visit_home_castle(game);
	}
	if (ctype == your) {
		visit_own_castle(game, id);
	}
	if (ctype == enemy) {
		lay_siege(game, id);
	}

	if (game->castle_owner[id] == 0xFF) {
		printf("Castle owned by you\n");
	} else if (game->castle_owner[id] == 0x7F) {
		printf("Castle owned by monsters\n");	
	} else {
		char *sign = KB_Resolve(STR_VNAME, game->castle_owner[id] & 0x1F);
		printf("Got: %s\n", sign);
		printf("Castle owned by [%08x]\n", game->castle_owner[id] );
	}


}

void gather_information(KBgame *game, int id) {

	char buf[1024];

	SDL_Rect *fs = &sys->font_size;
	SDL_Rect *text = KB_BottomFrame();	

	/* Header (few pixels up) */	
	KB_iloc(text->x, text->y - fs->h/4 - fs->h / 8);
	KB_ilh(fs->h + fs->h/8);
	KB_iprintf("Castle %s is under\n", castle_names[id]);
	if (game->castle_owner[id] == 0x7F) {
		KB_iprint("no one's rule.\n");
	} else {
		char *name = KB_Resolve(STR_VNAME, game->castle_owner[id] & 0x1F);
		KB_iprintf("%s's rule.\n", name);
	}

	/* Army */
	KB_iloc(text->x, text->y + fs->h*2 - fs->h / 8);
	int i;
	for (i = 0; i < 5; i++) {
		if (game->castle_numbers[id][i] == 0) break;
		KB_iprintf("  %s %s\n", number_name(game->castle_numbers[id][i]), troops[ game->castle_troops[id][i] ].name);
	}

	SDL_Flip(sys->screen);
}

void visit_town(KBgame *game) {

	int id = 0;
	int i;

	for (i = 0; i < MAX_TOWNS; i++) {
		if (town_coords[i][0] == game->continent
		&&	town_coords[i][1] == game->x
		&&	town_coords[i][2] == game->y) {
			id = i;
		}
	}


	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *text = KB_BottomFrame();

	int random_troop = rand() % MAX_TROOPS;

	KB_TopBox("        Press 'ESC' to exit");

	int done = 0;
	int frame = 0;
	int redraw = 1;
	int redraw_menu = 1;
	int msg_hold = 0;
	while (!done) {

		int key = KB_event( msg_hold ? &press_any_key_interactive : &five_choices);

		if (redraw_menu) {
			/** Draw menu **/
			text = KB_BottomFrame();

			/* Header (few pixels up) */	
			KB_iloc(text->x, text->y - fs->h/4 - fs->h/8);
			//KB_iprint(header);
			KB_iprintf("Town of %s\n", town_names[id]);
			KB_iprintf("                    GP=%dK\n", game->gold / 1000);
		
			/* Message */
			//KB_iloc(text->x, text->y + fs->h/4);
			//KB_iprint("\n\n");	
			KB_imenu(&five_choices, 0, 30);
			KB_iprint("A) Get New Contract\n");
			KB_imenu(&five_choices, 1, 30);
			if (game->boat == 0xFF)
				KB_iprintf("B) Rent boat (%d week) \n", 500);
			else
				KB_iprint("B) Cancel boat rental\n");
			KB_imenu(&five_choices, 2, 30);
			KB_iprint("C) Gather information\n");
			KB_imenu(&five_choices, 3, 30);
			KB_iprintf("D) %s spell (%d)\n", spell_names[ game->town_spell[id] ], spell_costs[ game->town_spell[id] ]);
			KB_imenu(&five_choices, 4, 30);
			KB_iprint("E) Buy seige weapons (3000)\n");
		}

		if (redraw) {

			/** Draw pretty picture (Background + Animated Troop) **/
			draw_location(1, random_troop, frame);
		
			//KB_BottomBox("Castle %s", "A) Recruit Soldiers\nB) Audience with the King\nC) \nD)\nE)",0);
		}

		if (redraw || redraw_menu) {
			SDL_Flip(sys->screen);
			redraw = 0;
			redraw_menu = 0;		
		}
		
		if (msg_hold) {
			
			if (key == 2) key = 6;
			else if (key) {
				msg_hold = 0;
				redraw_menu = 1;
				redraw = 1;
			}
		
		} else {
		
			if (key == 0xFF) done = 1;
			
			/** Get new contract **/
			if (key == 1) {
				printf("Choice: %d\n", key);
				for (i = 0 ; i < 5; i++) {
					printf("Contract cycle: %d = [ %02x] %d\n", i, game->contract_cycle[i], game->contract_cycle[i]);
				}
				printf("Last contract: %d\nMax contract: %d\n", game->last_contract, game->max_contract);

				game->last_contract++;
				if (game->last_contract > 4)
					game->last_contract = 0;

				game->contract = game->last_contract;

				/* show contract on screen */
				view_contract(game);
			}
			/** Rent/cancel Boat **/
			if (key == 2) {
				if (game->boat == 0xFF) {
					if (game->gold <= 500) {
						KB_BottomBox("\n\n\nYou don't have enough gold!", "", 0);
						msg_hold = 1;
					} else {
						int i, j;

						redraw_menu = 1;

						game->gold -= 500;

						game->boat = game->continent;
						game->boat_x = game->x - 1;
						game->boat_y = game->y;
	
						if (!IS_WATER(game->map[game->boat][game->boat_y][game->boat_x]))
							game->boat_y--;
						if (!IS_WATER(game->map[game->boat][game->boat_y][game->boat_x]))
							game->boat_y+=2;
						if (!IS_WATER(game->map[game->boat][game->boat_y][game->boat_x]))
							game->boat_x+=2;							
					}
				} else {
					if (game->mount == KBMOUNT_SAIL) 
					{
						KB_BottomBox(NULL, "\n\nPlease vacate the boat first", 0);
						msg_hold = 1;
					} else {				
						game->boat = 0xFF;
						redraw_menu = 1;
					}
				}
			}
			/** Gather information **/
			if (key == 3) {
				gather_information(game, id);
				msg_hold = 1;
			}
			/** Buy spell **/
			if (key == 4) {
				int known = known_spells(game);
				if (known >= game->max_spells) {
					KB_BottomBox(NULL, "\n\n   You have learned your\n  maximum number of spells.", 0);
					msg_hold = 1;
				} else 	if (game->gold <= spell_costs[ game->town_spell[id] ]) {
					KB_BottomBox("\n\n\nYou don't have enough gold!", "", 0);
					msg_hold = 1;
				} else {
					char buf[128];
					int left;

					game->spells[ game->town_spell[id] ]++;
					game->gold -= spell_costs[ game->town_spell[id] ];

					left = (game->max_spells - known - 1);
					sprintf(buf, "\n\n\nYou can learn %d more spell%s.", left, (left == 1 ? "" : "s")); 
					KB_BottomBox(buf, "", 0);
					msg_hold = 1;
				}
				redraw = 1;	
			}
			/** Buy siege weapons **/
			if (key == 5) {
				if (game->siege_weapons) {
					KB_BottomBox("\n\n\n   You have siege weapons!", "", 0);
					msg_hold = 1;
				} else if (game->gold <= 3000) {
					KB_BottomBox("\n\n\nYou don't have enough gold!", "", 0);
					msg_hold = 1;
				} else {				
					game->siege_weapons = 1;
					game->gold -= 3000;
				}
				redraw = 1;
			}
		}

		if (key == 6) { frame++; redraw = 1; }
		if (frame > 3) frame = 0;
	}

}

void visit_alcove(KBgame *game) {

}

int visit_telecave(KBgame *game, int force) {
	int i;
	/* See if it's a teleporting cave */
	for (i = 0; i < MAX_TELECAVES; i++) {
		/* If it's one end of the teleport */
		if (game->teleport_coords[game->continent][i][0] == game->x
		&&	game->teleport_coords[game->continent][i][1] == game->y) {
			/* Go to another end */
			if (force) {
				game->x = game->teleport_coords[game->continent][1 - i][0];
				game->y = game->teleport_coords[game->continent][1 - i][1];
			}
			return 0;
		}
	}
	return 1;
}

void visit_dwelling(KBgame *game, byte rtype) {

	int id = -1;
	int i;

	/* See if it's archmage's alcove */
	if (
		ALCOVE_CONTINENT == game->continent
		&& ALCOVE_X == game->x
		&& ALCOVE_Y == game->y
	) {
		visit_alcove(game);
		return;
	}

	/* Find which dwelling is it */
	for (i = 0; i < MAX_DWELLINGS; i++) {
		if (game->dwelling_coords[game->continent][i][0] == game->x
		&&	game->dwelling_coords[game->continent][i][1] == game->y)
			id = i;
	}

	/* Somehow, there's no dwelling here */
	if (id == -1) return;

	int max = 9999;

	const char *twirl = "\x1D" "\x05" "\x1F" "\x1C" ; /* stands for: | / - \ */
	byte twirl_pos = 0;

	byte troop_id = game->dwelling_troop[game->continent][id];

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *text = KB_BottomFrame();

	/* Status bar */
	KB_iloc(0, 0);
	KB_iprint("Press 'ESC' to exit");

	/* Calculate "MAX YOU CAN HANDLE" number based on leadership (and troop hp?) */
	max = army_leadership(game, troop_id) / troops [ troop_id ].hit_points;

	int done = 0;
	int redraw = 1;
	while (!done) {

		char *enter;

		if (redraw) {
			/** Background **/
			draw_location(2 + rtype, troop_id, 0);

			/** Menu **/
			text = KB_BottomFrame();

			/* Header (few pixels up) */
			KB_iloc(text->x, text->y - fs->h/4);
			KB_iprintf("           %s\n", dwelling_names[rtype]);
			KB_iprint("           ");
			for (i = strlen(dwelling_names[rtype]); i > 0 ; i--) 
				KB_iprint("-");

			/* Message */
			KB_iloc(text->x, text->y - fs->h/4);
			KB_iprint("\n\n\n");
			KB_iprintf("%d %s are available\n", game->dwelling_population[game->continent][id], troops[ troop_id ].name);
			KB_iprintf("Cost=% 3d each.      GP=%dK\n", troops[ troop_id ].recruit_cost, game->gold / 1000);
			KB_iprintf("You may recruit up to %d\n", max);
			KB_iprint ("Recruit how many        ");

			SDL_Flip(sys->screen);
			redraw = 0;
		}

		enter = text_input(6, 1, text->x + fs->w * 17, text->y - fs->h/4 + (fs->h) * 6);

		if (enter == NULL) { done = 1; }
		else {
			int number = atoi(enter);
			if (number > max) continue;
			if (number > game->dwelling_population[game->continent][id]) continue;

			/* BUY troop */
			if (!buy_troop(game, troop_id, number))
				/* Reduce dwelling population */
				game->dwelling_population[game->continent][id] -= number;

			/* Calculate new "MAX YOU CAN HANDLE" number based on leadership (and troop hp?) */
			max = army_leadership(game, troop_id) / troops [ troop_id ].hit_points;
		}

		redraw = 1;
	}
}

void read_signpost(KBgame *game) {

	int id = 0;
	int ok = 0;
	int i, j;
	for (j = 0; j < 64; j++) {
		for (i = 0; i < 64; i++) {
			if (game->map[0][j][i] == 0x90) {
				if (i == game->x && j == game->y) { ok = 1; break; }
				id ++;
			}
		}
		if (ok) break; 
	}

	char *sign = KB_Resolve(STR_SIGN, id);

	KB_stdlog("Read sign post [%d] at %d, %d { %s }\n", id, game->x, game->y, sign);

	KB_BottomBox("A sign reads:", sign, 1);

	SDL_Flip(sys->screen);
	KB_Pause();
}

void take_chest(KBgame *game) {
	SDL_Flip(sys->screen);
	KB_Pause();
	game->map[0][game->y][game->x] = 0;
}

void take_artifact(KBgame *game, byte id) {

}

int attack_foe(KBgame *game) {
	int id = 0;
	int i;
	for (i = 0; i < MAX_FOLLOWERS; i++) {
		if (game->follower_coords[game->continent][i][0] == game->x
		 && game->follower_coords[game->continent][i][1] == game->y) {
			id = i;
			break;
		 }
	}

	if (id < FRIENDLY_FOLLOWERS) {

	} else {

	}

	SDL_Rect *rect = KB_BottomBox("Your scouts have sighted:", "", 0);
	SDL_Rect *fs = &sys->font_size;

	KB_iloc(rect->x, rect->y + fs->h * 2 - fs->h / 8);
	KB_ilh(fs->h + fs->h/8);
	for (i = 0; i < 3; i++) {
		byte troop_id = game->follower_troops[game->continent][id][i];
		word troop_count = game->follower_numbers[game->continent][id][i];

		KB_iprintf("  %s %s\n", number_name(troop_count), troops[troop_id].name);
	}
	KB_iloc(rect->x, rect->y + fs->h * 6 - fs->h / 4);
	KB_iprint("               Attack (y/n)?\n");

	SDL_Flip(sys->screen);

	int key = 0;
	while (!key) key = KB_event(&yes_no_question);

	/* "Yes" */
	if (key == 1) {
		//run_combat(game, 0, id);
	}

	return key - 1;
}

void no_spell_banner() {

	SDL_Rect *fs = &sys->font_size;
	SDL_Rect *text = KB_BottomFrame();

	/* Adjust position */
	KB_iloc(text->x, text->y + fs->h - fs->h/2);
	KB_iprintf(
		"You have not been trained in\n"
		"the art of spellcasting yet.\n"
		"Visit the Archmage Aurange\n"
		"in %s at %2d,%2d for\n"
		"this ability.", continent_names[ALCOVE_CONTINENT], ALCOVE_X, ALCOVE_Y);

	KB_flip(sys);
	KB_Pause();
}

KBgamestate seven_choices = {
	{
		{	{ 0 }, SDLK_a, 0, 0      	},
		{	{ 0 }, SDLK_b, 0, 0      	},
		{	{ 0 }, SDLK_c, 0, 0      	},
		{	{ 0 }, SDLK_d, 0, 0      	},
		{	{ 0 }, SDLK_e, 0, 0      	},
		{	{ 0 }, SDLK_f, 0, 0      	},
		{	{ 0 }, SDLK_g, 0, 0      	},

		{	{ 60 }, SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};

KBgamestate cross_choice = {
	{
		{	{ 0 }, SDLK_LEFT, 0, 0      	},
		{	{ 0 }, SDLK_UP, 0, 0      	},
		{	{ 0 }, SDLK_DOWN, 0, 0      	},
		{	{ 0 }, SDLK_RIGHT, 0, 0      	},

		0,
	},
	0
};


int build_bridge(KBgame *game) {

	KB_TopBox("Build bridge in which direction <>ud");

	KB_flip(sys);

	int key = KB_event(&cross_choice);
	
	if (key == 0xFF) return 1;

	KB_TopBox("Not a suitable location for a bridge");
	KB_flip(sys);
	KB_Pause();

	KB_TopBox("What a waste of a good spell!");
	KB_flip(sys);
	KB_Pause();
	
	return 1;
}

int instant_army(KBgame *game) {

	KB_BottomBox("A few Sprites", "have joined to your army.", 1);

	return 0;
}

/* Mode 1 for adventure spells, mode 0 for combat spells */
int choose_spell(KBgame *game, int mode) {

	byte spell_id;

	SDL_Rect border;

	SDL_Surface *screen = sys->screen;

	SDL_Rect *fs = &sys->font_size;

	if (!game->knows_magic)
	{
		no_spell_banner();
		return;	
	}

	RECT_Text((&border), 15, 36);
	RECT_Center(&border, sys->screen);
	
	border.y -= fs->h;

	Uint32 *colors = KB_Resolve(COL_TEXT, 0);

	SDL_TextRect(sys->screen, &border, colors[0], colors[1]);

	KB_TopBox("        Press 'ESC' to exit");

	KB_iloc(border.x + fs->w, border.y + fs->h/2);
	KB_iprint("              Spells\n\n");
	KB_iprint("     Combat         Adventuring  \n");

	KB_iloc(border.x + fs->w, border.y + fs->h*5 - fs->h/2);

	int i, j;
	int half = MAX_SPELLS / 2;
	KB_ilh(fs->h + fs->h / 8);
	for (i = 0; i < half; i++) {
		j = i + half;
		if (!mode)
			KB_imenu(&seven_choices, i, 12);
		KB_iprintf("%2d %-12s", game->spells[j], spell_names[i]);
		KB_iprintf(" %c ", 'A' + i);
		if (mode)
			KB_imenu(&seven_choices, i, 12);
		KB_iprintf("%-13s %2d\n", spell_names[j], game->spells[j]);
	}

	KB_iloc(border.x + fs->w, border.y + fs->h * 13 + fs->h/4);
	KB_iprintf("Cast which %s spell (A-%c)?", (mode ? "Adventure" : "Combat"), 'A' + half);

	const char *twirl = "\x1D" "\x05" "\x1F" "\x1C" ; /* stands for: | / - \ */  
	byte twirl_pos = 0;

	int done = 0;
	int redraw = 1;
	while (!done) {

		int key = KB_event(&seven_choices);

		if (key == 0xFF) done = 1;

		if (key == 8) {
			twirl_pos++;
			if (twirl_pos > 3) twirl_pos = 0;
			redraw = 1;
		}

		else if (key && key < 8) {

			spell_id = key - 1 + (half * mode);

			if (game->spells[spell_id] == 0) {

				KB_iloc(border.x + fs->w * 34, border.y + fs->h * 13 + fs->h/4);
				KB_iprintf("%c", 'A' + key - 1);

				KB_TopBox("     You don't know that spell!");
				KB_flip(sys);
				KB_Pause();

			} else {

				switch (spell_id) {
					case 0:
						//clone
					break;
					case 1:
						//teleport
					break;
					case 2:
						//fireball
					break;		
					case 3:
						//lightning
					break;
					case 4:
						//freeze
					break;
					case 5:
						//resurrect
					break;
					case 6:
						//turn undead
					break;
					case 7:	build_bridge(game);	break;
					case 8:
						//time_stop(game)
					break;
					case 9:
						//find vilain
					break;
					case 10:
						//castle gate
					break;
					case 11:
						//town gate
					break;
					case 12: instant_army(game);	break;
					case 13: raise_control(game);	break;
				}

				/* Spend 1 spell */
				game->spells[spell_id]--;
			}

			done = 1;
		}

		if (redraw) {
			redraw = 0;

			KB_iloc(border.x + fs->w * 34, border.y + fs->h * 13 + fs->h/4);
			KB_iprintf("%c", twirl[twirl_pos]);

			KB_flip(sys);
		}
	}

	return spell_id;
}

void win_game(KBgame *game) {

	KB_TopBox("Press 'ESC' to exit");

	KB_Pause();

}

void lose_game(KBgame *game) {

	KB_TopBox("Press 'ESC' to exit");

	KB_Pause();

}

/* Returns 0 if the game was won, 1 if search was futile, 2 if search was cancelled */
int ask_search(KBgame *game) {

	int days = 10;

	SDL_Rect *rect = KB_BottomBox("Search...", "", 0);
	SDL_Rect *fs = &sys->font_size;

	KB_iloc(rect->x, rect->y + fs->h * 2 );
	KB_ilh(fs->h + fs->h/8);
	KB_iprintf("It will take %d days to do a\nsearch of this area.", days);

	KB_iloc(rect->x, rect->y + fs->h * 6 - fs->h / 4);
	KB_iprint("               Search (y/n)?\n");

	KB_flip(sys);

	int key = 0;
	while (!key) key = KB_event(&yes_no_question);

	/* "No" */
	if (key == 2) return 2;

	/* "Yes" */
	if (game->scepter_continent == game->continent
	 && game->scepter_y == game->y
	 && game->scepter_x == game->x) {

		win_game(game);
		return 0;

	} else {
		KB_BottomBox(NULL, "\n\nYour search of this area has\nrevealed nothing.", 1);
		spend_days(game, days);
	}

	return 1;
}

#define ARROW_KEYS 18
#define ACTION_KEYS 14

#define SYN_EVENT (ACTION_KEYS + ARROW_KEYS + 1)

#define KBACT_VIEW_ARMY 	0
#define KBACT_VIEW_CONTROLS	1 
#define KBACT_FLY       	2
#define KBACT_LAND      	3
#define KBACT_VIEW_CONTRACT	4
#define KBACT_VIEW_MAP  	5
#define KBACT_VIEW_PUZZLE  	6
#define KBACT_SEARCH    	7
#define KBACT_USE_MAGIC 	8
#define KBACT_VIEW_CHAR 	9
#define KBACT_END_WEEK  	10
#define KBACT_SAVE_QUIT 	11
#define KBACT_FAST_QUIT 	12
#define KBACT_DISMISS_ARMY 	13

KBgamestate adventure_state = {
	{
		{	{ SOFT_WAIT }, SDLK_HOME, 0, KFLAG_SOFTKEY     	},
		{	{ SOFT_WAIT }, SDLK_7, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_UP, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_8, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_PAGEUP, 0, KFLAG_SOFTKEY   	},
		{	{ SOFT_WAIT }, SDLK_9, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_LEFT, 0, KFLAG_SOFTKEY     	},
		{	{ SOFT_WAIT }, SDLK_4, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_SPACE, 0, KFLAG_SOFTKEY    	},
		{	{ SOFT_WAIT }, SDLK_5, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_RIGHT, 0, KFLAG_SOFTKEY    	},
		{	{ SOFT_WAIT }, SDLK_6, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_END, 0, KFLAG_SOFTKEY    	},
		{	{ SOFT_WAIT }, SDLK_1, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_DOWN, 0, KFLAG_SOFTKEY     	},
		{	{ SOFT_WAIT }, SDLK_2, 0, KFLAG_SOFTKEY      	},		
		{	{ SOFT_WAIT }, SDLK_PAGEDOWN, 0, KFLAG_SOFTKEY 	},
		{	{ SOFT_WAIT }, SDLK_3, 0, KFLAG_SOFTKEY      	},

		{	{ 0 }, SDLK_a, 0, 0      	},
		{	{ 0 }, SDLK_c, 0, 0      	},
		{	{ 0 }, SDLK_f, 0, 0      	},
		{	{ 0 }, SDLK_l, 0, 0      	},		
		{	{ 0 }, SDLK_i, 0, 0      	},
		{	{ 0 }, SDLK_m, 0, 0      	},
		{	{ 0 }, SDLK_p, 0, 0      	},
		{	{ 0 }, SDLK_s, 0, 0      	},
		{	{ 0 }, SDLK_u, 0, 0      	},
		{	{ 0 }, SDLK_v, 0, 0      	},
		{	{ 0 }, SDLK_w, 0, 0      	},
		{	{ 0 }, SDLK_q, 0, 0      	},

		{	{ 0 }, SDLK_q, KMOD_CTRL, 0	},
		{	{ 0 }, SDLK_d, 0, 0      	},

		{	{ SOFT_WAIT }, SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};

void draw_sidebar(KBgame *game, int tick) {
	SDL_Surface *screen = sys->screen;

	SDL_Rect *right_frame  = local.frames[FRAME_RIGHT];	

	SDL_Surface *purse = SDL_TakeSurface(GR_PURSE, 0, 0);
	SDL_Surface *sidebar = SDL_TakeSurface(GR_UI, 0, 0);

	SDL_Surface *coins = SDL_TakeSurface(GR_COINS, 0, 0);
	SDL_Surface *piece = SDL_TakeSurface(GR_PIECE, 0, 0);

	SDL_Rect *map = &local.map;

	/** Draw siderbar UI **/
	SDL_Rect hsrc = { 0, 0, purse->w, purse->h };
	SDL_Rect hdst = { screen->w - right_frame->w - purse->w, map->y, hsrc.w, hsrc.h };

	/* Contract */
	hsrc.x = 8 * hsrc.w;
	SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);

	/* Siege weapons */
	hdst.y += hsrc.h;
	hsrc.x = (game->siege_weapons ? tick * hsrc.w : 9 * hsrc.w);
	SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);
	
	/* Magic star */
	hdst.y += hsrc.h;
	hsrc.x = (game->knows_magic ? (tick + 4) * hsrc.w : 10 * hsrc.w);
	SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);

	/* Puzzle map */
	hdst.y += hsrc.h;
	hsrc.x = 11 * hsrc.w;
	SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);

	int i, j; /* puzzle pieces */
	for (j = 0; j < 5; j++)
	for (i = 0; i < 5; i++)
	{
		/* Refrence table with negative artifact and positive villain indexes */
		int id = puzzle_map[j][i]; 
		if ((id < 0 && game->artifact_found[-id - 1])
		|| (id >= 0 && game->villain_caught[id])) continue;

		/* Draw puzzle piece */
		SDL_Rect srect = { 0, 0, piece->w, piece->h };
		SDL_Rect mrect = { 
			hdst.x + i * piece->w + (2 * sys->zoom),
			hdst.y + j * piece->h + (2 * sys->zoom),
			piece->w,
			piece->h };
		SDL_BlitSurface(piece, &srect, screen, &mrect);
	}

	/* Gold purse */
	hdst.y += purse->h;
	hsrc.x = 12 * hsrc.w;
	SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);
	
	int cval[3]; /* gold coins */
	cval[0] = game->gold / 10000;
	cval[1] = (game->gold - cval[0] * 10000) / 1000;
	cval[2] = (game->gold - cval[0] * 10000 - cval[1] * 1000) / 100;
	for (j = 0; j < 3; j++)
	{
		SDL_Rect sr = { (coins->w / 3) * j, 0, coins->w / 3, coins->h };
		SDL_Rect dr = { hdst.x + sr.w * j, hdst.y + hdst.h - sr.h, sr.w, sr.h };
		for (i = 0; i < cval[j]; i++) {
			dr.y -= (2 * sys->zoom);
			SDL_BlitSurface(coins, &sr, screen, &dr);
		}
	}
}

void draw_map(KBgame *game, int tick) {

	SDL_Rect pos;
	SDL_Rect src;

	int i, j;

	SDL_Surface *screen = sys->screen;
	
	SDL_Surface *tileset = SDL_TakeSurface(GR_TILESET, 0, 0);
	SDL_Surface *hero = SDL_TakeSurface(GR_HERO, 0, 1);

	SDL_Rect *tile = local.map_tile;

	word perim_h = local.map.w / tile->w;
	word perim_w = local.map.h / tile->h;
	word radii_w = (perim_w - 1) / 2;
	word radii_h = (perim_h - 1) / 2;

	int border_y = game->y - radii_h;
	int border_x = game->x - radii_w;

	src.w = tile->w;
	src.h = tile->h;

	/** Draw map **/
	pos.w = tile->w;
	pos.h = tile->h;

	SDL_FillRect( screen , &local.map, 0xFF0000);

	for (j = 0; j < perim_h; j++)
	for (i = 0; i < perim_w; i++) {
		byte m;
		
		if (border_x + i > 63 || border_y + j > 63 || 
			border_x + i < 0 || border_y + j < 0) m = 32;
		else
			m = game->map[0][border_y + j][border_x + i];

		m &= 0x7F;

		int th = m / 8;
		int tw = m - (th * 8);

		src.x = tw * src.w;
		src.y = th * src.h;
		pos.x = i * (pos.w) + local.map.x;
		pos.y = (perim_h - 1 - j) * (pos.h) + local.map.y;

		SDL_BlitSurface( tileset, &src , screen, &pos );
	}

	/** Draw boat **/
	if (game->mount != KBMOUNT_SAIL && game->boat == game->continent)
	{
		int boat_lx = game->boat_x - game->x + radii_w;
		int boat_ly = game->y - game->boat_y + (perim_w - 1 - radii_h);

		if (boat_lx >= 0 && boat_ly >= 0 && boat_lx < perim_w && boat_ly < perim_h)
		{
			SDL_Rect hsrc = { 0, 0, src.w, src.h };
			SDL_Rect hdst = { local.map.x + boat_lx * src.w, local.map.y + boat_ly * src.h, src.w, src.h };

			hsrc.x += src.w * (0);
			hsrc.y += src.h * (local.boat_flip);

			SDL_BlitSurface( hero, &hsrc , screen, &hdst );
		}
	}

}

void draw_player(KBgame *game, int frame) {

	SDL_Rect *tile = local.map_tile;

	SDL_Surface *hero = SDL_TakeSurface(GR_HERO, 0, 0);

	word perim_h = local.map.w / tile->w;
	word perim_w = local.map.h / tile->h;
	word radii_w = (perim_w - 1) / 2;
	word radii_h = (perim_h - 1) / 2;

	int border_y = game->y - radii_h;
	int border_x = game->x - radii_w;

	/** Draw player **/
	{
		SDL_Rect hsrc = { 0, 0, tile->w, tile->h };
		SDL_Rect hdst = { 
			local.map.x + radii_w * tile->w, 
			local.map.y + (perim_h - 1 - radii_h) * tile->h, 
			tile->w, 
			tile->h 
		};
		
		hsrc.x += tile->w * (game->mount + frame);
		hsrc.y += tile->h * (local.hero_flip);

		SDL_BlitSurface( hero, &hsrc , sys->screen, &hdst );
	}

}

static signed char move_offset_x[9] = { -1, 0, 1, -1, 0, 1, -1,  0,  1 };
static signed char move_offset_y[9] = {  1, 1, 1,  0, 0, 0, -1, -1, -1 };

/* Main game loop (adventure screen) */
void display_overworld(KBgame *game) {

	SDL_Surface *screen = sys->screen;

	Uint32 *colors = KB_Resolve(COL_TEXT, 0);

	SDL_Rect status_rect = { local.status.x, local.status.y, local.status.w, local.status.h };

	SDL_BlitSurface(local.border[FRAME_MIDDLE], NULL, screen, local.frames[FRAME_MIDDLE] );

	//int tileset_pitch = local.tileset->w / tile->w;

	int key = 0;
	int done = 0;
	int redraw = 1;

	int frame  = 0;

	local.hero_flip = 0;
	local.boat_flip = 0;

	int max_frame = 3;
	int tick = 0;

	int walk = 0;

#define KEY_ACT(ACT) (ARROW_KEYS + 1 + KBACT_ ## ACT)

	while (!done) {

		key = KB_event(&adventure_state);

		if (key == 0xFF) done = 1;

		if (key == KEY_ACT(VIEW_ARMY)) {
			view_army(game);
		}

		if (key == KEY_ACT(VIEW_CONTROLS)) {
			//view_controls(game);
		}

		if (key == KEY_ACT(FLY)) {
			game->mount = KBMOUNT_FLY;
			redraw = 1;
		}

		if (key == KEY_ACT(LAND) && game->mount == KBMOUNT_FLY) {
			if (game->map[game->continent][game->y][game->x] == 0) {
				game->mount = KBMOUNT_RIDE;
				redraw = 1;
			}
		}

		if (key == KEY_ACT(VIEW_CONTRACT)) {
			view_contract(game);
		}

		if (key == KEY_ACT(VIEW_MAP)) {
			view_minimap(game);
		}

		if (key == KEY_ACT(VIEW_PUZZLE)) {
			view_puzzle(game);
		}

		if (key == KEY_ACT(SEARCH)) {
			if (!ask_search(game)) done = 1;
		}

		if (key == KEY_ACT(USE_MAGIC)) {
			choose_spell(game, 1);
		}

		if (key == KEY_ACT(VIEW_CHAR)) {
			view_character(game);
		}

		if (key == KEY_ACT(END_WEEK)) {
			spend_week(game);
		}

		if (key == KEY_ACT(SAVE_QUIT)) {

		}
		if (key == KEY_ACT(FAST_QUIT)) {

		}

		if (key == KEY_ACT(DISMISS_ARMY)) {
			dismiss_army(game);
			test_defeat(game);
			redraw = 1;
		}

		if (key == SYN_EVENT) {
			if (++tick > 3) tick = 0;
			if (max_frame == 3) {
				max_frame = 2;
			} else {
				frame++;
				if (frame > max_frame) {
					frame = 0;
					max_frame = 2;
				}
				redraw = 1;
			}
		}

		if (key > 0 && key < ARROW_KEYS + 1 && !walk) {

			int ox = move_offset_x[(key-1)/2];
			int oy = move_offset_y[(key-1)/2];

			int cursor_x = game->x + ox;
			int cursor_y = game->y + oy;

			if (cursor_x < 0) cursor_x = 0;
			if (cursor_x > 63) cursor_x = 63;
			if (cursor_y < 0) cursor_y = 0;
			if (cursor_y > 63) cursor_y = 63;

			byte m = game->map[game->continent][cursor_y][cursor_x];		

			walk = 1;

			if (ox == 1) local.hero_flip = 0;
			if (ox == -1) local.hero_flip = 1;
			if (game->mount == KBMOUNT_SAIL)
				local.boat_flip = local.hero_flip;

			if (!IS_GRASS(m) && game->mount != KBMOUNT_FLY) {

				if (game->boat_x == cursor_x
				&& game->boat_y == cursor_y
				&& game->boat == game->continent) {
					/* Boarding a boat */
					game->mount = KBMOUNT_SAIL;
				}
				else
				if (IS_WATER(m)) {
					if (game->mount != KBMOUNT_SAIL) walk = 0;
				}
				else
				if (IS_DESERT(m)) {
					game->steps_left = 0;
				}
				else
				if (!IS_INTERACTIVE(m))	{
					printf("Stopping at tile: %02x -- %02x (%d x %d)\n", m, m & 0x80, cursor_x, cursor_y);
					walk = 0;
				}

			}

			if (walk) {
				redraw = 1;

				game->last_x = game->x;
				game->last_y = game->y;

				game->x = cursor_x;
				game->y = cursor_y;

				if (m == 0x8e && !visit_telecave(game, 1)) {
					printf("HUH %d %d\n", cursor_x, cursor_y);
					m = game->map[game->continent][cursor_y][cursor_x];
					walk = 0;
				}

				frame = 3;
				max_frame = 3;

				/* When sailing, tuck the boat with us */
				if (game->mount == KBMOUNT_SAIL) {
					game->boat_x = game->x;
					game->boat_y = game->y;
				}
			} 
		}

		if (redraw) {

			draw_map(game, tick);

		}

		if (walk) {
			walk = 1;
			byte m = game->map[game->continent][game->y][game->x];
			if (IS_INTERACTIVE(m) && game->mount != KBMOUNT_FLY) {
				switch (m) {
					case 0x85:	visit_castle(game);	walk = 0; break;
					case 0x8a:	visit_town(game); walk = 0; break;
					case 0x8b:	take_chest(game);	break;
					case 0x8e:	if (!visit_telecave(game, 0)) break;
					case 0x8c:
					case 0x8d:
					case 0x8f:	visit_dwelling(game, m - 0x8c); walk = 0; break;
					case 0x90:	read_signpost(game);		break;
					case 0x91:	walk = !attack_foe(game);	break;
					case 0x92:
					case 0x93:	take_artifact(game, m - 0x92);	break;
					default:
						KB_errlog("Unknown interactive tile %02x at location %d, %d\n", m, game->x, game->y);
					break;
				}
			}
			if (!walk) {
				game->x = game->last_x;
				game->y = game->last_y;
				walk = 1;
			} else {
				walk = 0;
				game->steps_left -= 1;
				/* Hitting shore */
				if (!IS_WATER(m)) {
					/* Leave ship */
					if (game->mount == KBMOUNT_SAIL) {
						game->mount = KBMOUNT_RIDE;
						game->boat_x = game->last_x;
						game->boat_y = game->last_y;
					}
				}	
			}
			continue;
		}

		if (game->steps_left <= 0) {
			end_day(game);
		}
		if (game->days_left == 0) {
			lose_game(game);
			done = 1;
		}

		if (redraw) {

			draw_player(game, frame);

			draw_sidebar(game, tick);

			/* Status bar */
			SDL_FillRect(screen, &status_rect, colors[0]);
			KB_iloc(status_rect.x, status_rect.y + 1);
			KB_printf(sys, " Options / Controls / Days Left:%d ", game->days_left);

	    	SDL_Flip( screen );
			redraw = 0;
		}
	}
}

int run_game(KBconfig *conf) {

	int mod;

	/* Start new environment (game window) */
	sys = KB_startENV(conf);

	/* Must be successfull to continue */
	if (!sys) return -1;

	/* Module auto-discovery */
	if (conf->autodiscover)
		discover_modules(conf->data_dir, conf);

	/* User-configured modules */
	register_modules(conf);

	/* --- ! ! ! --- */
	mod = select_module();

	/* No module! (Unlikely...) */
	if (mod == -1) {
		KB_errlog("No module selected.\n");
		KB_stopENV(sys);
		return -1;
	}

	/* Initialize module(s) */
	init_modules(conf);

	/* Load and use module font */
	KB_setfont(sys, SDL_LoadRESOURCE(GR_FONT, 0, 0)); 

	/* Preload all resources */
	prepare_resources();

	/* --- X X X --- */
	display_logo();

	display_title();

	/* Select a game to play (new/load) */
	KBgame *game = select_game(conf);

	//display_debug();//debug(game);

	/* See if a game was selected */
	if (!game) KB_stdlog("No game selected.\n");
	else {

		/* Just for fun, output game name */
		KB_stdlog("%s the %s (%d days left)\n", game->name, classes[game->class][game->rank].title, game->days_left);

		/* PLAY THE GAME */
		display_overworld(game);
	}

	/* Free resources */
	free_resources();

	/* Kill modules */
	stop_modules(conf);

	/* Stop environment */
	KB_stopENV(sys);

	return 0;
}

void debug(KBgame *game) {

	int i;
	for (i = 0; i < MAX_DWELLINGS; i++) {
	/*
		printf("Dwelling: %d\n", i);
		//printf("\tCoords: ?, ?\n");
		//printf("\tType: %s\n", dwelling_names[i]);
		int troop_id = game->dwelling_troop[i];
		printf("\tTroop: %s, %d x %d\n", troops[troop_id].name, troop_id, 			game->dwelling_population[i]);
	//byte dwelling_troop[MAX_DWELLINGS];	 
	//byte dwelling_population[MAX_DWELLINGS];
	*/
	}

	int y,x;
	int f = 0, k, z;
	
	/*
	for (z = 0; z < 4; z++) {
	for (y = 0; y < LEVEL_H; y++) {
		for (x = 0; x < LEVEL_H; x++) {
			if (game->map[z][y][x] == 0x91) {
				printf("Found follower at {%d} %d, %d -- %d\n", z, x, y, f);
				f++;
			}	
		}
	}}
	sleep(2);
	*/
	int cont =0;
	int cont_flip;
	cont=-1;
	for (i = 0; i < 4; i++) {
		int x = game->orb_coords[i][0];
		int y = game->orb_coords[i][1];
		cont = i;
		printf("Orb Chest %04d - CONT=%d X=%2d Y=%2d\t\t", i, cont, x, y);
		printf("TILE: [%02X] [%02x] [%02x] [%02x]",
		 	game->map[0][y][x],
		 	game->map[1][y][x],
		 	game->map[2][y][x],
		 	game->map[3][y][x]
		 );
		 printf("\t %02x%02x%02x\n", cont, x, y);
	}
/*
	int cont;
	int cont_flip;
	cont = -1;
	for (i = 0; i < 20; i++) {
		int x = game->follower_coords[i][0];
		int y = game->follower_coords[i][1];
		if (!(i % 5)) cont++;
		printf("Follower %d (friendly) CONT=%d X=%d Y=%d\n", i, cont, x, y);
		if (game->map[cont][y][x] != 0x91) { printf("NOT FOUND\n"); sleep(1); }
	}
	cont = 0; cont_flip = 0;
	for (i = 20; i < MAX_FOLLOWERS; i++) {
		int x = game->follower_coords[i][0];
		int y = game->follower_coords[i][1];
		cont_flip++;
		if (cont_flip > 35) { cont++; cont_flip = 1;}
		if (x == 0 && y == 0) continue; 
		printf("Follower %d (hostile) CONT=%d X=%d Y=%d", i, cont, x, y);
		if (game->map[cont][y][x] != 0x91) { printf(" NOT FOUND!!! [%02x] [%02x] [%02x] [%02x] ", 
				game->map[0][y][x], game->map[1][y][x],
				game->map[2][y][x], game->map[3][y][x]);		
			//sleep(1); 
		}
		printf("\tArmy:\n");
		for (k = 0; k < 3; k++) {
			int troop_id = game->follower_troops[i][k];
			printf("\t\t%s x %d\n", troops[troop_id].name, game->follower_numbers[i][k]);
		} 
		printf("\n");
	}
	*/

}