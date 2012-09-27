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

#include "kbstd.h"
#include "kbdir.h"

#define MAX_MODULES	16
#define MAX_SLOTS	4

#define KBFAMILY_GNU	0x0F
#define KBFAMILY_DOS	0x0D
#define KBFAMILY_MD 	0x02

typedef struct KBmodule {

	char name[255];

	char slotA_name[1024];
	char slotB_name[1024];
	char slotC_name[1024];

	KB_DIR *slotA;
	KB_DIR *slotB;
	KB_DIR *slotC;

	int bpp;
	
	int kb_family;
	
	void *cache;

} KBmodule;

typedef void*	(*KBresolve_cb)(KBmodule *mod, int id, int sub_id);

#define C_config_file  0
#define C_config_dir   1
#define C_save_dir     2
#define C_data_dir     3

#define C_fullscreen   4
#define C_filter       5
#define C_module       6
#define C_autodiscover 7
#define C_fallback     8
#define C_sound        9

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
	int sound;

	int set[16];//what is SET

	KBmodule modules[MAX_MODULES];
	int num_modules;

} KBconfig;

extern void wipe_config(struct KBconfig *conf);
extern int read_env_config(struct KBconfig *conf);
extern int read_cmd_config(struct KBconfig *conf, int argc, char *args[]);
extern int read_file_config(struct KBconfig *conf, const char *path);
extern int test_config(const char *path, int make);
extern int find_config(struct KBconfig *conf);
extern void apply_config(struct KBconfig* dst, struct KBconfig* src);
extern void report_config(struct KBconfig *conf);
		
#endif /* _OPENKB_LIBKB_CONFIG */
