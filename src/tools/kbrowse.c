/*
 * KB-BROWSE
 *
 * King's Bounty resource explorer for SDL, based on openkb "library".
 *
 * The purpose of this little program is to expose some of the library
 * functionality, particularly virtual directory access.
 * Could be used as a testbed / modding tool.
 *
 * This code inherits the copyright and GPLv3 from the openkb library.
 */
#include "../lib/kbdir.h"
#include "../lib/kbstd.h"
#include "../lib/kbfile.h"

#include "SDL.h"
#include "../../vendor/savepng.h"

SDL_Surface *screen;

SDL_Surface *pal;

SDL_Surface *create_indexed_surface() {
	const int zoom = 16;
	int i, j; /* feel free to change surface resolution: */
	SDL_Surface *pal = SDL_CreateRGBSurface(0, 16*zoom, 16*zoom, 8, 0,0,0,0);
	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			SDL_Rect pixel = { i * zoom, j * zoom, zoom, zoom };
			SDL_FillRect(pal, &pixel, j *zoom + i);
		}
	}
	return pal;
}

void draw_hintbar() {

	typedef struct {
		char key[32];
		char desc[128];
	} hint_t;
	hint_t hints[] = {
		{ "F2", "Save As" },
		{ "F3", "Load From" },
		{ "^I", "Grab Palette" },
		{ "^O", "Apply Palette" },
		{ "^P", "Dump Palette" },
	};

	int x, y, i, n;

	n = sizeof(hints) / sizeof(hint_t); 

	x = 0;
	y = screen->h - 16;

	
	for (i = 0; i < n; i++) {
		hint_t *hint = &hints[i];
		int l = strlen(hint->key);

		incolor(0xAEAEAE, 0x000000);
		inprint(screen, hint->key, x, y);
		l += 1;
		x += l * 8;

		l = strlen(hint->desc);
		incolor(0x999999, 0x000000);
		inprint(screen, hint->desc, x, y);
		l += 3;
		x += l * 8;
	}	
}

int get_sdl_event(int *mod) {
	SDL_Event event;

	int eve = 0;
	*mod = 0;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT)
		{
			eve = SDLK_ESCAPE;
			break;
		}
		if (event.type == SDL_KEYDOWN) 
		{
			eve = event.key.keysym.sym;
			*mod = (event.key.keysym.mod & KMOD_CTRL) ? 1 : 0;
			break;
		}
	}

	return eve;
}

typedef unsigned char byte;

char *input_text(int max_len, int numbers_only, int x, int y) {
	int mod = 0;
	int key = 0;
	int done = 0;
	int redraw = 1;

	SDL_Rect menu;
	SDL_Rect font = { 0, 0, 8, 8 };
	SDL_Rect *fs = &font;

	char *entered_name = malloc(sizeof(char) * max_len);

	entered_name[0] = '\0';
	int curs = 0;

	const char *twirl[] = {
		"\x1D" "\0x00",
		"\x05" "\0x00",
		"\x1F" "\0x00",
		"\x1C" "\0x00" } ; /* stands for: | / - \ */
	byte twirl_pos = 0;

	while (!done) {

		key = get_sdl_event(&mod);

		if (key == SDLK_ESCAPE) { curs = 0; done = -1; }

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

			inprint(screen, entered_name, x, y);
			inprint(screen, twirl[twirl_pos], x + curs * fs->w, y);

			SDL_Flip( screen );

			redraw = 0;
		}
	}

	inprint(screen, " ", x + curs * 8, y);
	entered_name[curs] = '\0';

	if (done == -1) {
		free(entered_name);
		entered_name = NULL;
	}

	return entered_name;
}

