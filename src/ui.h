/*
 *  ui.h -- user interface routines
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
#ifndef _OPENKB_UI_H
#define _OPENKB_UI_H

#include <SDL.h>
#include "lib/kbstd.h" 
 
#define MAX_HOTSPOTS	64

/* Hotspot flags */
#define KFLAG_ANYKEY	0x01
#define KFLAG_RETKEY	0x02

#define KFLAG_TIMER 	0x10
#define KFLAG_TIMEKEY	0x20

#define KFLAG_TRAPSIGNAL 0x08

#define KFLAG_SOFTKEY 	(KFLAG_TIMER | KFLAG_TIMEKEY)

/* Fake keycode for "Sync" event */
#define SDLK_SYN 0x16

/* Delays */
#define SOFT_WAIT 150
#define SHORT_WAIT 50

/*
 * Session is a local in-memory cache
 */
typedef struct KBsession {

	SDL_Rect *frames[6];

	SDL_Surface *border[6];

	SDL_Rect map;
	SDL_Rect status;
	SDL_Rect side;

	SDL_Rect *map_tile;
	SDL_Rect *side_tile;

	int boat_flip;
	int hero_flip;

	/* Each "Uint32*" is a pointer to an array of 256 Uint32 colors 
	 * that follow enum COLOR_ convention */
	Uint32* message_colors;
	Uint32* status_colors;
	Uint32* topmenu_colors;
} KBsession;
extern KBsession local;

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
		(RECT)->x = (HOST)->x, \
		(RECT)->y = (HOST)->y

#define RECT_AddPos(RECT, HOST) \
		(RECT)->x += (HOST)->x, \
		(RECT)->y += (HOST)->y

#define RECT_Center(RECT, HOST) \
		(RECT)->x = ((HOST)->w - (RECT)->w) / 2, \
		(RECT)->y = ((HOST)->h - (RECT)->h) / 2

#define RECT_Right(RECT, HOST) \
		(RECT)->x = ((HOST)->w - (RECT)->w)

#define RECT_Bottom(RECT, HOST) \
		(RECT)->y = ((HOST)->h - (RECT)->h)


/* In this one, ROWS and COLS of text are used */
#define RECT_Text(RECT, ROWS, COLS) \
		(RECT)->w = (COLS) * sys->font_size.w, \
		(RECT)->h = (ROWS) * sys->font_size.h


/* MessageBox, BottomBox and TopBox "flag"s */
#define MSG_CENTERED	0x01
#define MSG_RIGHT   	0x02
#define MSG_PADDED    	0x04
#define MSG_XXX2     	0x08
#define MSG_HARDCODED	0x10
#define MSG_XXX4    	0x20
#define MSG_WAIT    	0x40
#define MSG_PAUSE   	0x80

#define MSG_FLUSH	(MSG_WAIT | MSG_PAUSE)

/* Gamestates */
extern KBgamestate debug_menu;
extern KBgamestate press_any_key;
extern KBgamestate press_any_key_interactive;
extern KBgamestate yes_no_interactive;
extern KBgamestate difficulty_selection;
extern KBgamestate enter_string;
extern KBgamestate character_selection;
extern KBgamestate module_selection;
extern KBgamestate savegame_selection;
extern KBgamestate settings_selection;
extern KBgamestate alphabet_letter;

/* Gamestate handling */
extern void KB_imenu(KBgamestate *state, int id, int cols);
extern int KB_reset(KBgamestate *state);
extern int KB_event(KBgamestate *state);
/* Utilities */
extern char* KB_KeyLabel(int key1, int key2);
extern void SDL_TextRect(SDL_Surface *dest, SDL_Rect *r, Uint32 fore, Uint32 back, int top);
inline void KB_Wait();
inline void KB_Pause();
/* Text output */
extern SDL_Rect* KB_MessageBox(const char *str, byte flag);
extern SDL_Rect* KB_BottomFrame();
extern SDL_Rect* KB_BottomBox(const char *header, const char *str, byte flag);
extern SDL_Rect* KB_TopBox(byte flag, const char *str, ...);

#endif
