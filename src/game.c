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
#include "save.h"
#include "ui.h"
#include "lib/kbconf.h"
#include "lib/kbres.h"
#include "lib/kbauto.h"
#include "lib/kbstd.h"
#include "lib/kbsound.h"

#include "../vendor/vendor.h" /* scale2x, inprint, etc */

#include "env.h"

/* Global/main environment */
KBenv *sys = NULL;

void update_ui_frames();

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

	local.message_colors = KB_Resolve(COL_TEXT, CS_GENERIC);
	local.status_colors = KB_Resolve(COL_TEXT, CS_STATUS_2);
	local.topmenu_colors = KB_Resolve(COL_TEXT, CS_TOPMENU);

	update_ui_frames();
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

	free(local.message_colors);
	free(local.status_colors);
	free(local.topmenu_colors);

	SDL_FreeCachedSurfaces();

}

void update_ui_colors(KBgame *game) {
	if (local.status_colors) free(local.status_colors);
	local.status_colors = KB_Resolve(COL_TEXT, CS_STATUS_1 + game->difficulty);
}

void update_ui_frames() {

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




void render_game(KBenv *env, KBgame *game, KBgamestate *state, KBconfig *conf) {

}

/* Usefull shortcuts */
#define combat_log(STR, ...) KB_TopBox(MSG_WAIT | MSG_PADDED, (STR), __VA_ARGS__)
#define combat_error combat_log

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

	RECT_Text(&menu, rows, cols);
	RECT_Center(&menu, screen); 


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
	
	Uint32 *colors = local.message_colors;
	Uint32 *colors_inner = KB_Resolve(COL_TEXT, CS_MINIMENU);
	KBgame *game = NULL;

	int done = 0;
	int redraw = 1;
	int key = 0;

	int sel = 0;

	SDL_Rect menu;
	SDL_Rect menu_inner;

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

	/* Inner menu position and size */
	menu_inner.x = menu.x + fs->w*4 + fs->w/2;
	menu_inner.y = menu.y + fs->h*4 - fs->h/4;
	menu_inner.w = menu.w - fs->w*7;
	menu_inner.h = fs->h * num_files + (fs->h/4)*2;

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
			if (game == NULL) KB_errlog("Unable to load game '%s'\n", buffer);
			else KB_strcpy(game->savefile, fullname[sel]);

			done = 1;
		}
		//if (key == 3) { done = 1; }
		//if (key >= 4) { done = 1; }

		if (sel < 0) sel = 0;
		if (sel > num_files - 1) sel = num_files - 1;
		savegame_selection.hover = sel + 3;

		if (redraw) {

			int i;

			SDL_TextRect(screen, &menu, colors[COLOR_FRAME], colors[COLOR_BACKGROUND]);

			incolor(colors[COLOR_TEXT], colors[COLOR_BACKGROUND]);
			inprint(screen, " Select game:", menu.x + fs->w, menu.y + fs->w - fs->w/2);
			inprint(screen, "   Overlord  ", menu.x + fs->w, menu.y + fs->w*2);
			inprint(screen, "             ", menu.x + fs->w, menu.y + fs->w*4 - fs->w/2);
			
			SDL_FillRect(screen, &menu_inner, colors_inner[COLOR_BACKGROUND]);

			for (i = 0; i < num_files; i++) {
				incolor(colors[COLOR_TEXT], colors[COLOR_BACKGROUND]);
				char buf[4];sprintf(buf, "%d.", i + 1);
				inprint(screen, buf, menu.x + fs->w*2, fs->h * i + menu.y + fs->h*4);
				incolor(colors_inner[COLOR_TEXT], colors_inner[COLOR_BACKGROUND]);
				if (i == sel) incolor(colors_inner[COLOR_SEL_TEXT], colors_inner[COLOR_SEL_BACKGROUND]);
				inprint(screen, filename[i], menu.x + fs->w * 5, fs->h * i + menu.y + fs->h*4);
			}

	incolor(local.status_colors[COLOR_TEXT], local.status_colors[COLOR_BACKGROUND]);
	inprint(screen, " 'ESC' to exit \x18\x19 Return to Select  ", local.status.x, local.status.y);

	    	SDL_Flip( screen );

			redraw = 0;
		}

	}
	free(colors_inner);
	return game;
}

void show_credits() {

	SDL_Rect *fs = &sys->font_size;
	SDL_Rect *max, pos = { 0 };

	SDL_Surface *userpic = SDL_LoadRESOURCE(GR_SELECT, 2, 0);

	char *credits = KB_Resolve(STRL_CREDITS, 0);
	char *credits_ptr = credits;

	if (credits == NULL) credits_ptr = "openkb " PACKAGE_VERSION;

	max = KB_MessageBox(credits_ptr, MSG_HARDCODED);

	RECT_Size(&pos, userpic);
	RECT_Right(&pos, max);
	RECT_AddPos(&pos, max);

	pos.x -= (fs->w * 1);
	pos.y += (fs->h * 3);

	SDL_BlitSurface(userpic, NULL, sys->screen, &pos);

	KB_flip(sys);
	KB_Pause();

	SDL_FreeSurface(userpic);
	free(credits);
}