void examine_directory(const char *path) {

	int done = 0;
	int redraw = 1;
	int reread = 1;
	int miniread = 0;

	int selected_file = 0;
	
	KB_Entry list[64];

	int reading_offset = 0;
	int display_lines = 20;
	
	int known_maximum = 0;

	SDL_Surface *draw = NULL;

	char buffer[4096];

	KB_DIR *dirp = KB_opendir(path);

	if (!dirp) {
		printf("Unable to open directory '%s' !\n", path);
		return;
	}

	display_lines = screen->h / 8;
	if (display_lines > 64) display_lines = 64;

	display_lines -= 2; /* because status bars */

	buffer[0] = '\0';
	while (!done) {
		
		int mod;
		int key = get_sdl_event(&mod);

		if (key == SDLK_i && mod) {
			//grab_pallette(draw, buffer);
			KB_File *f = KB_fopen(buffer, "rb");
			if (f) {
				SDL_Color *colors = DOS_ReadPalette_FD(f);
				//Apply to pal
				SDL_SetColors(pal, colors, 0, 256);
				//cleanup
				//KB_fclose(f);
				free(colors);
				redraw = 1;
			}
		}
		if (key == SDLK_o && mod && draw) {
			//apply_pallette(draw);
			SDL_SetColors(draw, pal->format->palette->colors, 0, 256);
			redraw = 1;
		}
		if (key == SDLK_p && mod) {
			//dump_pallette(pal);
			char *file;
			inprint(screen, "Enter filename to save palette as:            ", 0, screen->h - 16);
			file = input_text(80, 0, 10, screen->h - 8);
			if (file) {
				if (!SDL_SavePNG(pal, file)) {
					printf("Wrote PNG palette: %s\n", file);
				} else {
					printf("Unable to write PNG palette: %s\n", SDL_GetError());
				}
			}
			redraw = 1;
		}
		if (key == SDLK_F2 && draw) {
			char *file;
			inprint(screen, "Enter filename to save file as:            ", 0, screen->h - 16);
			file = input_text(80, 0, 10, screen->h - 8);
			if (file) {
				if (!SDL_SavePNG(draw, file)) {
					printf("Wrote PNG file: %s\n", file);
				} else {
					printf("Unable to write PNG file: %s\n", SDL_GetError());
				}
			}
			redraw = 1;
		}

		if (key == SDLK_ESCAPE) done = 1;
		if (key == SDLK_DOWN || key == SDLK_PAGEDOWN || key == SDLK_END) {
			int step = 1;
			if (key == SDLK_PAGEDOWN) step = display_lines / 2;
			if (key == SDLK_END) step = 1024;
			if (!known_maximum || selected_file < known_maximum - 1) {
				selected_file += step;
				redraw = 1;
			}
			if (selected_file >= display_lines) {
				display_lines = display_lines - 1;
			}
			if (selected_file - reading_offset > display_lines / 2) {
				reading_offset += step;
				reread = 1;
			}
			if (known_maximum && reading_offset >= known_maximum) {
				reading_offset = known_maximum - 1;
			}
			miniread = 1;
		}
		if (key == SDLK_UP || key == SDLK_PAGEUP || key == SDLK_HOME) {
			int step = 1;
			if (key == SDLK_PAGEUP) step = display_lines / 2;
			if (key == SDLK_HOME) step = 1024;
			if (selected_file > 0) {
				selected_file -= step;
				redraw = 1;
			}
			if (selected_file < 0) {
				selected_file = 0;
			}
			if (reading_offset && selected_file - reading_offset < display_lines / 2) {
				reading_offset -= step;
				reread = 1;
			}
			if (reading_offset < 0) {
				reading_offset = 0;
			}
			miniread = 1;
		}

		if (reread) {
			int i, j = 0;

			//KB_seekdir(dirp, reading_offset);			
			KB_seekdir(dirp, 0);

			for (i = 0; i < reading_offset; i++) {
				KB_Entry *e = KB_readdir(dirp);
			}
			for (i = 0; i < display_lines; i++) {

				KB_Entry *e = KB_readdir(dirp);

				if (e == NULL) {
					known_maximum = reading_offset + i;
					break;
				}

				memcpy(&list[i], e, sizeof(KB_Entry));
			}

			if (known_maximum && selected_file > known_maximum - 1) {
				selected_file = known_maximum - 1;
			}

			redraw = 1;
			reread = 0;
		}
		if (miniread == 1) {
			int i = selected_file - reading_offset;
			KB_dircpy(buffer, path);
			if (dirp->type == KBDTYPE_DIR)
				KB_dirsep(buffer);
			else
				KB_grpsep(buffer);
			KB_strcat(buffer, list[i].d_name);

			miniread = 0;
			key = SDLK_SPACE;
		}

		if (key == SDLK_SPACE) {
			if (draw) SDL_FreeSurface(draw);
			draw = KB_LoadIMG(buffer);
			redraw = 1;
		}

		if (key == SDLK_RETURN) {
			int re_selected_file = selected_file - reading_offset;

			if (!strcasecmp(list[re_selected_file].d_name, "..")) { done = 1; continue; }		

			if (!strcasecmp(list[re_selected_file].d_name, ".")) continue;

			if (buffer[0] != '\0')
				examine_directory(buffer);

			redraw = 1;
		}

		if (redraw) {
			SDL_FillRect(screen, NULL, 0x000000);

			int i;
			for (i = 0; i < display_lines; i++) {

				if (known_maximum && i + reading_offset >= known_maximum) {
					break;
				}

				if (reading_offset + i == selected_file)
					incolor(0xFFFFFF, 0x0000FF);
				else
					incolor(0x999999, 0x333333);

				inprint(screen, list[i].d_name, 10, 10 + i * 8);
			}

			if (draw) {
				SDL_Rect dst = { screen->w - draw->w - 10, 10, draw->w, draw->h };
				SDL_BlitSurface(draw, NULL, screen, &dst);
			}

			{
				SDL_Rect dst = { screen->w - pal->w - 10, (draw ? draw->h : 0) + 40, pal->w, pal->h };
				SDL_BlitSurface(pal, NULL, screen, &dst);
				inprint(screen, "grabbed palette:", dst.x, dst.y - 8);
			}

			incolor(0xFFFFFF, 0x333333);
			inprint(screen, path, 10, 0);

			draw_hintbar();

			SDL_Flip(screen);

			redraw = 0;		
		}
	}

	if (draw) SDL_FreeSurface(draw);

	KB_closedir(dirp);
}

int main(int argc, char* argv[]) {

	Uint32 width = 800;
	Uint32 height = 600;
	Uint32 bpp = 32;
	Uint32 flags = 0;

	if (argc < 2) {
		fprintf(stderr, "Usage: kbrowse DIRECTORY\n");
		return 1;
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		exit(-1);
	}

	if ((screen = SDL_SetVideoMode(width, height, bpp, flags)) == 0) {
		fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
       	exit(-1);
	}

	SDL_Surface *font = SDL_LoadBMP("../../data/free/openkb8x8.bmp"); 

	pal = create_indexed_surface();

	infont(font);

	examine_directory(argv[1]);

	SDL_FreeSurface(pal);
	SDL_FreeSurface(font);

	SDL_Quit(); 
	return 0; 
}