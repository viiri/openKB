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

void  KB_debuglog(int mod, char *fmt, va_list argptr) 
{
	static int level = 0;
	int i;
	level += mod;
	for (i = 0; i < level; i++) fprintf(stdout, " "); 
	fprintf(stdout, fmt, argptr);
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
	va_list argptr;
	va_start(argptr, fmt);
	vfprintf(stderr, fmt, argptr);
	va_end(argptr);
}

int KB_strlistcmp(const char *list, const char *needle)
{
	int i = 0, j = 0;
	int w = 0;
	
	int nervous = 0;

	int need = 0;
	int match = 0;	

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
