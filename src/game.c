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

#define SDLK_SYN 0x16

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
		{	_NON, SDLK_BACKSPACE, 0, KFLAG_RETKEY },	
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

/*
 * "inline" functions to adjust SDL_Rect around HOST.
 *
 * HOST could be another SDL_Rect or SDL_Surface, or anything
 * that has ->w and ->h members (thus the usage of macros).
 */
#define RECT_Size(RECT, HOST) \
		RECT->w = HOST->w, \
		RECT->h = HOST->h

#define RECT_Center(RECT, HOST) \
		RECT->x = (HOST->w - RECT->w) / 2, \
		RECT->y = (HOST->h - RECT->h) / 2

/* In this one, ROWS and COLS of text are used */
#define RECT_Text(RECT, ROWS, COLS) \
		RECT->w = COLS * 8, \
		RECT->h = ROWS * 8

//
// TODO: get rid of those, call RECT_* directly
inline void SDL_CenterRect(SDL_Rect *rect, SDL_Surface *img, SDL_Surface *host) 
{
	RECT_Size(rect, img);
	RECT_Center(rect, host);
}

inline void SDL_CenterRectTxt(SDL_Rect *rect, int rows, int cols, SDL_Surface *host)
{
	RECT_Text(rect, rows, cols);
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

		if (event.type == SDL_KEYDOWN) {
			SDL_keysym *kbd = &event.key.keysym;
			for (i = 0; i < state->max_spots; i++) {
				KBhotspot *sp = &state->spots[i];
				if ((sp->flag & KFLAG_ANYKEY) || 
					(sp->hot_key == kbd->sym && 
					( !sp->hot_mod || (sp->hot_mod & kbd->mod) )))
					{
						eve = i + 1; /* !!! */
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

	/* Choose colors, Fill Rect */
	SDL_FillRect(dest, r, back);	
	incolor(fore, back);

	/* Top and bottom (horizontal fill) */
	for (i = r->x + 8; i < r->x + r->w - 8; i += 8) {
		inprint(dest, "\x0E", i, r->y);
		inprint(dest, "\x0F", i, r->y + r->h);
	}
	/* Left and right (vertical fill) */
	for (j = r->y + 8; j < r->y + r->h; j += 8) {
		inprint(dest, "\x14", r->x, j);
		inprint(dest, "\x15", i, j);
	}
	/* Corners */
	inprint(dest, "\x10", r->x, r->y);/* Top-left */ 
	inprint(dest, "\x11", i, r->y);/* Top-right */
	inprint(dest, "\x12", r->x, j);/* Bottom-left */
	inprint(dest, "\x13", i, j);/* Bottom-right */
}
/* Enter name */
char *enter_name(int x, int y) {
	static char entered_name[11];

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	SDL_Rect menu;

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
			inprint(screen, buf, x + curs * 8, y);

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
 	while (!KB_event(&press_any_key));
}

/* Display a message. Wait for a key to discard. */
void KB_MessageBox(const char *str, int wait) {
	SDL_Surface *screen = sys->screen;
	
	int i, max_w = 28;

	SDL_Rect rect;

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
	rect.w = max_w * 8;
	rect.h = h * 8;

	/* To the center of the screen */
	rect.x = (screen->w - rect.w) / 2;
	rect.y = (screen->h - rect.h) / 2;

	/* A little bit up */
	rect.y -= 16;

	/* A nice frame */
	rect.x -= 8;rect.y -= 8;rect.w += 16;rect.h += (8*2);
	SDL_TextRect(screen, &rect, 0xFFFFFF, 0x000000);
	rect.x += 8;rect.y += 8;rect.w -= 16;rect.h -= (8*2);
	SDL_FillRect(screen, &rect, 0xFF0000);

	/* True-pass */
	i = 0; h = 0; w = 0;
	char buffer[80];
	do {
		buffer[w++] = str[i];
		if (str[i] == '\n' || str[i] == '\0' || (str[i] == ' ' && w > max_w - 3)) {
			buffer[w] = '\0';
			buffer[w-1] = '\0';
			inprint(screen, buffer, rect.x, rect.y + h * 8);
			w = 0;
			buffer[0] = '\0';
			h++;
		}
		i++;
	} while (str[i - 1] != '\0');
	
	SDL_Flip(screen);
	SDL_Delay(10);
	if (wait) KB_Pause();
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
	int rows = 11;//h

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
	
	inprint(screen, " Knight    Name:", menu.x+8, menu.y+8);
	inprint(screen, classes[pclass][0].title, menu.x+8 + 8*1, menu.y+8);
	if (has_name) inprint(screen, name, menu.x+16 + 16*8, menu.y+8);
	
	inprint(screen, "   Difficulty   Days  Score", menu.x+8, menu.y+8 + 8*2);

	inprint(screen, "   Easy         900    x.5 ", menu.x+8, menu.y+8 + 8*4);	
	inprint(screen, "   Normal       600     x1 ", menu.x+8, menu.y+8 + 8*5);
	inprint(screen, "   Hard         400     x2 ", menu.x+8, menu.y+8 + 8*6);
	inprint(screen, "   Impossible?  200     x4 ", menu.x+8, menu.y+8 + 8*7);
	
		if (has_name) {
	inprint(screen, "\x18\x19 to select   Ent to Accept", menu.x+8, menu.y+8 + 8*9);		

			int i;
			for (i = 0; i < 4; i++) 
				inprint(screen, (sel == i ? ">" : " "), menu.x+16, menu.y+8 + 8*4 + i*8);

		}

	    	SDL_Flip( screen );

			redraw = 0;
		}
		
		if (!has_name) {
			name = enter_name(menu.x + 16 + 16*8, menu.y + 8);
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
	menu.w = l * 8 + 16;
	menu.h = num_files * 8 + 8 * 5;

	/* To the center of the screen */
	menu.x = (screen->w - menu.w) / 2;
	//menu.y = (screen->h - menu.h) / 2;

	/* A little bit up */
	menu.y = 8 + 4 * 8;

	/* Update mouse hot-spots */
	for (i = 0; i < num_files; i++) {
		savegame_selection.spots[i + 3].coords.x = menu.x + 16;
		savegame_selection.spots[i + 3].coords.y = menu.y + 32 + i * 8;
		savegame_selection.spots[i + 3].coords.w = menu.w - 8;
		savegame_selection.spots[i + 3].coords.h = 8;
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
		if (key >= 3) {
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
			inprint(screen, " Select game:", menu.x + 8, menu.y + 8 - 4);
			inprint(screen, "   Overlord   ", menu.x + 8, menu.y + 16);
			inprint(screen, "             ", menu.x + 8, menu.y + 32 - 4);

			for (i = 0; i < num_files; i++) {

				incolor(0xFFFFFF, 0x000000);
				char buf[4];sprintf(buf, "%d.", i + 1);
				inprint(screen, buf, menu.x + 16, 8 * i + menu.y + 32);

				if (i == sel) incolor(0x000000, 0xFFFFFF);
				inprint(screen, filename[i], menu.x + 40, 8 * i + menu.y + 32);
			}

	incolor(0xFFFFFF, 0x000000);
	inprint(screen, " 'ESC' to exit \x18\x19 Return to Select  ", 8 + 8, 8);

	    	SDL_Flip( screen );

			redraw = 0;
		}

	}

	return game;
}

KBgame *select_game(KBconfig *conf) {

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	KBgame *game = NULL;

	while (!done) {

		key = KB_event(&character_selection);

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

		if (key == 0xFF) done = 1;

		if (redraw) {

			SDL_Rect pos;

			SDL_Surface *title = KB_LoadIMG8(GR_SELECT, 0);
			
			if (title == NULL) {
				KB_errlog("Can't find 'SELECT'!\n");
				return NULL;
			}

			SDL_CenterRect(&pos, title, screen);

			SDL_BlitSurface( title, NULL , screen, &pos );

	inprint(screen, "Select Char A-D or L-Load saved game", 8 + 8, 8);

	    	SDL_Flip( screen );

			SDL_FreeSurface(title);

			SDL_Delay(10);

			redraw = 0;
		}

	}
	
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
	menu.h = conf->num_modules * 8 + 8;

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

			SDL_Surface *title = KB_LoadIMG8(GR_LOGO, 0);

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

			SDL_Surface *title = KB_LoadIMG8(GR_TITLE, 0);

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
			SDL_Surface *font = KB_LoadIMG8(GR_FONT, 0);
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
	sys->font = KB_LoadIMG8(GR_FONT, 0);
	infont(sys->font);

	/* --- X X X --- */
	display_logo();

	display_title();

	/* Select a game to play (new/load) */
	KBgame *game = select_game(conf);

	display_debug();//debug(game);

	/* No game! Quit */
	if (!game) {
		KB_stdlog("No game selected.\n");
		KB_stopENV(sys);
		return 0;
	}

	/* Just for fun, output game name */
	KB_stdlog("%s the %s\n", game->name, classes[game->class][game->rank].title);
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