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

#include "SDL.h"

SDL_Surface *screen;

int get_sdl_event() {
	SDL_Event event;

	int eve = 0;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT)
		{
			eve = SDLK_ESCAPE;
			break;
		}
		if (event.type == SDL_KEYDOWN) 
		{
			eve = event.key.keysym.sym;
			break;
		}
	}

	return eve;
}

void examine_directory(const char *path) {

	int done = 0;
	int redraw = 1;
	int reread = 1;

	int selected_file = 0;
	
	KB_Entry list[20];

	int reading_offset = 0;
	int display_lines = 20;
	
	int known_maximum = 0;

	KB_DIR *dirp = KB_opendir(path);

	if (!dirp) {
		printf("Unable to open directory '%s' !\n", path);
		return;
	}

	while (!done) {

		int key = get_sdl_event();

		if (key == SDLK_ESCAPE) done = 1;
		if (key == SDLK_DOWN) {
			if (!known_maximum || selected_file < known_maximum -1) {
				selected_file ++;
				redraw = 1;
			}
			
			if (selected_file - reading_offset > 10) {
				reading_offset += 1;
				reread = 1;
			}
		}
		if (key == SDLK_UP) {
			if (selected_file > 0) {
				selected_file --;
				redraw = 1;
			}
			if (reading_offset && selected_file - reading_offset < 10) {
				reading_offset -= 1;
				reread = 1;
			}
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
#if 0
		if (key == SDLK_g) {

			char buffer[4096];
			strcpy(buffer, path);
			if (buffer[strlen(buffer)-1] != '#') 
				strcat(buffer, "#");
			strcat(buffer, "ammi.4");

			examine_directory(buffer);

			redraw = 1;
		}
		if (key == SDLK_h) {

			char buffer[4096];
			strcpy(buffer, path);
			if (buffer[strlen(buffer)-1] != '/') 
				strcat(buffer, "/");
			strcat(buffer, "416.CC");

			examine_directory(buffer);

			redraw = 1;
		}
#endif
		if (key == SDLK_RETURN) {

			if (!strcasecmp(list[selected_file].d_name, "..")) { done = 1; continue; }		

			if (!strcasecmp(list[selected_file].d_name, ".")) continue;

			char buffer[4096];
			strcpy(buffer, path);
			if (buffer[strlen(buffer)-1] != '/') 
				strcat(buffer, "/");
			strcat(buffer, list[selected_file].d_name);

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

			SDL_Flip(screen);

			redraw = 0;		
		}
	}

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

	infont(font);

	examine_directory(argv[1]);

	SDL_FreeSurface(font);

	SDL_Quit(); 
	return 0; 
}