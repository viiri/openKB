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
#define VERSION "0.0.1"
#include "lib/kbconf.h"

#include "string.h"

#include "stdio.h"

struct KBconfig KBconf;

extern int run_game(KBconfig *conf);

int main(int argc, char* argv[]) {

	int playing = 1;	/* Play 1 game of KB */

	/* Lots of very boring things must happen for a proper initialisation... */

	/* Let's start by searching for a config file,
	 * then see if there're "data" and "save" directories, and finally
	 * if we could load any usable data from "data" */

	/* All this strcat code reeks of error, but please bear with it */
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
			fprintf(stderr, "Unable to read config file '%s'\n", CMDconf.config_file); 
			exit(1);
		}
	}
	/* Search for a config file */
	else
	{
		find_config(&KBconf);
	}

	/* Read found/requested config file */
	if (!test_config(KBconf.config_file, 0)) {
		struct KBconfig FILEconf;

		fprintf(stdout, "Reading config from file '%s'\n", KBconf.config_file);

		read_file_config(&FILEconf, KBconf.config_file);

		/* Apply it */
		apply_config(&KBconf, &FILEconf);
	}

	/* Apply command-line config lastly */
	apply_config(&KBconf, &CMDconf);

	/* Output final config to stdout */
	fprintf(stdout, "\n");
	report_config(&KBconf);
	fprintf(stdout, "\n");

	/* Verify data dir */
	if (test_directory(KBconf.data_dir, 0)) {
		fprintf(stderr, "Unable to read '%s' directory\n", KBconf.data_dir);
		exit(1);  
	}

	/* Verify save dir */
	if (test_directory(KBconf.save_dir, 1)) {
		fprintf(stderr, "Unable to read/write '%s' directory\n", KBconf.save_dir);
		exit(1);  
	}

	/* Play the game! */
	while (playing > 0)
		playing = run_game(&KBconf);

	if (!playing) fprintf(stdout, "Thank you for playing!\n");
	else fprintf(stderr, "A Fatal Error has occured, terminating!\n");

	return playing;
}
