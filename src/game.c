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

typedef struct KBhotspot {

	SDL_Rect coords;
	word	hot_key;
	char	hot_mod;

} KBhotspot;

typedef struct KBgamestate {

	KBhotspot spots[MAX_HOTSPOTS];
	int hover;

} KBgamestate;

#define KMOD_ANYKEY	0x01
#define KMOD_RETKEY	0x02
#define KMOD_XXX_1	0x04
#define KMOD_XXX_2	0x08


KBgamestate press_any_key = {
	{
		{	{ 0, 0, 1024, 768  }, 0xFF, KMOD_ANYKEY },
		0,
	},
	0
};

KBgamestate difficulty_selection = {
	{
		{	{ 0, 0, 0, 0  }, SDLK_UP, KMOD_RETKEY },
		{	{ 0, 0, 0, 0  }, SDLK_DOWN, KMOD_RETKEY },
		{	{ 0, 0, 0, 0  }, SDLK_RETURN, KMOD_RETKEY },
		0,
	},
	0
};

KBgamestate enter_string = {
	{
		{	{ 0, 0, 0, 0  }, SDLK_BACKSPACE, KMOD_RETKEY },	
		{	{ 0, 0, 0, 0  }, 0xFF, KMOD_ANYKEY | KMOD_RETKEY },
		0,
	},
	0
};


KBgamestate character_selection = {
	{
		{	{ 0, 0, 0, 0 }, SDLK_a, 0      	},
		{	{ 0, 0, 0, 0 }, SDLK_b, 0      	},
		{	{ 0, 0, 0, 0 }, SDLK_c, 0      	},
		{	{ 0, 0, 0, 0 }, SDLK_d, 0      	},
		{	{ 180, 8, 140, 8 }, SDLK_l, 0      	},		
		0,
	},
	0
};

KBgamestate module_selection = {
	{
		{	{ 0, 0, 0, 0 }, SDLK_UP, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_DOWN, 0	},
		{	{ 0, 0, 0, 0 }, SDLK_RETURN, 0	},
		{	{ 0, 0, 0, 0 }, SDLK_1, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_2, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_3, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_4, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_5, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_6, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_7, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_8, 0		},		
		{	{ 0, 0, 0, 0 }, SDLK_9, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_0, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_MINUS, 0	},
		0,
	},
	0
};

KBgamestate savegame_selection = {
	{
		{	{ 0, 0, 0, 0 }, SDLK_UP, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_DOWN, 0	},
		{	{ 0, 0, 0, 0 }, SDLK_RETURN, 0	},
		{	{ 0, 0, 0, 0 }, SDLK_1, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_2, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_3, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_4, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_5, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_6, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_7, 0		},
		{	{ 0, 0, 0, 0 }, SDLK_8, 0		},		
		{	{ 0, 0, 0, 0 }, SDLK_9, 0		},
		0,
	},
	0
};

void render_game(KBenv *env, KBgame *game, KBgamestate *state, KBconfig *conf) {

}

inline void SDL_CenterRect(SDL_Rect *rect, SDL_Surface *img, SDL_Surface *host) 
{
	/* Size */
	rect->w = img->w;
	rect->h = img->h;

	/* To the center of the screen */
	rect->x = (host->w - rect->w) / 2;
	rect->y = (host->h - rect->h) / 2; 
}

inline void SDL_CenterRectTxt(SDL_Rect *rect, int rows, int cols, SDL_Surface *host)
{
	/* Size */
	rect->w = cols * 8;
	rect->h = rows * 8;

	/* To the center of the screen */
	rect->x = (host->w - rect->w) / 2;
	rect->y = (host->h - rect->h) / 2; 
}



KBenv *KB_startENV(KBconfig *conf) {

	KBenv *nsys = malloc(sizeof(KBenv));

	if (!nsys) return NULL; 

    SDL_Init( SDL_INIT_VIDEO );

    nsys->screen = SDL_SetVideoMode( 320, 200, 32, SDL_SWSURFACE );

    nsys->conf = conf;

	nsys->font = NULL;

	KB_RegisterConfig(conf);

	prepare_inline_font();	// <-- inline font

	return nsys;
}

