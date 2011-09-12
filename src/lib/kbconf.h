/*
 *  kbconf.h -- A "config" abstraction 
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

#ifndef _OPENKB_LIBKB_CONFIG
#define _OPENKB_LIBKB_CONFIG

#define PATH_LEN 4096
#define FILE_LEN 1024

#define FULLPATH_LEN 5000 

typedef struct KBconfig {

	char config_file[FULLPATH_LEN];
	char config_dir[PATH_LEN];
	char save_dir[PATH_LEN];
	char data_dir[PATH_LEN];

	int fullscreen;
	int mode;

	int set[16];//what is SET
} KBconfig;

#endif /* _OPENKB_LIBKB_CONFIG */
