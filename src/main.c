/*
 *  main.c -- the openkb game itself
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
#include "config.h"
#ifndef HAVE_LIBSDL
	#error "Sorry, there's no reason to compile without HAVE_LIBSDL define..."
#else
	/* We must include SDL.h here, because on some platforms (OSX)
         * main shall be replaced with SDLmain. */
	#include <SDL.h>
#endif
#include "lib/kbconf.h"

#include "lib/kbstd.h"

struct KBconfig KBconf;

extern int run_game(KBconfig *conf);

void dump_version(void) {
	KB_stdlog("openKB " PACKAGE_VERSION "\n");
	KB_stdlog("Copyright (C) 2011-2014 openKB AUTHORS\n");
	KB_stdlog("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
	KB_stdlog("This is free software: you are free to change and redistribute it.\n");
	KB_stdlog("There is NO WARRANTY, to the extent permitted by law.\n");

	//KB_stdlog("==============================================================================\n");
	KB_stdlog("Compiled with SDL %d.%d.%d\n",SDL_MAJOR_VERSION,SDL_MINOR_VERSION,SDL_PATCHLEVEL);
#ifdef HAVE_LIBSDL_IMAGE
	KB_stdlog("Compiled with SDL_Image\n");
#endif
	//* print LGPL license...
#ifdef HAVE_LIBPNG
	KB_stdlog("Compiled with libpng\n");
#endif
	//* print zlib license..
#ifdef DEBUG
	KB_stdlog("Compiled with DEBUG define.\n");
#endif
#ifdef USE_WINAPI
	KB_stdlog("Compiled with Win32Api support.\n");
#endif
}

int main(int argc, char* argv[]) {

	int playing = 1;	/* Play 1 game of KB */

	/* Hack -- dump version and exit */
	if (argc > 1 && !KB_strcasecmp(argv[1], "--version")) {
		dump_version();
		return 0;
	}

	/* Lots of very boring things must happen for a proper initialisation... */
	KB_stdlog("openKB version " PACKAGE_VERSION "\n");
	KB_stdlog("=====================================================\n");

	/* Let's start by searching for a config file,
	 * then see if there're "data" and "save" directories, and finally
	 * if we could load any usable data from "data" */
	struct KBconfig CMDconf;

	/* Some defaults */
	wipe_config(&KBconf);
	read_env_config(&KBconf);

	/* Obtain config from command-line arguments */
	wipe_config(&CMDconf);
	read_cmd_config(&CMDconf, argc, argv);

	/* User requested a specific config file */
	if (CMDconf.config_file[0] != '\0') 
	{
		if (test_config(CMDconf.config_file, 0)) 
		{
			KB_errlog("[config] Unable to read config file '%s'\n", &CMDconf.config_file[0]); 
			return 1;
		}
		KB_strcpy(KBconf.config_file, CMDconf.config_file);
	}
	/* Search for a config file */
	else
	{
		find_config(&KBconf);
	}

	/* Read found/requested config file */
	if (!test_config(KBconf.config_file, 0)) {
		struct KBconfig FILEconf;

		wipe_config(&FILEconf);
		
		KB_stdlog("Reading config from file '%s'\n", KBconf.config_file);

		if ( read_file_config(&FILEconf, KBconf.config_file) )
		{
			KB_errlog("[config] Unable to read config file '%s'\n", KBconf.config_file); 
			return 1;
		}

		/* Apply it */
		apply_config(&KBconf, &FILEconf);
	}

	/* Apply command-line config lastly */
	apply_config(&KBconf, &CMDconf);

	/* Output final config to stdout */
	KB_stdlog("\n");
	report_config(&KBconf);
	KB_stdlog("\n");

	/* Verify data dir */
	if (test_directory(KBconf.data_dir, 0)) {
		KB_errlog("Unable to read '%s' directory\n", KBconf.data_dir);
		KB_errlog("Fatal Error: Can't start without a proper datadir\n");
		return 1;
	}

	/* Verify save dir */
	if (test_directory(KBconf.save_dir, 1)) {
		KB_errlog("Unable to read/write '%s' directory\n", KBconf.save_dir);
		KB_errlog("Fatal Error: Can't start without a proper savedir\n");
		return 1;
	}

	/* Play the game! */
	while (playing > 0)
		playing = run_game(&KBconf);

	if (!playing) KB_stdlog("Thank you for playing!\n");
	else KB_errlog("A Fatal Error has occurred, terminating!\n");

	return playing;
}