void KB_stopENV(KBenv *env) {

	if (env->font) SDL_FreeSurface(env->font);

	kill_inline_font();

	free(env);

	SDL_Quit();
}

int KB_event(KBgamestate *state) {
	SDL_Event event;

	int eve = 0;
	int new_hover = -1;
	
	int click = -1;
	int mouse_x = -1;
	int mouse_y = -1;

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
			int i, j = 0;
			for (i = 0; i < MAX_HOTSPOTS; i++) {
				if (state->spots[i].hot_key == 0) break;
				if (state->spots[i].hot_mod & KMOD_ANYKEY 
				 || (state->spots[i].hot_key == event.key.keysym.sym
				 	&& (1 == 1))	/* Modifier */
				 ) 
				{
					eve = i + 1;
					if (state->spots[i].hot_mod & KMOD_RETKEY)
					{
						eve = event.key.keysym.sym;
						if (event.key.keysym.mod & KMOD_SHIFT && eve < 128) { //shift -- uppercase!
							if ((char)eve >= SDLK_a && (char)eve <= SDLK_z) {
								eve = (char)eve - 32;
							}
						}
					}
					j = 1;									
					break;
				}
			}
			if (j) break;
		}
		if (event.type == SDL_MOUSEBUTTONUP) {
			if (event.button.button == 1) {
			mouse_x = event.button.x;
			mouse_y = event.button.y;
			click = 1;
			break;
			}
		}
	}

	/* Mouse moved */
	if (mouse_x != -1 || mouse_y != -1) {
		int i;
		SDL_Rect *r;
		for (i = 0; i < MAX_HOTSPOTS; i++) {
			if (state->spots[i].hot_key == 0) break;

			r = &state->spots[i].coords;

			if (mouse_x >= r->x && mouse_x <= (r->x+r->w)
			 && mouse_y >= r->y && mouse_y <= (r->y+r->h)) 
			{
				new_hover = i;					
			}
		}
	}

	/* Over a hotspot */
	if (new_hover != -1) {
		state->hover = new_hover;
		if (click != -1) {
			eve = new_hover + 1;
		}
	}

	//if (click != -1) eve = 1;

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
		if (key) {
			if (key < 128 && isascii((char)key)) {
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
			inprint(screen, "/", x + curs * 8, y);

	    	SDL_Flip( screen );

			redraw = 0;
		}
	}

	inprint(screen, " ", x + curs * 8, y);
	entered_name[curs] = '\0';

	return &entered_name;	
}

/* Actual KBgame* allocator */
KBgame *spawn_game(char *name, int pclass, int difficulty) {

	KBgame* game;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	strcpy(game->name, name);

	game->class = pclass;
	game->rank = 0;
	game->difficulty = difficulty;

	return game;
}

/* create game screen (pick name and difficulty) */
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
			if (game) done = 1;
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

void display_debug() {

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	int troop_frame = 0;
	SDL_Rect src = {0, 0, 48, 32};
	
//	RESOURCE_DefaultConfig(sys->conf);

	while (!done) {

		key = KB_event(&press_any_key);

		if (key) done = 1;

		if (redraw) {

			SDL_Rect pos;

			SDL_Surface *title = KB_LoadIMG8(GR_TROOP, 0);
			SDL_Surface *font = KB_LoadIMG8(GR_FONT, 0);
			SDL_Surface *peasant = KB_LoadIMG8(GR_TROOP, 2);

			SDL_CenterRect(&pos, peasant, screen);

			SDL_FillRect( screen , NULL, 0xF1F101);

			SDL_BlitSurface( title, NULL , screen, &pos );

			//SDL_BlitSurface( font, NULL , screen, &pos );

			SDL_Rect right = { 0, 0, peasant->w, peasant->h };

			src.x = troop_frame * 48;
			src.h = peasant->h;
			troop_frame++;
			if (troop_frame > 3) troop_frame = 0; 			
			
			SDL_BlitSurface( peasant, &src , screen, NULL );

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