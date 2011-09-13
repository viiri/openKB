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

// For the meantime, use SDL directly, it if gets unwieldy, abstract it away 
#include "SDL.h"

#include "env.h"

#include "lib/kbauto.h"

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


KBgamestate press_any_key = {
	{
		{	{ 0, 0, 1024, 768  }, 0xFF, 0xFF},
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

void render_game(KBenv *env, KBgame *game, KBgamestate *state, KBconfig *conf) {

}

KBenv *KB_startENV(KBconfig *conf) {

	KBenv *nsys = malloc(sizeof(KBenv));
	
	if (!nsys) return NULL; 

    SDL_Init( SDL_INIT_VIDEO );

    nsys->screen = SDL_SetVideoMode( 320, 200, 32, SDL_SWSURFACE );
    
    nsys->conf = conf;

	return nsys;
}

void KB_stopENV(KBenv *env) {
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
		if (event.type == SDL_KEYUP) {
			int i, j = 0;
			for (i = 0; i < MAX_HOTSPOTS; i++) {
				if (state->spots[i].hot_key == 0) break;
				if (state->spots[i].hot_mod & 0xFF	/* "Any key" */ 
				 || (state->spots[i].hot_key == event.key.keysym.sym
				 	&& (1 == 1))	/* Modifier */
				 ) 
				{
					eve = i + 1;
					j = 1;									
					break;
				}
			}
			if (j) break;
		}
		if (event.type == SDL_MOUSEBUTTONUP) {
			mouse_x = event.button.x;
			mouse_y = event.button.y;
			click = 1;
			break;
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
			eve = new_hover;
		}
	}

	if (click != -1) eve = 1;

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

KBgame *select_game(KBconfig *conf) {

	return NULL;
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
		fprintf(stderr, "No modules found! You must either enable auto-discovery, either provide explicit file paths via config file.\n"); 	
		return -1;
	}

	prepare_inline_font();	// <-- inline font

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

		if (key == 0xFF) done = 1;
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

	main_module = &conf->modules[sel];
	return sel;
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

	int troop_frame = 0;
	SDL_Rect src = {0, 0, 48, 32};

	while (!done) {

		key = KB_event(&press_any_key);

		if (key) done = 1;

		if (redraw) {

			SDL_Rect pos;

			SDL_Surface *title = KB_LoadIMG8(GR_TITLE, 0);
			SDL_Surface *font = KB_LoadIMG8(GR_FONT, 0);
			SDL_Surface *peasant = KB_LoadIMG8(GR_TROOP, 2);

			SDL_CenterRect(&pos, peasant, screen);

			SDL_FillRect( screen , NULL, 0xFF3366);

			SDL_BlitSurface( title, NULL , screen, &pos );
			
			SDL_BlitSurface( font, NULL , screen, &pos );

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
	if (!main_module) {
		KB_errlog("No module selected.\n");
		KB_stopENV(sys);
		return -1;
	}

	/* --- X X X --- */
	display_logo();

	display_title();

	/* Select a game to play (new/load) */
	KBgame *game = select_game(conf);

	/* No game! Quit */
	if (!game) {
		KB_stdlog("No game selected.\n");
		KB_stopENV(sys);
		return 0;
	}

	/* Just for fun, output game name */
	KB_stdlog("%s the %s\n", game->name, classes[game->class][game->rank].title);

	//debug(game);

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