KBgame *select_game(KBconfig *conf) {

	SDL_Surface *screen = sys->screen;

	int key = 0;
	int done = 0;
	int redraw = 1;

	int credits = 1;

	KBgame *game = NULL;

	Uint32 *colors = KB_Resolve(COL_TEXT, CS_CHROME);

	SDL_Surface *title = SDL_LoadRESOURCE(GR_SELECT, 0, 0);

	while (!done) {

		key = KB_event(&character_selection);

		if (redraw) {

			SDL_Rect pos;

			RECT_Size(&pos, title);
			RECT_Center(&pos, screen);

			SDL_BlitSurface( title, NULL , screen, &pos );

			if (!credits) {
				KB_iloc(local.status.x, local.status.y);
				KB_icolor(local.status_colors);
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

			RECT_Size(&pos, title);
			RECT_Center(&pos, screen);

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

			RECT_Size(&pos, title);
			RECT_Center(&pos, screen);

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

			RECT_Size(&pos, peasant2);
			RECT_Center(&pos, screen);

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

inline byte KB_GetMapTile(KBgame *game, byte continent, int y, int x) {
	/* If coordinates are in bounds, return the tile */
	return  (y >= 0 && y < LEVEL_W - 1 && x >= 0 && x <= LEVEL_H - 1) 
			? game->map[continent][y][x] & 0x7F /* ***WITH INTERACTIVITY BIT REMOVED*** */
			: TILE_DEEP_WATER; /* otherwise, return water tile */
}

void KB_DrawMapTile(SDL_Surface *dest, SDL_Rect *dest_rect,	SDL_Surface *tileset, byte m) {

	SDL_Rect src;
	int th, tw;

	/* Calculate needed offsets on the 8x? tileset */
	th = m / 8;
	tw = m - (th * 8);

	src.w = dest_rect->w;
	src.h = dest_rect->h;
	src.x = tw * src.w;
	src.y = th * src.h;

/*	pos.x = i * (pos.w) + local.map.x;
	pos.y = (perim_h - 1 - j) * (pos.h) + local.map.y; */

	SDL_BlitSurface(tileset, &src, dest, dest_rect);
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
/*
 * Puzzle map screen.
 *
 * DOS aesthetics:
 *  first, "Press 'ESC' to exit" is displayed.
 *  then, whole puzzle map is shown
 *  then, it gets cleared piece by piece from top left corner
 *  artifacts actually go first, then villains
 *  while the clearing is performed, villains continue their animation routine.
 * NOTE:
 *  Other modules might require different aesthetics later :(
 */
void view_puzzle(KBgame *game) {

	SDL_Surface *artifacts = SDL_LoadRESOURCE(GR_VIEW, 0, 0);
	SDL_Surface *tile = SDL_LoadRESOURCE(GR_TILE, 0, 0);
	SDL_Surface *tileset = SDL_LoadRESOURCE(GR_TILESET, 0, 0);

	SDL_Surface *faces[MAX_VILLAINS]; /* Initialized below */

	/* Local variables to handle "opening" animation */ 
	int opened[PUZZLEMAP_W][PUZZLEMAP_H] = { 0 };
	int open_x = 0;
	int open_y = 0;
	enum {
		BEGIN,
		ARTIFACTS,
		VILLAINS,
		DONE,
	} open_mode = BEGIN;

	/* Puzzle map is drawn over regular map */
	SDL_Rect pos;
	RECT_Pos(&pos, &local.map); 

	/* Display this as soon as possible: */
	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");
	KB_flip(sys);

	/* Now, load all the villain faces */
	int i;
	for (i = 0; i < MAX_VILLAINS; i++)
		faces[i] = SDL_LoadRESOURCE(GR_VILLAIN, i, 0);

	int j;

	int border_x = game->scepter_x - (PUZZLEMAP_W / 2);// + PUZZLEMAP_W % 2);
	int border_y = game->scepter_y - (PUZZLEMAP_H / 2);// + PUZZLEMAP_H % 2);
 
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

			/* Attend to the "opening" animation */
			if (open_mode == BEGIN) {
				open_mode++;
			} else if (open_mode != DONE) {
				int id = puzzle_map[open_y][open_x];
				/* "id"s < 0 refer to artifacts; only open those in ARTIFACTS mode */
				if (id < 0 && open_mode == ARTIFACTS) {
					int artifact_id = -id - 1;
					if (game->artifact_found[artifact_id]) {
						opened[open_y][open_x] = 1;
					}
				}
				/* Other "id"s refer to villains; only open those in VILLAINS mode */
				if (id >= 0 && open_mode == VILLAINS) {
					if (game->villain_caught[id]) {
						opened[open_y][open_x] = 1;
					}
				}
				/* Advance the cursor */
				open_x++;
				if (open_x > PUZZLEMAP_W - 1) { open_x = 0; open_y++; }
				if (open_y > PUZZLEMAP_H - 1) { open_x = 0; open_y = 0; open_mode++; }
			}
		}

		if (redraw) {

			for (j = 0; j < PUZZLEMAP_H; j ++) {
				for (i = 0; i < PUZZLEMAP_W; i ++) {

					SDL_Rect dst = { pos.x + i * tile->w, pos.y + j * tile->h, tile->w, tile->h };

					/* Draw a map tile */
					if (opened[j][i]) {

							byte tile = KB_GetMapTile(game, game->scepter_continent,
								 border_y + (PUZZLEMAP_H - j) - 1,
								 border_x + i);

							if (IS_MAPOBJECT(tile)) tile = 0; /* Hide important objects */

							KB_DrawMapTile(sys->screen, &dst, tileset, tile);

					}
					/* Draw a villain face/artifact */
					else {

						int id = puzzle_map[j][i];

						if (id < 0) {

							int artifact_id = -id - 1;

							SDL_Rect src = { artifact_id * tile->w, 0, tile->w, tile->h };

							SDL_BlitSurface(artifacts, &src, sys->screen, &dst);

						} else {

							SDL_Rect src = { frame * tile->w, 0, tile->w, tile->h };

							SDL_BlitSurface(faces[id], &src, sys->screen, &dst);
						}
					}
				}
			}

			KB_flip(sys);
			redraw = 0;
		}
	}

	/* Now UNLOAD all the villains */
	for (i = 0; i < MAX_VILLAINS; i++)
		SDL_FreeSurface(faces[i]);

	/* And other resources */
	SDL_FreeSurface(artifacts);
	SDL_FreeSurface(tile);
	SDL_FreeSurface(tileset);
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

	Uint32 *colors = local.message_colors;

	Uint32 *map_colors = KB_Resolve(COL_MINIMAP, 0);

	SDL_Surface *tile = SDL_LoadRESOURCE(GR_PURSE, 0, 0);

	SDL_TextRect(sys->screen, &border, colors[COLOR_BACKGROUND], colors[COLOR_TEXT]);

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
				KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");
			else if (!orb)
				KB_TopBox(MSG_CENTERED, "'ESC' to exit / 'SPC' whole map");
			else
				KB_TopBox(MSG_CENTERED, "'ESC' to exit / 'SPC' your map");

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

	Uint32 *colors = local.message_colors;

	SDL_Surface *tile = SDL_LoadRESOURCE(GR_PURSE, 0, 0);

	SDL_TextRect(sys->screen, &border, colors[COLOR_BACKGROUND], colors[COLOR_TEXT]);

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

		char *text = KB_Resolve(STRL_VDESCS, villain_id);

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

		/* Print description, along with known residence information */
		KB_iloc(border.x + fs->w, border.y + fs->h);
		KB_iprintf(text,
			(continent == -1 ? "Unknown" : continent_names[continent]),
			(   castle == -1 ? "Unknown" : castle_names[castle])
		);

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

	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

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

			KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

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
		"proclaims:           (space)", MSG_PAUSE);

	sprintf(message, "\n\n"
		"My dear %s,\n\n"
		"I can aid you better after\n"
		"you've captured %d more\n"
		"villains.",
	game->name, needed);

	KB_BottomBox(message, "", MSG_PAUSE);
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
	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

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
				int result;
				int number = atoi(enter);
				if (number > max) number = max;

				/* BUY troop */
				result = buy_troop(game, home_troops[whom-1], number);

				/* Display error if any */
				if (result == 2) KB_BottomBox("\n\n\nYou don't have enough gold!", "", MSG_PAUSE);
				else if (result == 1) KB_BottomBox("", "No troop slots left!", MSG_PAUSE);//verify this one

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
	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

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
		char *name = STR_LoadRESOURCE(STRL_VNAMES, 0, game->castle_owner[id] & 0x1F);
		KB_iprintf("%s's rule.\n", name);
		free(name);
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

	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

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
	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

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
			int result;
			int number = atoi(enter);
			if (number > max) continue;
			if (number > game->dwelling_population[game->continent][id]) continue;

			/* BUY troop */
			result = buy_troop(game, troop_id, number); 
			if (!result) /* Success */
				/* Reduce dwelling population */
				game->dwelling_population[game->continent][id] -= number;

			/* Display error if any */
			else if (result == 2) KB_BottomBox("\n\n\nYou don't have enough gold!", "", MSG_PAUSE);
			else if (result == 1) KB_BottomBox("", "No troop slots left!", 1);//verify this one

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
	for (j = 0; j < LEVEL_H; j++) {
		for (i = 0; i < LEVEL_W; i++) {
			if (game->map[0][j][i] == TILE_SIGNPOST) {
				if (i == game->x && j == game->y) { ok = 1; break; }
				id ++;
			}
		}
		if (ok) break; 
	}

	char *sign = KB_Resolve(STR_SIGN, id);

	if (sign == NULL) {
		KB_errlog("Unable to read Signpost #%d\n", id);
		return;
	}

	KB_stdlog("Read sign post [%d] at %d, %d { %s }\n", id, game->x, game->y, sign);

	KB_BottomBox("A sign reads:", sign, MSG_PAUSE);
}

void take_chest(KBgame *game) {

	KBsound *snd_chest = KB_Resolve(SN_TUNE, TUNE_CHEST);

	KB_play(sys, snd_chest);

	SDL_Flip(sys->screen);
	KB_Pause();
	game->map[0][game->y][game->x] = 0;
	
	free(snd_chest);
}

void take_artifact(KBgame *game, byte id) {

}

void move_unit(KBcombat *war, int side, int id, int ox, int oy) {
	KBunit *u = &war->units[side][id];

	int nx = u->x + ox;
	int ny = u->y + oy;

	/* Screen border */
	if (nx < 0 || nx > CLEVEL_W - 1
	 || ny < 0 || ny > CLEVEL_H - 1) return;

	/* Obstacle */
	if (war->omap[ny][nx]) return;

	/* An other troop ... */
	if (war->umap[ny][nx]) {

		/* Left side */
		if (side == 0) {
			/* Meets left side! -- Friendly troop */
			if (war->umap[ny][nx] >= 1 && war->umap[ny][nx] <= MAX_UNITS) return;
			/* Otherwise -- Hostile troop */

			combat_log("PC Unit attacks!", NULL);
		}
		/* Right side */
		else {
			/* Meets right sie! -- Friendly troop */
			if (war->umap[ny][nx] >= (MAX_UNITS+1) && war->umap[ny][nx] <= (MAX_UNITS*2)) return;
			/* Otherwise -- Hostile troop */
			
			combat_log("NPC Unit attacks!", NULL);
		}

		return;
	}

	/* Actually move */
	unit_relocate(war, side, id, nx, ny);

	/* Spend 1 move point */
	u->moves--;
	if (!u->moves) u->acted = 1;
}

/* TOH: */ void combat_loop(KBgame *game, KBcombat *combat);
int run_combat(KBgame *game, int mode, int id) {

	KBcombat combat = { 0 };

	switch (mode) {

		case 0: /* Player VS Foe (id) */
		default:

			prepare_units_player(&combat, 0, game);
			prepare_units_foe(&combat, 1, game, game->continent, id);

		break;

	}

	combat_loop(game, &combat);

	return 0;
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
		run_combat(game, 0, id);
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

	KB_TopBox(0, "Build bridge in which direction <>ud");

	KB_flip(sys);

	int key = KB_event(&cross_choice);
	
	if (key == 0xFF) return 1;

	KB_TopBox(0, "Not a suitable location for a bridge");
	KB_flip(sys);
	KB_Pause();

	KB_TopBox(0, "What a waste of a good spell!");
	KB_flip(sys);
	KB_Pause();
	
	return 1;
}

int instant_army(KBgame *game) {

	KB_BottomBox("A few Sprites", "have joined to your army.", MSG_PAUSE);

	return 0;
}

int clone_army(KBgame *game, KBcombat *war) {

	int ok, x, y, unit_id, clones;

	KBunit *u = &war->units[war->side][war->unit_id];

	KB_TopBox(MSG_CENTERED, "Select your army to Clone");

	x = u->x;
	y = u->y;

	ok = pick_target(war, &x, &y, 3);

	if (ok) {

		unit_id = war->umap[y][x];

		clones = clone_troop(game, war, unit_id);

		combat_log("%d %s cloned", clones, troops[u->troop_id].name);

	}

}

int teleport_army(KBgame *game, KBcombat *war) {

	int ok, x, y, side, unit_id;

	KBunit *u = &war->units[war->side][war->unit_id];

	KB_TopBox(MSG_CENTERED, "Select army to Teleport");

	x = u->x;
	y = u->y;

	ok = pick_target(war, &x, &y, 2);

	if (ok) {

		side = 0;
		unit_id = war->umap[y][x] - 1;
		if (unit_id > 4) { side = 1; unit_id -= 5; }

		KB_TopBox(MSG_CENTERED, "Select new location");

		ok = pick_target(war, &x, &y, 1);

		if (ok) {
			/* Relocate unit */
			unit_relocate(war, side, unit_id, x, y);
		}

	}

	return ok;
}


/* Pass "combat" pointer for combat spells, NULL for adventure spells */
int choose_spell(KBgame *game, KBcombat *combat) {

	byte spell_id = 0xFF;

	SDL_Rect border;

	SDL_Surface *screen = sys->screen;

	SDL_Rect *fs = &sys->font_size;

	if (!game->knows_magic)
	{
		no_spell_banner();
		return;	
	}

	if (combat && combat->spells)
	{
		KB_TopBox(MSG_CENTERED, "Only 1 spell per round!");
		KB_flip(sys);
		KB_Wait();
		return;
	}

	int mode = (combat == NULL ? 1 : 0);

	RECT_Text((&border), 15, 36);
	RECT_Center(&border, sys->screen);
	
	border.y -= fs->h;

	Uint32 *colors = local.message_colors;

	SDL_TextRect(sys->screen, &border, colors[COLOR_BACKGROUND], colors[COLOR_TEXT]);

	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

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
		KB_iprintf("%2d %-12s", game->spells[i], spell_names[i]);
		KB_iprintf(" %c ", 'A' + i);
		if (mode)
			KB_imenu(&seven_choices, i, 12);
		KB_iprintf("%-13s %2d\n", spell_names[j], game->spells[j]);
	}

	KB_iloc(border.x + fs->w, border.y + fs->h * 13 + fs->h/4);
	KB_iprintf("Cast which %s spell (A-%c)?", (mode ? "Adventure" : "Combat"), 'A' + half);

	word twirl_x, twirl_y;

	KB_getpos(sys, &twirl_x, &twirl_y);

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

				KB_iloc(twirl_x, twirl_y);
				KB_iprintf("%c", 'A' + key - 1);

				KB_TopBox(MSG_CENTERED, "You don't know that spell!");
				KB_flip(sys);
				KB_Pause();

			} else {

				switch (spell_id) {
					case 0:	clone_army(game, combat);	break;
					case 1: teleport_army(game, combat);break;
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
				if (combat) combat->spells++;
			}

			done = 1;
		}

		if (redraw) {
			redraw = 0;

			KB_iloc(twirl_x, twirl_y);
			KB_iprintf("%c", twirl[twirl_pos]);

			KB_flip(sys);
		}
	}

	return spell_id;
}

