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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().

#define DEFAULT_DATA_DIR "/usr/local/games/openkb-data/"
#define DEFAULT_SAVE_DIR "/var/games/openkb-saves/"
#define DEFAULT_CONF_DIR "/usr/local/etc/"

#define SAVE_BASE_DIR "saves/"
#define DATA_BASE_DIR ""

#define CONFIG_BASE_DIR "/.openkb/"
#define CONFIG_INI_NAME "openkb.ini"

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
	fprintf(f, "[sdl]\n");
	fprintf(f, "fullscreen = 0\n");	

	fclose(f);
	
	fprintf(stdout, "Created file '%s'\n", path);
	return 0;
}

int read_file_config(struct KBconfig *conf, const char *path) {

	FILE *f = fopen(path, "r");
	
	if (f == NULL) return -1;
	
	char buf[1024];
	char buf1[1024];
	char buf2[1024];
 	while (fgets(buf, sizeof buf, f) != NULL)
 	{
		
		if (buf[0] == ';') continue;
		if (buf[0] == '[') continue;
	
		if (sscanf(buf, "%s = %s", buf1, buf2) == 2) {
			if (!strcasecmp(buf1, "fullscreen")) {
				conf->fullscreen = atoi(buf2);
				conf->set[5] = 1;
			} else 
			if (!strcasecmp(buf1, "datadir")) {
				strcpy(conf->data_dir, buf2);
				conf->set[3] = 1;
			} else 
			if (!strcasecmp(buf1, "savedir")) {
				strcpy(conf->save_dir, buf2);
				conf->set[2] = 1;
			}
		}
	}	
	
	fclose(f);
	return 0;
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

int test_directory(const char *path, int make) {
	struct stat status;
	if (!stat(path, &status))
	{
    	if (status.st_mode & S_IFDIR)
	    {
			return 0;
    	}
	}
	if (!make) return 1;
	if (mkdir(path, 0755))
	{
		fprintf(stderr, "Unable to create directory '%s'%s\n", path, (errno == EACCES) ? ", permission denied" : "");
		return -1;
	}
	return 0;
}

int read_cmd_config(struct KBconfig *conf, int argc, char *args[]) {

	if (argc > 1) {
		int i;
		for (i = 1; i < argc; i++) {
		
			if (!strcasecmp(args[i], "--fullscreen")) {
				conf->fullscreen = 1;
				conf->set[5] = 1;
				continue;
			}		

			if (i == argc - 1) {
				strcpy(conf->config_file, args[i]);
				continue;
			}

			fprintf(stderr, "Unrecognized argument '%s'\n", args[i]);
		}
	}

	return 0;
}

void wipe_config(struct KBconfig *conf) {
	conf->config_file[0] = '\0';
	conf->set[0] = 0;
	
	conf->config_dir[0] = '\0';
	conf->set[1] = 0;
	
	conf->save_dir[0] = '\0';
	conf->set[2] = 0;
	
	conf->data_dir[0] = '\0';
	conf->set[3] = 0;

	conf->fullscreen = 0;
	conf->set[5] = 0;
	
	conf->mode = 0;
	conf->set[6] = 0;
}

void read_env_config(struct KBconfig *conf) {

	char *pPath;

	pPath = getenv ("HOME");
	strcat(conf->config_dir, pPath);
	strcat(conf->config_dir, CONFIG_BASE_DIR);

	strcat(conf->config_file, conf->config_dir);
	strcat(conf->config_file, CONFIG_INI_NAME);

	//printf("Config ROOT: %s[%s]\n", KBconf.config_dir, CONFIG_INI_NAME);

	strcat(conf->save_dir, pPath);
	strcat(conf->save_dir, CONFIG_BASE_DIR);
	strcat(conf->save_dir, SAVE_BASE_DIR);

	//printf("Save DIR: %s\n", KBconf.save_dir);

	strcat(conf->data_dir, pPath);
	strcat(conf->data_dir, CONFIG_BASE_DIR);
	strcat(conf->data_dir, DATA_BASE_DIR);

	//printf("Data DIR: %s\n", KBconf.data_dir);
}

void report_config(struct KBconfig *conf) {

	fprintf(stdout, "Data dir: %s\n", conf->data_dir);
	fprintf(stdout, "Save dir: %s\n", conf->save_dir);

	fprintf(stdout, "Fullscreen: %s\n", conf->fullscreen ? "Yes" : "No");

}

int find_config(struct KBconfig *conf) {

	char config_dir[FULLPATH_LEN];
	char config_file[FULLPATH_LEN];
	char *pPath;

	/* Try the local directory */
	strcpy(config_file, "./");//TODO: proper local name of *binary*
	strcat(config_file, CONFIG_INI_NAME);
	fprintf(stdout, "Looking for config at '%s'\n", config_file);
	if (!test_config(config_file, 0))
	{
		strcpy(conf->config_file, config_file);
		return 0;
	}

	/* Try HOME directory */
	pPath = getenv("HOME");//NULL?
	strcpy(config_dir, pPath);
	strcat(config_dir, CONFIG_BASE_DIR);
	strcpy(config_file, config_dir);
	strcat(config_file, CONFIG_INI_NAME);
	fprintf(stdout, "Looking for config at '%s'\n", config_file);

	if (!test_directory(config_dir, 1)) {	
		if (!test_config(config_file, 1))
		{
			strcpy(conf->config_file, config_file);
			return 0;
		}
	}

	/* Try SYSTEM directory */
	strcpy(config_file, DEFAULT_CONF_DIR);
	strcat(config_file, CONFIG_INI_NAME);		
	fprintf(stdout, "Looking for config at '%s'\n", config_file);
	if (!test_config(config_file, 0))
	{
		strcpy(conf->config_file, config_file);
		return 0;
	}
	
	return 1;
}

void apply_config(struct KBconfig* dst, struct KBconfig* src) {

	if (src->set[2]) strcpy(dst->save_dir, src->save_dir);
	if (src->set[3]) strcpy(dst->data_dir, src->data_dir);

	if (src->set[5]) dst->fullscreen = src->fullscreen;

}

