/*
 *  kbconf.c -- run-time configuration routines
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
#include "kbconf.h"
#include "kbstd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_DATA_DIR "/usr/local/games/openkb-data/"
#define DEFAULT_SAVE_DIR "/var/games/openkb-saves/"
#define DEFAULT_CONF_DIR "/usr/local/etc/"

#define SAVE_BASE_DIR "saves/"
#define DATA_BASE_DIR ""

#define CONFIG_BASE_DIR "/.openkb/"
#define CONFIG_INI_NAME "openkb.ini"

extern void register_module(KBconfig *conf, KBmodule *mod);
extern void wipe_module(KBmodule *mod);

/* Test file by attempting a read operation */
int test_file(const char *path) {
	FILE *f = fopen(path, "r");
	if (f == NULL) return -1;
	fclose(f);
	return 0;
}

int write_default_config(const char *path) {

	FILE *f = fopen(path, "w");
	
	if (f == NULL) return -1;

	fprintf(f, "; openkb config file. lines starting with ';' are comments.\n");
	fprintf(f, "[main]\n");
	fprintf(f, ";savedir = \n");
	fprintf(f, ";datadir = \n");
	fprintf(f, "autodiscover = 1\n");
	fprintf(f, "[sdl]\n");
	fprintf(f, "sound = 0\n");
	fprintf(f, "fullscreen = 0\n");
	fprintf(f, "filter = normal2x\n");
	fprintf(f, ";[module]\n");
	fprintf(f, ";For open modules, specify the path to the INI and PNG files,\n");
	fprintf(f, ";and set 'free' as type.\n");
	fprintf(f, ";name = Free\n");
	fprintf(f, ";type = free\n");
	fprintf(f, ";path = free/\n");
	fprintf(f, ";[module]\n");
	fprintf(f, ";For DOS module, specify the path to the EXE and CC files,\n");
	fprintf(f, ";and pick from 'dos', 'dos-ega', 'dos-cga' or 'dos-mono'\n");
	fprintf(f, ";for type.\n");
	fprintf(f, ";name = DOS\n");
	fprintf(f, ";type = dos\n");
	fprintf(f, ";path = dos/\n");

	fclose(f);
	
	KB_stdlog("Created file '%s'\n", path);
	return 0;
}

