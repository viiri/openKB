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
#include "bounty.h"
#include "lib/kbconf.h"
#include "lib/kbres.h"
#include "lib/kbauto.h"
#include "lib/kbstd.h"

// For the meantime, use SDL directly, it if gets unwieldy, abstract it away 
#include "SDL.h"

#include "../vendor/vendor.h" /* scale2x, inprint, etc */

#include "env.h"

#define MAX_HOTSPOTS	64

/* Global/main environment */
KBenv *sys = NULL;

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

	word	hot_key;
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
#define KB_iprint(ARGS...) KB_print(sys, ## ARGS)
#define KB_iprintf(ARGS...) KB_printf(sys, ## ARGS)

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
		RECT->x = (HOST->w - RECT->w) / 2, \
		RECT->y = (HOST->h - RECT->h) / 2

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
	SDL_Rect *size = KB_fontsize(sys);
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
			kbd_state[kbd->sym] = 0;
		}

		if (event.type == SDL_KEYDOWN) {
			SDL_keysym *kbd = &event.key.keysym;
			kbd_state[kbd->sym] = 1;
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

	SDL_Rect *fs = KB_fontsize(sys);

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
char *enter_name(int x, int y) {
	static char entered_name[11];

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	SDL_Rect menu;
	SDL_Rect *fs = KB_fontsize(sys);

	entered_name[0] = '\0';
	int curs = 0;

	const char *twirl = "\x1D" "\x05" "\x1F" "\x1C" ; /* stands for: | / - \ */  
	byte twirl_pos = 0;

	while (!done) {

		key = KB_event(&enter_string);

		if (key == 0xFF) { curs = 0; done = 1; }

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
			if (key < 128 && isascii(key)) {
				if (curs < 10) {
					entered_name[curs] = (char)key;
					curs++;
					entered_name[curs] = '\0';
					redraw = 1;
				}
			}
		}

		if (redraw) {

			inprint(screen, entered_name, x, y);
			char buf[2]; sprintf(buf, "%c", twirl[twirl_pos]); 
			inprint(screen, buf, x + curs * fs->w, y);

	    	SDL_Flip( screen );

			redraw = 0;
		}
	}

	inprint(screen, " ", x + curs * 8, y);
	entered_name[curs] = '\0';

	return &entered_name;	
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

	SDL_Rect *fs = KB_fontsize(sys);

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
	
	SDL_Rect *left_frame = RECT_LoadRESOURCE(RECT_UI, 1);
	SDL_Rect *bottom_frame = RECT_LoadRESOURCE(RECT_UI, 3);

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

	free(left_frame);
	free(bottom_frame);

	return &text;
}

SDL_Rect* KB_BottomBox(const char *header, const char *str, int wait) {
	SDL_Surface *screen = sys->screen;

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *text = KB_BottomFrame();	

	/* Header (few pixels up) */	
	KB_iloc(text->x, text->y - fs->h/4 - fs->h/8);
	KB_iprint(header);

	/* Message */
	KB_iloc(text->x, text->y + fs->h/4);
	KB_iprint("\n\n");	
	KB_iprint(str);

	SDL_Flip(screen);

	if (wait) KB_Pause();

	return &text;
}


/* Actual KBgame* allocator */
KBgame *spawn_game(char *name, int pclass, int difficulty) {

	KBgame* game;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	KB_strcpy(game->name, name);

	game->class = pclass;
	game->rank = 0;
	game->difficulty = difficulty;

	return game;
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

	SDL_Rect *fs = KB_fontsize(sys);

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
			name = enter_name(menu.x + fs->w * 18, menu.y + fs->h);
			if (name[0] == '\0') done = 1;
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

	SDL_Rect *fs = KB_fontsize(sys);
	
	SDL_Rect *top_frame = RECT_LoadRESOURCE(RECT_UI, 0);
	SDL_Rect *left_frame = RECT_LoadRESOURCE(RECT_UI, 1);

	KB_DIR *d = KB_opendir(conf->save_dir);
	KB_Entry *e;
    while ((e = KB_readdir(d)) != NULL) {
		if (e->d_name[0] == '.') continue;
		
		char base[255];
		char ext[255];
		
		name_split(e->d_name, &base, &ext);

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
	inprint(screen, " 'ESC' to exit \x18\x19 Return to Select  ", left_frame->w, top_frame->h);

	    	SDL_Flip( screen );

			redraw = 0;
		}

	}

	return game;
}

