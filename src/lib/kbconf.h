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

#define C_config_file  0
#define C_config_dir   1
#define C_save_dir     2
#define C_data_dir     3

#define C_fullscreen   4
#define C_filter       5
#define C_module       6
#define C_autodiscover 7
#define C_fallback     8

#include "kbstd.h"
#include "kbauto.h"

typedef struct KBconfig {

	char config_file[PATH_LEN];
	char config_dir[PATH_LEN];
	char save_dir[PATH_LEN];
	char data_dir[PATH_LEN];

	int fullscreen;
	int filter;
	int module;
	int autodiscover;
	int fallback;

	int set[16];//what is SET

	KBmodule modules[MAX_MODULES];
	int num_modules;

} KBconfig;

extern void discover_modules(const char *path, KBconfig *conf);

#endif /* _OPENKB_LIBKB_CONFIG */
