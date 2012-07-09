/*
 *  kbstd.c -- stdlib-like functionality.
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>

#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().

#ifdef USE_WINAPI
#include <windows.h>
#endif

#include "kbstd.h"

void KB_debuglog(int mod, char *fmt, ...) 
{
	static int level = 0;
	int i;

	va_list argptr;
	va_start(argptr, fmt);

	level += mod;
	for (i = 0; i < level; i++) fputc(' ', stdout); 
	vfprintf(stdout, fmt, argptr);

	va_end(argptr);
}


void  KB_stdlog(char *fmt, ...) 
{ 
	va_list argptr;
	va_start(argptr, fmt);
	vfprintf(stdout, fmt, argptr);
	va_end(argptr);
}

void  KB_errlog(char *fmt, ...) 
{
#ifdef USE_WINAPI
	char buffer[1024];
#endif
	va_list argptr;
	va_start(argptr, fmt);
#ifdef USE_WINAPI
	vsprintf(buffer, fmt, argptr);
	MessageBox(NULL, buffer, "Critical Error", MB_ICONEXCLAMATION);
#endif
	vfprintf(stderr, fmt, argptr);
	va_end(argptr);
}

int KB_strlist_max(const char *list)
{
	int ind = 0, i = 0, n = sizeof(list);
	while (i < n) {
		if (list[i++] == '\0') {
			ind++;
			if (i < n && list[i++] == '\0') break;
		}
	}
	return ind;
}

char* KB_strlist_ind(const char *list, int id)
{
	const char *match = list;
	int w = 0;
	while (1) {
		if (*list == '\0') {
			if (w == id) break;
			w++;
			list++;
			match = NULL;
			if (*list == '\0') break;
			match = list;
			continue;
		}
		list++;				
	}
	return (char*)match;
}

int KB_strlistcmp(const char *list, const char *needle)
{
	int i = 0, j = 0;
	int w = 0;

	while (needle[j] != '\0') {
		/* Mismatch */
		if (list[i] != needle[j]) {
			/* Goto end of word */
			while(list[i] != '\0') i++;
			if (list[++i] == '\0') return 0;
			/* Reset needle */
			j = 0;
			w++;
			continue;
		}

		i++;
		j++;
	}

	return w + 1;
}

void KB_strlistcpy(char *dst, ...) {

}

char *vmakepath(int *_len, va_list arglist) {

	int arg_len, len;
	int used, step = FILE_LEN;

	char *str, *input, *arg;

	len = PATH_LEN;
	used = 0;
	str = malloc(sizeof(char) * len);
	if (str == NULL) { /* Out of memory */
		*_len = 0;
		return NULL;
	}

	str[0] = '\0';
	while (input = va_arg(arglist, char*)) {

		arg = input;

		/* Hack -- apply OS-specific directory separator instead of '/' */
		if (input[0] == '/' && input[1] == '\0') {
			if (str[used - 1] == PATH_SEP_SYM || str[used - 1] == '#') continue;
			arg = PATH_SEP;
		}

		arg_len = strlen(arg);
		if (arg_len + used + 1 > len) {

			len += step;
			step *= 2;
			str = realloc(str, sizeof(char) * len);
			if (str == NULL) { /* Out of memory */
				len = 0;
				break;
			}
		}

		strlcat(str, arg, len);
		used += arg_len;
	}

	if (_len) *_len = len;
	return str;
}

char *makepath(int *len, ...) {
	char *path;
	va_list argptr;
	va_start(argptr, len);
	path = vmakepath(len, argptr);
	va_end(argptr);
	return path;
}

char *KB_makepath(int *_len, ...) {

	char *path;
	int len;

	va_list argptr;
	va_start(argptr, _len);

	path = vmakepath(&len, argptr);
	if (path == NULL)
		KB_errlog("[makepath] Can't allocate %d bytes for path string\n", len);

	va_end(argptr);
	if (_len) *_len = len;
	return path;
}

void KB_strncat(char *dst, const char *src, unsigned int n) {
	if (strlcat(dst, src, n) >= n)
		KB_errlog("[strlcat] Can't append '%s' to '%s', %d limit reached\n", src, dst, n);
}

void KB_strncpy(char *dst, const char *src, unsigned int n) {
	if (strlcpy(dst, src, n) >= n)
		KB_errlog("[strlcpy] Can't copy '%s' into %d-sized buffer\n", src, n);
}

void KB_strncat_dbg(char *dst, const char *src, unsigned int n, const char *dst_name, const char *src_name, const char *filename, unsigned int line) {
	if (strlcat(dst, src, n) >= n)
		KB_errlog("[strlcat] Can't append '%s' \"%s\" to '%s' \"%s\", %d-byte limit reached; %s:%d\n", src_name, src, dst_name, dst, n, filename, line);
}

void KB_strncpy_dbg(char *dst, const char *src, unsigned int n, const char *dst_name, const char *src_name, const char *filename, unsigned int line) {
	if (strlcpy(dst, src, n) >= n)
		KB_errlog("[strlcpy] Can't copy '%s' \"%s\" into %d-sized buffer '%s'; %s:%d\n", src_name, src, n, dst_name, filename, line);
}

void KB_grpnsep(char *dst, unsigned int n) {
	int l = strlen(dst);
	if (dst[l - 1] == PATH_SEP_SYM || dst[l - 1] == GROUP_SEP_SYM) return;
	KB_strncat(dst, GROUP_SEP, n);
}

void KB_dirnsep(char *dst, unsigned int n) {
	int l = strlen(dst);
	if (dst[l - 1] == PATH_SEP_SYM || dst[l - 1] == GROUP_SEP_SYM) return;
	KB_strncat(dst, PATH_SEP, n);
}

void KB_dirncpy(char *dst, const char *src, unsigned int n) {
	int k, l = strlen(src);
	dst[0] = '\0';
	k = strlcpy(dst, src, n);
	if (l && src[l - 1] == PATH_SEP_SYM) dst[k - 1] = '\0';
}

int file_size(const char *filename) { 
	struct stat status; 
	if (stat(filename, &status)) return 0; 
	return status.st_size; 
}

int test_directory(const char *path, int make) {
	struct stat status;
	if (!stat(path, &status))
	{
		if (status.st_mode & S_IFDIR) return 0;
	}
	if (!make) return 1;
	if (mkdir(path, 0755))
	{
		KB_errlog("Unable to create directory '%s'%s\n", path, (errno == EACCES) ? ", permission denied" : "");
		return -1;
	}
	return 0;
}

int hex2dec(const char *hex_str) {
	if (isxdigit(hex_str[0]))
		return (int)strtol(hex_str, NULL, 16);
	else
		return 0;
}

void name_nsplit(const char *name, char *base, int base_max, char *ext, int ext_max) {
	int i = 0;
	int l = strlen(name);
	int dot = -1;

	for (i = l - 1; i > -1; i--) {
		if (name[i] == '.') {
			dot = i;
			break;
		}
	}

	if (dot == -1) {
		KB_strncpy(base, name, base_max);
		KB_strncpy(ext, "", ext_max);
	} else if (dot == 0) {
		KB_strncpy(base, "", base_max);
		KB_strncpy(ext, name + 1, ext_max);
	} else {
		KB_strncpy(base, name, base_max); base[dot] = '\0';
		KB_strncpy(ext, name + dot + 1, ext_max);
	}
}