void show_credits() {

	SDL_Surface *userpic = SDL_LoadRESOURCE(GR_SELECT, 2, 0);
	SDL_Rect *fs = KB_fontsize(sys);

	SDL_Rect *max;

	KB_MessageColor(0x0000AA, 0xFFFFFF, 0xFFFF55);

	max = KB_MessageBox(
		"King's Bounty Designed By:    "
		"  Jon Van Caneghem\n"
		"\n"
		"Programmed By:\n"
		"  Mark Caldwell\n"
		"  Andy Caldwell\n"
		"\n"
		"Graphics By:\n"
		"  Kenneth L. Mayfield\n"
		"  Vincent DeQuattro, Jr.\n"
		"\n"
		"      Copyright 1990-95\n"
		"   New World Computing, Inc\n"
		"     All Rights Reserved"
		, 2);

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
	
	SDL_Rect *top_frame = RECT_LoadRESOURCE(RECT_UI, 0);
	SDL_Rect *left_frame = RECT_LoadRESOURCE(RECT_UI, 1);
	Uint32 *colors = KB_Resolve(COL_TEXT, 0);

	SDL_Surface *title = SDL_LoadRESOURCE(GR_SELECT, 0, 0);

	while (!done) {

		key = KB_event(&character_selection);

		if (redraw) {

			SDL_Rect pos;

			SDL_CenterRect(&pos, title, screen);

			SDL_BlitSurface( title, NULL , screen, &pos );

			if (!credits) {
				KB_iloc(left_frame->w, top_frame->h);
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

	free(top_frame);
	free(left_frame);
	SDL_FreeSurface(title);

	if (game) {
		if (done == 1) KB_MessageBox("%s the %s,\n\nA new game is being created. Please wait while I perform godlike actions to make this game playable.",  0);
		if (done == 2) KB_MessageBox("%s the %s,\n\nPlease wait while I prepare a suitable environment for your bountying enjoyment!",  0);
	}

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

		{	{ SOFT_WAIT }, SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};


void draw_location(int loc_id, int troop_id, int frame) {
	SDL_Surface *bg = SDL_LoadRESOURCE(GR_LOCATION, loc_id, 0);
	SDL_Surface *troop = SDL_LoadRESOURCE(GR_TROOP, troop_id, 0);

	SDL_Rect pos;
	SDL_Rect tpos;
	SDL_Rect troop_frame = { 0, 0, troop->w / 4, troop->h };

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *left_frame = RECT_LoadRESOURCE(RECT_UI, 1);
	SDL_Rect *top_frame = RECT_LoadRESOURCE(RECT_UI, 0);
	SDL_Rect *bar_frame  = RECT_LoadRESOURCE(RECT_UI, 4);

	RECT_Size(&pos, bg); 
	pos.x = left_frame->w;
	pos.y = top_frame->h + bar_frame->h + fs->h + 2;

	troop_frame.x = frame * troop_frame.w;

	tpos.x = troop_frame.w * 1;
	RECT_Size(&tpos, &troop_frame);
	RECT_Bottom(&tpos, bg);
	RECT_AddPos((&tpos), (&pos));

	SDL_BlitSurface(bg, NULL, sys->screen, &pos);

	SDL_BlitSurface(troop, &troop_frame, sys->screen, &tpos);

	SDL_FreeSurface (bg);
	free(left_frame);
	free(top_frame);
	free(bar_frame);
}


KBgamestate throne_room_or_barracks = {
	{
		{	{ 0 }, SDLK_a, 0, 0      	},
		{	{ 0 }, SDLK_b, 0, 0      	},

		{	{ SOFT_WAIT }, SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};

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

	SDL_Rect *text = KB_BottomFrame();	

	/* Status bar */
	KB_iloc(0, 0);
	KB_iprint("Press 'ESC' to exit");

	/* Header (few pixels up) */	
	KB_iloc(text->x, text->y - fs->h/4 - fs->h/8);
	//KB_iprint(header);
	KB_iprintf("Castle %s\n", castle_names[id]);

	/* Message */
	KB_iloc(text->x, text->y + fs->h/4);
	KB_iprint("\n\n");	
	//KB_iprint(str);

	//KB_BottomBox("Castle %s", "A) Recruit Soldiers\nB) Audience with the King\nC) \nD)\nE)",0);


	int random_troop = home_troops[ rand() % 5 ];

	int done = 0;
	int frame = 0;
	while (!done) {

		int key = KB_event(&throne_room_or_barracks);	
	
		if (key == 0xFF) done = 1;
		if (key == 1) {
			printf("Audience with king\n");
		}
		if (key == 2) {
			printf("Recruit Soldiers\n");
		}
		if (key == 3) frame++;
		if (frame > 3) frame = 0;

		draw_location(0, random_troop, frame);

		SDL_Flip(sys->screen);
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
		KB_iprint("Various groups of monsters occupy this castle.");
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
	KB_iloc(text->x, text->y - fs->h/4 - fs->h/8);
	if (game->castle_owner[id] == 0x7F) {
		KB_iprintf("Castle %s is \n");
	} else {
		char *name = KB_Resolve(STR_VNAME, game->castle_owner[id] & 0x1F);
		KB_iprintf("Castle %s is under\n%s's rule.\n", castle_names[id], name);
	}

	/* Army */
	KB_iloc(text->x, text->y + fs->h * 2);
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

	/* Status bar */
	KB_iloc(0, 0);
	KB_iprint("Press 'ESC' to exit");

	int done = 0;
	int frame = 0;
	int redraw = 1;
	int redraw_menu = 1;
	int msg_hold = 0;
	while (!done) {

		int key = KB_event( msg_hold ? &press_any_key_interactive : &five_choices);

		if (redraw_menu) {
		
			/** Draw menu **/
			/* Header (few pixels up) */	
			KB_iloc(text->x, text->y - fs->h/4 - fs->h/8);
			//KB_iprint(header);
			KB_iprintf("Town of %s\n", town_names[id]);
			KB_iprintf("                    GP=%dK\n", 8000 / 1000);
		
			/* Message */
			//KB_iloc(text->x, text->y + fs->h/4);
			//KB_iprint("\n\n");	
			KB_iprint("A) Get New Contract\n");
			KB_iprint("B) Cancel boat rental\n");
			KB_iprint("C) Gather information\n");
			KB_iprintf("D) %s spell (%d)\n", spell_names[ game->town_spell[id] ], spell_costs[ game->town_spell[id] ]);
			KB_iprint("E) Buy seige weapons (3000)\n");
			//KB_iprint(str);
			
			redraw_menu = 0;

		}

		if (redraw) {

			/** Draw pretty picture (Background + Animated Troop) **/
			draw_location(1, random_troop, frame);
		
			//KB_BottomBox("Castle %s", "A) Recruit Soldiers\nB) Audience with the King\nC) \nD)\nE)",0);
	
			SDL_Flip(sys->screen);
			
			redraw = 0;		
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
			if (key == 1) {
				printf("Choice: %d\n", key);
			}
			if (key == 3) {
				gather_information(game, id);
				msg_hold = 1;
			}
			
		}

		if (key == 6) { frame++; redraw = 1; }
		if (frame > 3) frame = 0;

	}

}

void visit_dwelling(KBgame *game) {
	SDL_Flip(sys->screen);
	KB_Pause();
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

void take_artifact(KBgame *game) {

}

int attack_follower(KBgame *game) {

	KB_BottomBox("Danger", "Attack? y/n", 0);
	//KB_iloc(0, 0);
	//KB_iprintf("Attack ? y / n\n", game->x, game->y);

	SDL_Flip(sys->screen);

	int key = 0;
	while (!key) key = KB_event(&yes_no_question);
	
	return key - 1;
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

static signed char move_offset_x[9] = { -1, 0, 1, -1, 0, 1, -1,  0,  1 };
static signed char move_offset_y[9] = {  1, 1, 1,  0, 0, 0, -1, -1, -1 };

/* Main game loop (adventure screen) */
void display_overworld(KBgame *game) {

	SDL_Surface *screen = sys->screen;
	int filter = sys->conf->filter;
	//sys->conf->filter = 0;
	SDL_Surface *tile = SDL_LoadRESOURCE(GR_TILE, 0, 0);
	SDL_Surface *ts = SDL_LoadRESOURCE(GR_TILESET, 0, 0);
	SDL_Surface *hero = SDL_LoadRESOURCE(GR_CURSOR, 0, 1);
	sys->conf->filter = filter;

	SDL_Surface *purse = SDL_LoadRESOURCE(GR_PURSE, 0, 0);	/* single sidebar tile */
	SDL_Surface *sidebar = SDL_LoadRESOURCE(GR_UI, 0, 0);

	SDL_Surface *bar = SDL_LoadRESOURCE(GR_SELECT, 1, 0);
	
	SDL_Rect *top_frame  = RECT_LoadRESOURCE(RECT_UI, 0);	
	SDL_Rect *left_frame  = RECT_LoadRESOURCE(RECT_UI, 1);
	SDL_Rect *right_frame  = RECT_LoadRESOURCE(RECT_UI, 2);
	SDL_Rect *bottom_frame  = RECT_LoadRESOURCE(RECT_UI, 3);	
	SDL_Rect *bar_frame  = RECT_LoadRESOURCE(RECT_UI, 4);

	SDL_Rect *fs = KB_fontsize(sys);
	Uint32 *colors = KB_Resolve(COL_TEXT, 0);	

	SDL_Rect status_rect =  { left_frame->w, top_frame->h, screen->w - left_frame->w - right_frame->w, fs->h + 2 };
	SDL_FillRect(screen, &status_rect, colors[0]);
	SDL_BlitSurface(bar, NULL, screen, bar_frame);

	int tileset_pitch = ts->w / tile->w;

	SDL_Rect map = { left_frame->w, top_frame->h + status_rect.h + bar->h, screen->w - purse->w - right_frame->w - left_frame->w, screen->h - top_frame->h - bar->h - status_rect.h - bottom_frame->h};

	word perim_h = map.w / tile->w;
	word perim_w = map.h / tile->h;
	word radii_w = (perim_w - 1) / 2;
	word radii_h = (perim_h - 1) / 2;

	status_rect.y++;

	int key = 0;
	int done = 0;
	int redraw = 1;

	int cursor_x = game->x;
	int cursor_y = game->y;

	int frame  = 0;
	int flip = 0;
	int boat_flip = 0;

	int max_frame = 3;
	int tick = 0;

	int walk = 0;

	SDL_Rect src = { 0 };
	
	src.w = tile->w;
	src.h = tile->h;

	SDL_FreeSurface(tile);

	while (!done) {

		key = KB_event(&adventure_state);

		if (key == 0xFF) done = 1;

		if (key > 0 && key < ARROW_KEYS + 1 && !walk) {

			int ox = move_offset_x[(key-1)/2];
			int oy = move_offset_y[(key-1)/2];

			if (ox == 1) flip = 0;
			if (ox == -1) flip = 1;

			cursor_x += ox;
			cursor_y += oy;
			
			if (cursor_x < 0) cursor_x = 0;
			if (cursor_x > 63) cursor_x = 63;
			if (cursor_y < 0) cursor_y = 0;
			if (cursor_y > 63) cursor_y = 63;
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

		if (cursor_x != game->x || cursor_y != game->y) {

			byte m = game->map[0][cursor_y][cursor_x];		

#define IS_GRASS(M) ((M) < 2 || (M) == 0x80)
#define IS_WATER(M) ((M) >= 0x14 && (M) <= 0x20)
#define IS_DESERT(M) ((M) >= 0x2e && (M) <= 0x3a)
#define IS_INTERACTIVE(M) ((M) & 0x80)

			walk = 1;

			if (game->mount == KBMOUNT_SAIL)
				boat_flip = flip;

			if (!IS_GRASS(m)) {

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
					printf("Stopping at tile: %02x -- %02x\n", m, m & 0x80);
					walk = 0;
				}
			}

			if (walk) {
				redraw = 1;

				/* Hitting shore */
				if (!IS_WATER(m)) {
					/* Leave ship */
					if (game->mount == KBMOUNT_SAIL) {
						game->mount = KBMOUNT_RIDE;
						game->boat_x = game->x;
						game->boat_y = game->y;
					}
				}	

				game->last_x = game->x;
				game->last_y = game->y;

				game->x = cursor_x;
				game->y = cursor_y;

				frame = 3;
				max_frame = 3;

				/* When sailing, tuck the boat with us */
				if (game->mount == KBMOUNT_SAIL) {
					game->boat_x = game->x;
					game->boat_y = game->y;
				}
			} else {
				cursor_x = game->x;
				cursor_y = game->y;
			}
		}

		SDL_Rect inpos = { 0 };
			
		if (redraw) {

			SDL_Rect pos;

			pos.w = src.w;
			pos.h = src.h;

			SDL_FillRect( screen , &map, 0xFF0000);

			int i, j;

			int border_y = game->y - radii_h;
			int border_x = game->x - radii_w;

			/** Draw map **/
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
				pos.x = i * (pos.w) + map.x;
				pos.y = (perim_h - 1 - j) * (pos.h) + map.y;

				SDL_BlitSurface( ts, &src , screen, &pos );

				//SDL_Flip(screen);
				//SDL_Delay(100);
			}

			/** Draw boat **/
			if (game->mount != KBMOUNT_SAIL && game->boat == game->continent)
			{
				int boat_lx = game->boat_x - game->x + radii_w;
				int boat_ly = game->y - game->boat_y + (perim_w - 1 - radii_h);

				if (boat_lx >= 0 && boat_ly >= 0 && boat_lx < perim_w && boat_ly < perim_h)
				{
					SDL_Rect hsrc = { 0, 0, src.w, src.h };
					SDL_Rect hdst = { map.x + boat_lx * src.w, map.y + boat_ly * src.h, src.w, src.h };

					hsrc.x += src.w * (0);
					hsrc.y += src.h * (boat_flip);

					SDL_BlitSurface( hero, &hsrc , screen, &hdst );
				}
			}
		}

		if (walk) {
			walk = 1;
			byte m = game->map[0][cursor_y][cursor_x];
			if (IS_INTERACTIVE(m)) {
				switch (m) {
					case 0x85:	visit_castle(game);	walk = 0; break;
					case 0x8a:	visit_town(game); walk = 0; break;
					case 0x8b:	take_chest(game);	break;
					case 0x8c:
					case 0x8d:
					case 0x8e:
					case 0x8f:	visit_dwelling(m - 0x8c); walk = 0; break;
					case 0x90:	read_signpost(game);		break;
					case 0x91:	walk = !attack_follower(game);	break;
					case 0x92:
					case 0x93:	take_artifact(m - 0x92);	break;
					default:
						KB_errlog("Unknown interactive tile %02x at location %d, %d\n", m, cursor_x, cursor_y);
					break;
				}
			}
			if (!walk) {
				cursor_x = game->last_x;
				cursor_y = game->last_y;
				walk = 1;
			} else {
				walk = 0;
			}
		}

		if (redraw) {

			/** Draw player **/
			{
				SDL_Rect hsrc = { 0, 0, src.w, src.h };
				SDL_Rect hdst = { map.x + radii_w * src.w, map.y + (perim_h - 1 - radii_h) * src.h, src.w, src.h };
				
				hsrc.x += src.w * (game->mount + frame);
				hsrc.y += src.h * (flip);

				SDL_BlitSurface( hero, &hsrc , screen, &hdst );
			//	SDL_Flip(screen);
			//	SDL_Delay(3000);
			}

			/** Draw siderbar UI **/
			{
				SDL_Rect hsrc = { 0, 0, purse->w, purse->h };
				SDL_Rect hdst = { screen->w - right_frame->w - purse->w, map.y, src.w, src.h };

				/* Contract */
				hsrc.x = 8 * hsrc.w;
				SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);

				/* Siege weapons */
				hdst.y += hsrc.h;
				game->siege_weapons = 1;
				hsrc.x = (game->siege_weapons ? tick * hsrc.w : 9 * hsrc.w);
				SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);
				
				/* Magic star */
				hdst.y += hsrc.h;
				game->knows_magic = 1;
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
					SDL_Rect mrect = { hdst.x + i * 9 + 2, hdst.y + j * 6 + 2, 9, 6 } ;
					SDL_FillRect(screen, &mrect, 0x000000);
					mrect.w -= 1; mrect.h -= 1;
					SDL_FillRect(screen, &mrect, 0xAA0000);
				}

				/* Gold purse */
				hdst.y += purse->h;
				hsrc.x = 12 * hsrc.w;
				SDL_BlitSurface( sidebar, &hsrc, screen, &hdst);
			}

			/* Status bar */
			KB_iloc(status_rect.x, status_rect.y);
			KB_printf(sys, " Options / Controls / Days Left:%d ", game->days_left);

	    	SDL_Flip( screen );
			redraw = 0;
		}
	}

	SDL_FreeSurface(purse);
	SDL_FreeSurface(sidebar);
	SDL_FreeSurface(ts);
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

	/* Load and use module font */
	KB_setfont(sys, SDL_LoadRESOURCE(GR_FONT, 0, 0)); 

	/* --- X X X --- */
	display_logo();

	display_title();

	/* Select a game to play (new/load) */
	KBgame *game = select_game(conf);

	//display_debug();//debug(game);

	/* No game! Quit */
	if (!game) {
		KB_stdlog("No game selected.\n");
		KB_stopENV(sys);
		return 0;
	}

	/* PLAY THE GAME */
	display_overworld(game);

	/* Just for fun, output game name */
	KB_stdlog("%s the %s (%d days left)\n", game->name, classes[game->class][game->rank].title, game->days_left);
	sleep(2);

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