void win_game(KBgame *game) {

	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

	KB_Pause();

}

void lose_game(KBgame *game) {

	char message[1024];
	char *lines;

	SDL_Rect full, half;
	SDL_Rect pos;

	SDL_Surface *image = SDL_LoadRESOURCE(GR_ENDING, 1, 0);

	full.x = local.map.x;
	full.y = local.map.y;
	full.w = sys->screen->w - local.frames[FRAME_RIGHT]->w - local.map.x;// + local.side.w;
	full.h = local.map.h;

	half.x = full.x;
	half.y = full.y;
	half.w = full.w - image->w;
	half.h = full.h;

	pos.x = 0;
	pos.y = 0;

	lines = KB_Resolve(STRL_ENDINGS, 1);

	//TODO: Remove this mess
	int i, j = 0, n = 19;
	char *line = lines;
	for (i = 0; i < n; ) {
		if (*line == '\0') {
			i++;
			*line = '\n';
		}
		line++;
	}

	/* TODO: Make sure message comes with appropriate "%s the %s" substring
	sprintf(message,
		game->name,
		classes[game->class][game->rank].title);
	*/

	KB_strcpy(message, lines);

	RECT_Pos(&pos, &full);
	RECT_Size(&pos, image);
	RECT_Right(&pos, &full);
	pos.x += full.x;

	KB_TopBox(MSG_CENTERED, "Press 'ESC' to exit");

	SDL_BlitSurface( image, NULL, sys->screen, &pos );

	SDL_FillRect( sys->screen, &half, 0xFF0000 );
	KB_iloc(half.x, half.y);
	KB_iprint(message);

   	KB_flip(sys);
	KB_Pause();

	SDL_FreeSurface(image);
	free(lines);
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
		KB_BottomBox(NULL, "\n\nYour search of this area has\nrevealed nothing.", MSG_PAUSE);
		spend_days(game, days);
	}

	return 1;
}

