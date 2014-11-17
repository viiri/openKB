/*
 *  ui.c -- user interface routines
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

#include <SDL.h>
#include "lib/kbstd.h" 
#include "lib/kbres.h"
#include "env.h"
#include "ui.h"

/* Global/main session */
KBsession local = { NULL };

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

KBgamestate yes_no_interactive = {
	{
		{	{ 0 }, SDLK_y, 0, 0      	},
		{	{ 0 }, SDLK_n, 0, 0      	},
		{	_TIME(SHORT_WAIT), SDLK_SYN, 0, KFLAG_TIMER },
		0
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
		{	_TIME(80), SDLK_SYN, 0, KFLAG_TIMER | KFLAG_RETKEY },
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

KBgamestate settings_selection = {
	{
		{	_AREA(0, 0, 1024, 768), 0xFF, 0, KFLAG_ANYKEY },
		{	_NON, SDLK_UP, 0, 0 	},
		{	_NON, SDLK_DOWN, 0, 0 	},
		{	_NON, SDLK_LEFT, 0, 0	},
		{	_NON, SDLK_RIGHT, 0, 0 	},
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

KBgamestate alphabet_letter = {
	{
		{	_AREA(0, 0, 0, 0), SDLK_a, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_b, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_c, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_d, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_e, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_f, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_g, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_h, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_i, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_j, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_k, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_l, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_m, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_n, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_o, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_p, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_q, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_r, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_s, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_t, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_u, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_v, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_w, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_x, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_y, 0, 0		},
		{	_AREA(0, 0, 0, 0), SDLK_z, 0, 0		},
		{	_TIME(SHORT_WAIT), SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};

#undef _NON
#undef _TIME
#undef _AREA

static char kbd_state[512] = { 0 };

int KB_reset(KBgamestate *state) {

	SDL_Event event;
	int i;

	/* Flush all events (Evil) */
	//while (SDL_PollEvent(&event))
		//;

	/* Reset all timers */
	if (state)
	for (i = 0; i < MAX_HOTSPOTS; i++) {
		if (state->spots[i].hot_key == 0) break;
		if (state->spots[i].flag & KFLAG_TIMER) {
			state->spots[i].coords.w = 0;
		}
	}

	/* Unpush all keys */
	for (i = 0; i < 512; i++) {
		kbd_state[i] = 0;
	}
}

void KB_imenu(KBgamestate *state, int id, int cols) {

	word x, y;

	KB_getpos(sys, &x, &y);

	state->spots[id].coords.x = x;
	state->spots[id].coords.y = y;
	state->spots[id].coords.w = sys->font_size.w * cols;
	state->spots[id].coords.h = sys->font_size.h;
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
						if (sp->hot_key != kbd->sym && (
							kbd->sym == SDLK_LSUPER || kbd->sym == SDLK_RSUPER ||
							kbd->sym == SDLK_LSHIFT || kbd->sym == SDLK_RSHIFT ||
							kbd->sym == SDLK_LCTRL || kbd->sym == SDLK_RCTRL ||
							kbd->sym == SDLK_LALT || kbd->sym == SDLK_RALT ))
						{
							continue;
						}
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
								else if (eve == SDLK_2) eve = '@';
								else if (eve == SDLK_6) eve = '^';
								else if (eve == SDLK_7) eve = '&';
								else if (eve == SDLK_8) eve = '*';
								else if (eve == SDLK_9) eve = '(';
								else if (eve == SDLK_0) eve = ')';
								else if (eve >= SDLK_0 && eve <= SDLK_9) eve -= 16;
								else if (eve == '`') eve = '~';
								else if (eve == '-') eve = '_';
								else if (eve == '=') eve = '+';
								else if (eve == '\\') eve = '|';
								else if (eve == ',') eve = '<';
								else if (eve == '.') eve = '>';
								else if (eve == '/') eve = '?';
								else if (eve == ';') eve = ':';
								else if (eve == '\'') eve = '"'; //"
								else if (eve == '[') eve = '{';
								else if (eve == ']') eve = '}';
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

char KB_KeyLabelChar(int key) {
	if (key >= SDLK_0 && key <= SDLK_9) {
		key = '0' + (key - SDLK_0);
	}
	else if (key >= SDLK_a && key <= SDLK_z) {
		key = 'A' + (key - SDLK_a);
	}
	return key;
}

char* KB_KeyLabel(int key1, int key2) {
	static char buf[32];

	char val1[8];
	char val2[8];

	if (key2 != -1) {
		int tmp = key2;
		key2 = key1;
		key1 = tmp;
	}

	sprintf(val1, "  %c", KB_KeyLabelChar(key1));
	sprintf(val2, "%c", KB_KeyLabelChar(key2));

	if (key1 == SDLK_SPACE) sprintf(val1, "  SPC");
	if (key1 == SDLK_HOME) sprintf(val1, "HOME");
	if (key1 == SDLK_END) sprintf(val1, "END ");
	if (key1 == SDLK_PAGEDOWN) sprintf(val1, "PGDN");
	if (key1 == SDLK_PAGEUP) sprintf(val1, "PGUP");
	if (key1 == SDLK_UP) sprintf(val1, "  \x18 ");
	if (key1 == SDLK_DOWN) sprintf(val1, "  \x19 ");
	if (key1 == SDLK_LEFT) sprintf(val1, "  \x1A ");
	if (key1 == SDLK_RIGHT) sprintf(val1, "  \x1B ");

	if (key2 != -1) {
		sprintf(buf, "%s or %s", val1, val2);
	} else {
		sprintf(buf, "%s", val1);
	}

	return &buf[0];
}

void SDL_TextRect(SDL_Surface *dest, SDL_Rect *r, Uint32 fore, Uint32 back, int top) {

	int i, j;

	SDL_Rect *fs = &sys->font_size;

	/* Choose colors, Fill Rect */
	SDL_FillRect(dest, r, back);	
	incolor(fore, back);

	/* Top and bottom (horizontal fill) */
	for (i = r->x ; i + fs->w < r->x + r->w; i += fs->w) {
		if (top) inprint(dest, "\x0E", i, r->y);
		inprint(dest, "\x0F", i, r->y + r->h - fs->h);
	}
	/* Left and right (vertical fill) */
	for (j = r->y + fs->h; j + fs->h < r->y + r->h; j += fs->h) {
		inprint(dest, "\x14", r->x, j);
		inprint(dest, "\x15", i, j);
	}
	/* Corners */
	if (top) inprint(dest, "\x10", r->x, r->y);/* Top-left */
	else inprint(dest, "\x14", r->x, r->y);
	if (top) inprint(dest, "\x11", i, r->y);/* Top-right */
	else inprint(dest, "\x15", i, r->y);
	inprint(dest, "\x12", r->x, r->y + r->h - fs->h);/* Bottom-left */
	inprint(dest, "\x13", i, r->y + r->h - fs->h);/* Bottom-right */
}


/* Wait for ~1.5 seconds or a keypress */
inline void KB_Wait() { 
 	Uint32 delay = 1500;
 	while (!KB_event(&press_any_key) && delay) {
 		delay -= 10;
 		SDL_Delay(10);
 	}
}

/* Wait for a keypress */
inline void KB_Pause() { 
 	while (!KB_event(&press_any_key)) SDL_Delay(10);
}


/* Display a message. Wait for a key to discard. */
SDL_Rect* KB_MessageBox(const char *str, byte flag) {

	SDL_Surface *screen = sys->screen;
	Uint32 bg = local.message_colors[COLOR_BACKGROUND];
	Uint32 fg = local.message_colors[COLOR_TEXT];
	Uint32 ui = local.message_colors[COLOR_FRAME];

	SDL_Rect *fs = &sys->font_size;

	int i, max_w = 28;

	static SDL_Rect frame;	/* Outer rectangle */
	SDL_Rect text;      	/* Inner rectangle */

	if (flag & MSG_HARDCODED) {
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
	text.w = max_w * fs->w;
	text.h = h * fs->h;

	/* To the center of the screen */
	RECT_Center(&text, screen);

	/* A little bit up */
	text.y -= fs->h / 2;

	/* A nice frame */
	frame.x = text.x - fs->w;
	frame.y = text.y - fs->h;
	frame.w = text.w + fs->w * 2;
	frame.h = text.h + fs->h * 2;
	SDL_TextRect(screen, &frame, ui, bg, 1);
	SDL_FillRect(screen, &text, 0xFF0000);

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
			inprint(screen, buffer, text.x, text.y + h * fs->h);
			w = 0;
			buffer[0] = '\0';
			h++;
		}
		i++;
	} while (str[i - 1] != '\0');

	if (flag & MSG_FLUSH) KB_flip(sys);
	if (flag & MSG_PAUSE) KB_Pause();

	return &frame;
}

SDL_Rect* KB_BottomFrame() {
	SDL_Surface *screen = sys->screen;

	Uint32 bg = local.message_colors[COLOR_BACKGROUND];
	Uint32 fg = local.message_colors[COLOR_TEXT];
	Uint32 ui = local.message_colors[COLOR_FRAME];

	SDL_Rect *fs = &sys->font_size;
	
	SDL_Rect *left_frame = local.frames[FRAME_LEFT];
	SDL_Rect *bottom_frame = local.frames[FRAME_BOTTOM];

	SDL_Rect border;
	static SDL_Rect text;

	/* Make a 30 x 8 border (+1 character on each side) */ 
	RECT_Text(&border, 8, 30);
	border.h += fs->h / 2; /* Plus some extra pixels, because 'bottom box' is uneven */
	border.x = left_frame->w;
	border.y = screen->h - bottom_frame->h - border.h;

	/* Actual text is 28 x 6 (meaning 28 cols per 6 rows, btw) */
	RECT_Pos(&text, &border);
	text.y += fs->h;
	text.x += fs->w;
	RECT_Text((&text), 6, 28);

	/* A nice frame */
	SDL_TextRect(screen, &border, ui, bg, 1);
	//SDL_FillRect(screen, &text, 0x00FF00);//debug fill

	KB_icolor(&local.message_colors[0]);

	return &text;
}

SDL_Rect* KB_BottomBox(const char *header, const char *str, byte flag) {

	SDL_Rect *fs = &sys->font_size;

	SDL_Rect *text = KB_BottomFrame();	

	/* Header (few pixels up) */
	KB_iloc(text->x, text->y - fs->h/4 - fs->h/8);
	if (flag & MSG_HARDCODED) KB_ilh(fs->h + 1);
	if (header) KB_iprint(header);

	/* Message */
	KB_iloc(text->x, text->y + fs->h/4);
	if (header) KB_iprint("\n\n");	
	if (flag & MSG_PADDED) KB_iloc(text->x, text->y + fs->h * 5);
	KB_iprint(str);

	/* Update screen and possibly wait */
	if (flag & MSG_FLUSH) KB_flip(sys);
	if (flag & MSG_PAUSE) KB_Pause();
	if (flag & MSG_WAIT)  KB_Wait();

	return text;
}

SDL_Rect* KB_TopBox(byte flag, const char *str, ...) {

	Uint32 *colors;
	byte padding;

	/* Possible varags */
	va_list argptr;
	va_start(argptr, str);
	va_end(argptr); //too early?

	/* Get colors */
	colors = local.status_colors;

	/* Calculate padding if needed */
	padding = 0;
	if (flag & MSG_CENTERED) {
		int len = strlen(str);
		int max = local.status.w / sys->font_size.w;
		if (len < max) padding = (max - len) / 2 ;//+ (max - len) % 2;
	} else if (flag & MSG_PADDED) {
		padding = 1;
	}

	/* Clear line */
	SDL_FillRect(sys->screen, &local.status, colors[COLOR_BACKGROUND]);

	/* Print string [with varargs] */
	KB_icolor(colors);
	KB_iloc(local.status.x, local.status.y + sys->font_size.h / 8);
	KB_icurs(padding, 0);
	KB_ivprintf(str, argptr);

	/* Update screen and possibly wait */
	if (flag & MSG_FLUSH) KB_flip(sys);
	if (flag & MSG_WAIT) KB_Wait();

	return &local.status;
}