int read_file_config(struct KBconfig *conf, const char *path) {

	FILE *f = fopen(path, "r");
	
	int err = 0;
	int lineno = -1;	

	if (f == NULL) return -1;

	int mN = -1;
	int slot = 0;

	char buf[1024];
	char buf1[1024];
	char buf2[1024];
 	while (fgets(buf, sizeof buf, f) != NULL)
 	{
		lineno++;
		if (buf[0] == ';') continue;
		if (buf[0] == '[') continue;
	
		if (sscanf(buf, "%s = %s", buf1, buf2) == 2) {
			if (!KB_strcasecmp(buf1, "fullscreen")) {
				conf->fullscreen = atoi(buf2);
				conf->set[C_fullscreen] = 1;
			} else 
			if (!KB_strcasecmp(buf1, "sound")) {
				conf->sound = atoi(buf2);
				conf->set[C_sound] = 1;
			} else
			if (!KB_strcasecmp(buf1, "datadir")) {
				KB_dircpy(conf->data_dir, buf2);
				conf->set[C_data_dir] = 1;
			} else 
			if (!KB_strcasecmp(buf1, "savedir")) {
				KB_dircpy(conf->save_dir, buf2);
				conf->set[C_save_dir] = 1;
			} else
			if (!KB_strcasecmp(buf1, "autodiscover")) {
				conf->autodiscover = atoi(buf2);
				conf->set[C_autodiscover] = 1;
			} else
			if (!KB_strcasecmp(buf1, "fallback")) {
				conf->fallback = atoi(buf2);
				conf->set[C_fallback] = 1;
			} else
			if (!KB_strcasecmp(buf1, "filter")) {
				switch(KB_strlistcmp(
					"none\0" "normal2x\0"//1, 2
					"scale2x\0" //3
					"\0", buf2)) {
					case 1://none
						conf->filter = 0;
						conf->set[C_filter] = 1;
					break;
					case 2://normal2x
						conf->filter = 1;
						conf->set[C_filter] = 1;
					break;
					case 3://scale2x
						conf->filter = 2;
						conf->set[C_filter] = 1;
					break;
					default:
						err = -1;					
					break;
				}
				if (err == -1) {
					KB_errlog("[config] Unrecognized filter '%s' on line %d, accepted filters are: none, normal2x, scale2x \n", &buf2[0], lineno);
					err = -1;
					break;
				}			
			} else
			if (!KB_strcasecmp(buf1, "name")) {
				mN++;
				slot = 0;
				KB_strcpy(conf->modules[mN].name, buf2);
			} else
			if (!KB_strcasecmp(buf1, "type")) {
				switch(KB_strlistcmp(
					"free\0" "open\0"//1, 2
					"dos\0" "dos-vga\0"//3, 4
					"dos-ega\0"//5
					"dos-cga\0"//6
					"dos-mono\0" "dos-herc\0"//7, 8
					"md\0" "sega\0"//9,10
					"\0", buf2)) {
					case 1://free
					case 2://open
						conf->modules[mN].kb_family = KBFAMILY_GNU;
						conf->modules[mN].bpp = 0; /* png? */
					break;
					case 3://dos
					case 4://dos-vga
						conf->modules[mN].kb_family = KBFAMILY_DOS;
						conf->modules[mN].bpp = 8; /* VGA */
					break;
					case 5://dos-ega
						conf->modules[mN].kb_family = KBFAMILY_DOS;
						conf->modules[mN].bpp = 4; /* EGA */
					break;
					case 6://dos-cga
						conf->modules[mN].kb_family = KBFAMILY_DOS;
						conf->modules[mN].bpp = 2; /* CGA */
					break;
					case 7://dos-mono
					case 8://dos-herc
						conf->modules[mN].kb_family = KBFAMILY_DOS;
						conf->modules[mN].bpp = 1; /* Hercules */					
					break;
					case 9://md
						conf->modules[mN].kb_family = KBFAMILY_MD;
						conf->modules[mN].bpp = 4; /* default? */
					break;					
					case 0:
					default:
						err = -1;					
					break;
				}
				if (err == -1) {
					KB_errlog("[config] Unrecognized module type '%s' on line %d, accepted types are: auto, free, dos-mono, dos-cga, dos-ega, dos, md, ami, c64, aii \n", &buf2[0], lineno);
					err = -1;
					break;
				}
			} else
			if (!KB_strcasecmp(buf1, "path")) {
				char *buf_ptr[3] = {
					conf->modules[mN].slotA_name,
					conf->modules[mN].slotB_name,
					conf->modules[mN].slotC_name,
				};
				if (slot >= MAX_SLOTS) {
					KB_errlog("[config] Too many paths '%s' on line %d \n", &buf[0], lineno);
					err = -1;
					break;
				}
				if (buf2[0] != '/')
				{
					KB_dirncpy(buf_ptr[slot], conf->data_dir, sizeof(conf->modules[mN].slotA_name));
					KB_dirnsep(buf_ptr[slot], sizeof(conf->modules[mN].slotA_name));
				}
				else 
					buf_ptr[slot][0] = 0;
				KB_strncat(buf_ptr[slot], buf2, sizeof(conf->modules[mN].slotA_name));
				slot++;
			} else
			{
				KB_errlog("[config] Undefined value '%s' on line %d\n", &buf1[0], lineno);
				err = -1;
				break;
			}
		} else {
			KB_errlog("[config] Can't parse '%s' on line %d\n", &buf[0], lineno);
			err = -1;
			break;
		}
	}

	conf->num_modules = mN + 1;

	fclose(f);
	return err;
}

int test_config(const char *path, int make) {
	if (test_file(path)) 
	{
		if (make)
		{ 
			return write_default_config(path);
		}
		return -1;
	}
	return 0; 
}

int read_cmd_config(struct KBconfig *conf, int argc, char *args[]) {

	if (argc > 1) {
		int i;
		for (i = 1; i < argc; i++) {

			if (!KB_strcasecmp(args[i], "--fullscreen")) {
				conf->fullscreen = 1;
				conf->set[C_fullscreen] = 1;
				continue;
			}

			if (!KB_strcasecmp(args[i], "--nosound")) {
				conf->sound = 0;
				conf->set[C_sound] = 1;
				continue;
			}

			if (i == argc - 1) {
				KB_strcpy(conf->config_file, args[i]);
				continue;
			}

			KB_errlog("Unrecognized argument '%s'\n", args[i]);
		}
	}

	return 0;
}

void wipe_config(struct KBconfig *conf) {
	conf->num_modules = 0;

	conf->config_file[0] = '\0';
	conf->set[C_config_file] = 0;

	conf->config_dir[0] = '\0';
	conf->set[C_config_dir] = 0;

	conf->save_dir[0] = '\0';
	conf->set[C_save_dir] = 0;

	conf->data_dir[0] = '\0';
	conf->set[C_data_dir] = 0;

	conf->fullscreen = 0;
	conf->set[C_fullscreen] = 0;

	conf->sound = 0;
	conf->set[C_sound] = 0;

	conf->filter = 0;
	conf->set[C_filter] = 0;

	conf->module = 0;
	conf->set[C_module] = 0;

	conf->autodiscover = 0;
	conf->set[C_autodiscover] = 0;

	conf->fallback = 0;
	conf->set[C_fallback] = 0;

	int i;
	for (i = 0; i < MAX_MODULES; i++)
		wipe_module(&conf->modules[i]);
}