#define TARGET_ARROW_KEYS 16

#define TARGET_CONFIRM_EVENT (TARGET_ARROW_KEYS + 1)
#define TARGET_SYN_EVENT (TARGET_ARROW_KEYS + 2)

KBgamestate target_state = {
	{
		{	{ SOFT_WAIT }, SDLK_HOME, 0, KFLAG_SOFTKEY     	},
		{	{ SOFT_WAIT }, SDLK_7, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_UP, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_8, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_PAGEUP, 0, KFLAG_SOFTKEY   	},
		{	{ SOFT_WAIT }, SDLK_9, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_LEFT, 0, KFLAG_SOFTKEY     	},
		{	{ SOFT_WAIT }, SDLK_4, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_RIGHT, 0, KFLAG_SOFTKEY    	},
		{	{ SOFT_WAIT }, SDLK_6, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_END, 0, KFLAG_SOFTKEY    	},
		{	{ SOFT_WAIT }, SDLK_1, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_DOWN, 0, KFLAG_SOFTKEY     	},
		{	{ SOFT_WAIT }, SDLK_2, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_PAGEDOWN, 0, KFLAG_SOFTKEY 	},
		{	{ SOFT_WAIT }, SDLK_3, 0, KFLAG_SOFTKEY      	},

		{	{ 0 }, SDLK_RETURN, 0, 0      	},

		{	{ SOFT_WAIT }, SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};


#define COMBAT_ARROW_KEYS 16
#define COMBAT_ACTION_KEYS 9

#define COMBAT_SYN_EVENT (COMBAT_ACTION_KEYS + COMBAT_ARROW_KEYS + 1)

#define COMBAT_VIEW_ARMY    	0
#define COMBAT_VIEW_CONTROLS	1 
#define COMBAT_FLY          	2
#define COMBAT_GIVE_UP      	3
#define COMBAT_SHOOT        	4
#define COMBAT_USE_MAGIC    	5
#define COMBAT_VIEW_CHAR    	6
#define COMBAT_WAIT         	7
#define COMBAT_PASS         	8

KBgamestate combat_state = {
	{
		{	{ SOFT_WAIT }, SDLK_HOME, 0, KFLAG_SOFTKEY     	},
		{	{ SOFT_WAIT }, SDLK_7, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_UP, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_8, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_PAGEUP, 0, KFLAG_SOFTKEY   	},
		{	{ SOFT_WAIT }, SDLK_9, 0, KFLAG_SOFTKEY      	},
		{	{ SOFT_WAIT }, SDLK_LEFT, 0, KFLAG_SOFTKEY     	},
		{	{ SOFT_WAIT }, SDLK_4, 0, KFLAG_SOFTKEY      	},
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
		{	{ 0 }, SDLK_g, 0, 0      	},
		{	{ 0 }, SDLK_s, 0, 0      	},
		{	{ 0 }, SDLK_u, 0, 0      	},
		{	{ 0 }, SDLK_v, 0, 0      	},
		{	{ 0 }, SDLK_w, 0, 0      	},
		{	{ 0 }, SDLK_SPACE, 0, 0      	},

		{	{ SOFT_WAIT }, SDLK_SYN, 0, KFLAG_TIMER },
		0,
	},
	0
};

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

	SDL_Surface *tileset = SDL_TakeSurface(GR_TILESET, game->continent, 0);
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

		byte tile = KB_GetMapTile(game, game->continent, border_y + j, border_x + i); 

		pos.x = i * (pos.w) + local.map.x;
		pos.y = (perim_h - 1 - j) * (pos.h) + local.map.y;

		KB_DrawMapTile(screen, &pos, tileset, tile);
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

void draw_combat(KBcombat *war) {

	SDL_Rect *tile = local.map_tile;
	
	SDL_Surface *comtiles = SDL_TakeSurface(GR_COMTILES, 0, 0);
	
	/** Draw combat **/
	{
		int i, j;

		/* Draw obstacles */
		for (j = 0; j < CLEVEL_H; j++)
		for (i = 0; i < CLEVEL_W; i++) {

			SDL_Rect src = { war->omap[j][i] * tile->w, 0, tile->w, tile->h };
			SDL_Rect dst = { i * tile->w, j * tile->h, tile->w, tile->h };
			
			dst.x += local.map.x;
			dst.y += local.map.y;

			SDL_BlitSurface(comtiles, &src, sys->screen, &dst);
		}

		/* Draw units */
		for (j = 0; j < MAX_SIDES; j++) {
			for (i = 0; i < MAX_UNITS; i++) {

				KBunit *u = &war->units[j][i];

				if (u->count == 0) continue;

				SDL_Rect src = { u->frame * tile->w, j * tile->h, tile->w, tile->h };
				SDL_Rect dst = { 0, 0, tile->w, tile->h };

				dst.x = u->x * tile->w + local.map.x;
				dst.y = u->y * tile->h + local.map.y;

				SDL_Surface *troop = SDL_TakeSurface(GR_TROOP, u->troop_id, 1);

				SDL_BlitSurface(troop, &src, sys->screen, &dst);
			}
		}
	}

}

void draw_damage(KBcombat *war, KBunit *u) {

	SDL_Rect *tile = local.map_tile;
	SDL_Surface *comtiles = SDL_TakeSurface(GR_COMTILES, 0, 0);

	int frame = 0;
	int _x, _y;

	/** Draw "damage" **/
	{
		SDL_Rect src = { (frame+4) * tile->w, 0, tile->w, tile->h };
		SDL_Rect dst = { u->x * tile->w, u->y * tile->h, tile->w, tile->h };

		dst.x += local.map.x;
		dst.y += local.map.y;

		SDL_BlitSurface(comtiles, &src, sys->screen, &dst);
	}
}


void draw_combat_statusbar(KBcombat *war) {

	Uint32 *colors = local.status_colors;

	/* Status bar */
	SDL_FillRect(sys->screen, &local.status, colors[COLOR_BACKGROUND]);

	if (war->side) return;

	KB_iloc(local.status.x, local.status.y + 1);
	KB_iprint(" ");
	KB_iprint("Options");
	KB_iprint(" / ");

	/* Unit info */
	if (war->side == 0) {
		KBunit *u = &war->units[war->side][war->unit_id];
		KBtroop *t = &troops[u->troop_id];

		KB_iprintf("%s ", t->name);

		if (t->abilities & ABIL_FLY) {
			KB_iprint("F,");
		}

		KB_iprintf("M%d", u->moves);

		if (troops[u->troop_id].ranged_ammo) {
			KB_iprintf(",S%d", u->shots);
		}
	}
}

void draw_defeat() {

}

static signed char target_move_offset_x[9] = { -1, 0, 1, -1, 1, -1, 0, 1 };
static signed char target_move_offset_y[9] = { -1,-1,-1,  0, 0,  1, 1, 1 };

/* Shows a target cursor and allows player to move over the battlefield using arrow keys.
 *  The "filter" argument should be:
 *   0 - no filter, accept any tile, not too usefull
 *	 1 - tile unoccupied
 *   2 - tile with a unit on it
 *   3 - friendly unit
 *   4 - enemy unit
 */
int pick_target(KBcombat *war, int *x, int *y, int filter) {

	int confirmed = 0; 
	int done = 0;
	int redraw = 1;

	int key = 0;

	int frame = 0;
	int max_frame = 3;

	int _x = *x;
	int _y = *y;
	
	SDL_Rect *tile = local.map_tile;
	SDL_Surface *comtiles = SDL_TakeSurface(GR_COMTILES, 0, 0);

	while (!done) {

		key = KB_event(&target_state);
		
		if (key == 0xFF) done = 1;

		if (key == TARGET_CONFIRM_EVENT) {
			int accept = 0;
			/* If there's a filter, verify tile is acceptable. See above for "filter" values */
			if (!filter) accept = 1;
			else if (filter == 1) {
				accept = (!war->umap[_y][_x] && !war->omap[_y][_x] ? 1 : 0); 
			} else if (filter >= 2) {
				if (war->umap[_y][_x]) {
					int side = (war->umap[_y][_x] <= MAX_UNITS ? 0 : 1);
					if (filter == 3) accept = (side == 0 ? 1 : 0);
					else if (filter == 4) accept = (side == 1 ? 1 : 0);
					else accept = 1;
				}
			}
			/* We're done */
			if (accept) {
				done = 1;
				confirmed = 1;
				redraw = 1;
			}
		}

		/* Cursor is being moved */
		if (key >= 1 && key <= TARGET_ARROW_KEYS) {
			int ox = target_move_offset_x[(key-1)/2];
			int oy = target_move_offset_y[(key-1)/2];

			/* Do not allow moving past screen border */
			if (_x + ox < 0 || _x + ox > CLEVEL_W - 1) ox = 0;
			if (_y + oy < 0 || _y + oy > CLEVEL_H - 1) oy = 0;

			_x += ox;
			_y += oy;
		}

		if (key == TARGET_SYN_EVENT) {
			frame++;
			if (frame > max_frame) {
				frame = 0;
			}
			redraw = 1;
		}

		if (redraw) {

			draw_combat(war);

			/** Draw cursor **/
			if (!done) {
				SDL_Rect src = { (frame+11) * tile->w, 0, tile->w, tile->h };
				SDL_Rect dst = { _x * tile->w, _y * tile->h, tile->w, tile->h };

				dst.x += local.map.x;
				dst.y += local.map.y;

				SDL_BlitSurface(comtiles, &src, sys->screen, &dst);
			}

			KB_flip(sys);
			redraw = 0;
		}

	}

	*x = _x;
	*y = _y;

	return confirmed;
}

int unit_try_wait(KBcombat *war) {
	KBunit *u = &war->units[war->side][war->unit_id];
	KBtroop *t = &troops[u->troop_id];

	if (war->phase) u->acted = 1;

	combat_log("%s wait", t->name);

	return 1;
}

void unit_try_pass(KBcombat *war) {
	KBunit *u = &war->units[war->side][war->unit_id];
	KBtroop *t = &troops[u->troop_id];

	u->acted = 1;

	combat_log("%s pass", t->name);
}

void unit_try_fly(KBcombat *war) {

	int nx, ny;
	int ok;

	KBunit *u = &war->units[war->side][war->unit_id];
	KBtroop *t = &troops[u->troop_id];

	if (!(t->abilities & ABIL_FLY)) {
		combat_error("Can't Fly", NULL);
		return;
	}

	nx = u->x;
	ny = u->y;

	ok = pick_target(war, &nx, &ny, 1);

	if (ok) {
		/* Actually move */
		war->umap[u->y][u->x] = 0;
		war->umap[ny][nx] = (war->side * MAX_UNITS) + war->unit_id + 1;

		u->x = nx;
		u->y = ny;

		/* Spend 1 flight point */
		u->flights--;
		if (!u->flights) u->acted = 1;
	} 
}

void unit_try_shoot(KBcombat *war) {

	int x, y;
	int other_id, other_side;
	int ok, kills;

	// TODO: display "Can't Shoot" message for units that can't shoot

	if (unit_surrounded(war, war->side, war->unit_id)) return;

	x = war->units[war->side][war->unit_id].x;
	y = war->units[war->side][war->unit_id].y; 

	ok = pick_target(war, &x, &y, 4);

	if (ok) {

		other_id = war->umap[y][x] - 1;
		other_side = 0;
		if (other_id >= MAX_UNITS) {
			other_id -= MAX_UNITS;
			other_side = 1;
		}

		kills = unit_ranged_shot(war, war->side, war->unit_id, other_side, other_id);

		KBunit *u = &war->units[war->side][war->unit_id];
		KBunit *victim = &war->units[other_side][other_id];

		draw_damage(war, victim);

		combat_log("%s shoot %s killing %d", troops[u->troop_id].name, troops[victim->troop_id].name, kills);

		//unit_apply_damage(war, victim);

		/* A turn well spent */
		u->acted = 1;
	}

}

int ai_pick_target(KBcombat *combat, int nearby) {

	int side = 1 - combat->side;
	int pick = -1;
	int i;
	for (i = 0; i < MAX_UNITS; i++) {
		KBunit *u = &combat->units[side][i];
		if (!u->count) continue;

		if (nearby && unit_touching(combat, side, i, combat->unit_id)) continue; 

		if (pick != -1) {
			KBunit *picked = &combat->units[side][pick];
			/* Units that shoot has higher priority */
			if (picked->shots) {
				if (!u->shots) continue;
				/* If both shoot, pick one with higher damage */
				if (troops[u->troop_id].ranged_max < troops[picked->troop_id].ranged_max) continue; 
			} else {
				/* Pick one with higher melee */
				if (troops[u->troop_id].melee_max < troops[picked->troop_id].melee_max) continue;
			}
		}
		/* Save new one */
		pick = i;
	}

	return pick;
}

void ai_unit_think(KBcombat *combat) {

	KBunit *u = &combat->units[combat->side][combat->unit_id];
	KBtroop *t = &troops[u->troop_id];

	int close_target = ai_pick_target(combat, 1);

	if (t->abilities & ABIL_FLY) {

		int far_target = ai_pick_target(combat, 0);

		//draw_status("Unitia fly");
		KB_flip(sys);
		SDL_Delay(300);

	}

	if (u->shots) {

		/* Must not be blocked */
		if (close_target == -1) {

			int far_target = ai_pick_target(combat, 0);
			if (far_target != -1) {
				KBunit *target = &combat->units[1-combat->side][far_target];
				//shoot_projectile(combat, far_target);
			}

		}

	}

}

static signed char combat_move_offset_x[9] = { -1, 0, 1, -1, 1, -1, 0, 1 };
static signed char combat_move_offset_y[9] = { -1,-1,-1,  0, 0,  1, 1, 1 };

static signed char move_offset_x[9] = { -1, 0, 1, -1, 0, 1, -1,  0,  1 };
static signed char move_offset_y[9] = {  1, 1, 1,  0, 0, 0, -1, -1, -1 };

/* Main combat loop (combat screen) */
void combat_loop(KBgame *game, KBcombat *combat) {
	
	reset_match(combat);

	int key = 0;
	int done = 0;
	int redraw = 1;

	int frame = 0;
	int max_frame = 3;

	int ai_turn = 0;
	int ai_think = 2;
	int pass = 0;

#define KEY_ACT(ACT) (COMBAT_ARROW_KEYS + 1 + COMBAT_ ## ACT)

	while (!done) {
		key = KB_event(&combat_state);

		if (key == 0xFF) done = 1;

		switch (key) {
			case KEY_ACT(VIEW_ARMY):	view_army(game);    	break;
			case KEY_ACT(VIEW_CONTROLS):

			break;
			case KEY_ACT(FLY):      	unit_try_fly(combat);	break;
			case KEY_ACT(GIVE_UP):

			break;
			case KEY_ACT(SHOOT):    	unit_try_shoot(combat);	break;
			case KEY_ACT(USE_MAGIC):

				choose_spell(game, combat);

			break;
			case KEY_ACT(VIEW_CHAR):	view_character(game);	break;
			case KEY_ACT(WAIT):     	pass = unit_try_wait(combat); 	break;
			case KEY_ACT(PASS):     	unit_try_pass(combat); 	break;
			default: break;
		}

		if (key > 0 && key < COMBAT_ARROW_KEYS) {
			int ox = combat_move_offset_x[(key-1)/2];
			int oy = combat_move_offset_y[(key-1)/2];
			
			move_unit(combat, 0, combat->unit_id, ox, oy);
		}

		if (key == COMBAT_SYN_EVENT) {

			if (++combat->units[combat->side][combat->unit_id].frame > 3)
				combat->units[combat->side][combat->unit_id].frame = 0;

			frame++;
			if (frame > max_frame) {
				frame = 0;
				max_frame = 2;
			}
			redraw = 1;
		}

		if (pass || combat->units[combat->side][combat->unit_id].acted) {

			combat->units[combat->side][combat->unit_id].frame = 0;

			if (!test_defeat(game, combat)) {

				done = 2; 
				break;

			} else if (!test_victory(combat)) {

				done = 1;
				break;

			} else {

				int next = next_unit(combat);
				if (next == -1) next_turn(combat);
				else
					combat->unit_id = next;

			}

			redraw = 1;
			pass = 0;
		}

		if (redraw) {

			draw_combat(combat);

			draw_combat_statusbar(combat);

			KB_flip(sys);
			redraw = 0;
		}

	}
#undef KEY_ACT
}


int save_game(KBgame *game) {
	int err;
	char buffer[PATH_LEN];
	KB_dircpy(buffer, sys->conf->save_dir);
	KB_dirsep(buffer);
	KB_strcat(buffer, game->savefile);
	KB_strcat(buffer, "2");//TODO: Remove this '2', it's for debug purposes only

	err = KB_saveDAT(buffer, game);
	if (err) KB_errlog("Unable to save game '%s'\n", buffer);
	else KB_stdlog("Saved game into '%s'\n", buffer);
}

KBgamestate quit_question = {
	{
		{	{ 0 }, 0xFF, 0, KFLAG_ANYKEY },
		{	{ 0 }, SDLK_q, KMOD_CTRL, 0	},
		0
	},
	0
};
int ask_quit_game(KBgame *game) {

	save_game(game);

	KB_BottomBox(NULL, "\nYour game has been saved.\n\nPress Control-Q to Quit or\nany other key to continue.", 0);

	int done = 0;
	while (!done) {
		int key = KB_event(&quit_question);
		if (key) done = key;
	}

	return 2 - done;
}
int ask_fast_quit(KBgame *game) {

	const char *twirl = "\x1D" "\x05" "\x1F" "\x1C" ; /* stands for: | / - \ */
	byte twirl_pos = 0;
	word twirl_x, twirl_y;

	KB_TopBox(0, " Quit to DOS without saving (y/n) ");

	KB_getpos(sys, &twirl_x, &twirl_y);

	int redraw = 1;
	int done = 0;
	while (!done) {
		int key = KB_event(&yes_no_interactive);

		if (key == 3) {
			twirl_pos++;
			if (twirl_pos > 3) twirl_pos = 0;
			redraw = 1;
		}
		else
		if (key) done = key;

		if (redraw) {
			redraw = 0;

			KB_iloc(twirl_x, twirl_y);
			KB_iprintf("%c", twirl[twirl_pos]);

			KB_flip(sys);
		}

	}

	return done - 1;
}

/* Main game loop (adventure screen) */
void adventure_loop(KBgame *game) {

	SDL_Surface *screen = sys->screen;

	Uint32 *colors = local.message_colors;

	SDL_Rect status_rect = { local.status.x, local.status.y, local.status.w, local.status.h };

	SDL_BlitSurface(local.border[FRAME_MIDDLE], NULL, screen, local.frames[FRAME_MIDDLE] );

	//int tileset_pitch = local.tileset->w / tile->w;

	KBsound *snd_walk = KB_Resolve(SN_TUNE, TUNE_WALK);

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
			choose_spell(game, NULL);
		}

		if (key == KEY_ACT(VIEW_CHAR)) {
			view_character(game);
		}

		if (key == KEY_ACT(END_WEEK)) {
			spend_week(game);
			end_week(game);
		}

		if (key == KEY_ACT(SAVE_QUIT)) {
			if (!ask_quit_game(game)) done = 1;
		}
		if (key == KEY_ACT(FAST_QUIT)) {
			if (!ask_fast_quit(game)) done = 1;
		}

		if (key == KEY_ACT(DISMISS_ARMY)) {
			dismiss_army(game);
			if (!test_defeat(game, NULL)) {
				temp_death(game);
				draw_defeat();
			}
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

			KB_play(sys, snd_walk);

			int ox = move_offset_x[(key-1)/2];
			int oy = move_offset_y[(key-1)/2];

			int cursor_x = game->x + ox;
			int cursor_y = game->y + oy;

			if (cursor_x < 0) cursor_x = 0;
			if (cursor_y < 0) cursor_y = 0;
			if (cursor_x > LEVEL_W - 1) cursor_x = LEVEL_W - 1;
			if (cursor_y > LEVEL_H - 1) cursor_y = LEVEL_H - 1;

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

				if (m == TILE_TELECAVE && !visit_telecave(game, 1)) {
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
					case TILE_CASTLE:   	visit_castle(game);	walk = 0; break;
					case TILE_TOWN:     	visit_town(game); walk = 0; break;
					case TILE_CHEST:    	take_chest(game);       	break;
					case TILE_DWELLING_4:	if (!visit_telecave(game, 0)) break;
					case TILE_DWELLING_1:
					case TILE_DWELLING_2:
					case TILE_DWELLING_3:	visit_dwelling(game, m - TILE_DWELLING_1); walk = 0; break;
					case TILE_SIGNPOST: 	read_signpost(game);    	break;
					case TILE_FOE:      	walk = !attack_foe(game);	break;
					case TILE_ARTIFACT_1:
					case TILE_ARTIFACT_2:	take_artifact(game, m - TILE_ARTIFACT_1);	break;
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
			if (end_day(game)) {
				end_week(game);
			}
		}
		if (game->days_left == 0) {
			lose_game(game);
			done = 1;
		}

		if (redraw) {

			draw_player(game, frame);

			draw_sidebar(game, tick);

			/* Status bar */
			KB_TopBox(0, " Options / Controls / Days Left:%d ", game->days_left);

	    	KB_flip(sys);
			redraw = 0;
		}
	}
#undef KEY_ACT
	free(snd_walk);
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
		/* Put character name into window title bar */
		char buffer[1024];
		KB_strcpy(buffer, game->name);
		KB_strcat(buffer, " the ");  
		KB_strcat(buffer, classes[game->class][game->rank].title);
		KB_strcat(buffer, " - openkb " PACKAGE_VERSION);
		SDL_WM_SetCaption(buffer, buffer);

		/* And log it into stdout */
		KB_stdlog("%s the %s (%d days left)\n", game->name, classes[game->class][game->rank].title, game->days_left);

		/* HACK - Update color resource based on difficulty setting */
		update_ui_colors(game);

		/* PLAY THE GAME */
		adventure_loop(game);
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