int read_env_config(struct KBconfig *conf) {

	char *pPath;
#ifdef USE_WINAPI
	pPath = (char*)getenv("USERPROFILE");
#else
	pPath = (char*)getenv("HOME");
#endif
	if (pPath == NULL) return -1;

	KB_strcpy(conf->config_dir, pPath);
	KB_strcat(conf->config_dir, CONFIG_BASE_DIR);

	KB_strcpy(conf->config_file, conf->config_dir);
	KB_strcat(conf->config_file, CONFIG_INI_NAME);

	//printf("Config ROOT: %s[%s]\n", KBconf.config_dir, CONFIG_INI_NAME);

	KB_strcpy(conf->save_dir, pPath);
	KB_strcat(conf->save_dir, CONFIG_BASE_DIR);
	KB_strcat(conf->save_dir, SAVE_BASE_DIR);

	//printf("Save DIR: %s\n", KBconf.save_dir);

	KB_strcpy(conf->data_dir, pPath);
	KB_strcat(conf->data_dir, CONFIG_BASE_DIR);
	KB_strcat(conf->data_dir, DATA_BASE_DIR);

	//printf("Data DIR: %s\n", KBconf.data_dir);
	return 0;
}

void report_config(struct KBconfig *conf) {
	int i;
	KB_debuglog(0, "Data dir:\t %s\n", conf->data_dir);
	KB_debuglog(0, "Save dir:\t %s\n", conf->save_dir);

	KB_debuglog(0, "Enable Sound:\t %s\n", conf->sound ? "Yes" : "No");
	KB_debuglog(0, "Fullscreen:\t %s\n", conf->fullscreen ? "Yes" : "No");
	KB_debuglog(0, "Zoom Filter:\t %s\n", (conf->filter == 1 ? "Normal2x" : conf->filter ? "Scale2x" : "None" ));
	KB_debuglog(0, "Auto-discover modules:\t %s\n", conf->autodiscover ? "Yes" : "No");

	for (i = 0; i < conf->num_modules; i++) {
		KB_debuglog(1, "Module: %s\n", conf->modules[i].name);
		KB_debuglog(1, "SlotA: %s\n", conf->modules[i].slotA_name);
		KB_debuglog(0, "SlotB: %s\n", conf->modules[i].slotB_name);
		KB_debuglog(0, "SlotC: %s\n", conf->modules[i].slotC_name);
		KB_debuglog(-2, "", NULL);
	}
}

int find_config(struct KBconfig *conf) {

	char config_dir[PATH_LEN];
	char config_file[PATH_LEN];
	char *pPath;

	/* Try the local directory */
								//TODO: proper local name of *binary*
	pPath = (char*)getcwd(config_file, sizeof(config_file));//NULL?
	KB_strcpy(config_file, pPath);
	KB_dirsep(config_file);
	KB_strcat(config_file, CONFIG_INI_NAME);
	KB_stdlog("Looking for config at '%s'\n", config_file);
	if (!test_config(config_file, 0))
	{
		KB_strcpy(conf->config_file, config_file);
		return 0;
	}

	/* Try HOME directory */
	pPath = (char*)getenv("HOME");//NULL?
	KB_strcpy(config_dir, pPath);
	KB_strcat(config_dir, CONFIG_BASE_DIR);
	KB_strcpy(config_file, config_dir);
	KB_strcat(config_file, CONFIG_INI_NAME);
	KB_stdlog("Looking for config at '%s'\n", config_file);

	if (!test_directory(config_dir, 1)) {	
		if (!test_config(config_file, 1))
		{
			KB_strcpy(conf->config_file, config_file);
			return 0;
		}
	}

	/* Try SYSTEM directory */
	KB_strcpy(config_file, DEFAULT_CONF_DIR);
	KB_strcat(config_file, CONFIG_INI_NAME);		
	KB_stdlog("Looking for config at '%s'\n", config_file);
	if (!test_config(config_file, 0))
	{
		strcpy(conf->config_file, config_file);
		return 0;
	}

	return 1;
}

void apply_config(struct KBconfig* dst, struct KBconfig* src) {

	if (src->set[C_save_dir]) KB_strcpy(dst->save_dir, src->save_dir);
	if (src->set[C_data_dir]) KB_strcpy(dst->data_dir, src->data_dir);

	if (src->set[C_fullscreen]) dst->fullscreen = src->fullscreen;
	if (src->set[C_filter]) dst->filter = src->filter;
	
	if (src->set[C_sound]) dst->sound = src->sound;
	
	if (src->set[C_autodiscover]) dst->autodiscover = src->autodiscover;
	if (src->set[C_fallback]) dst->fallback = src->fallback;

	int i;
	for (i = 0; i < src->num_modules; i++) {
		register_module(dst, &src->modules[i]);
	}
}

