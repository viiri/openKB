/*
 *  kbstd.h -- stdlib-like functionality.
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
#ifndef _OPENKB_LIBKB_STD
#define _OPENKB_LIBKB_STD

#include "../config.h"
#include <malloc.h>

#ifdef PATH_MAX
#define PATH_LEN PATH_MAX
#else
#define PATH_LEN 4096
#endif

#ifdef FILE_MAX
#define FILE_LEN FILE_MAX
#else
#define FILE_LEN 1024
#endif

#ifdef USE_WINAPI
#define PATH_SEP "\\"
#define PATH_SEP_SYM '\\'
#else
#define PATH_SEP "/"
#define PATH_SEP_SYM '/'
#endif

extern void KB_stdlog(char *fmt, ...);
extern void KB_errlog(char *fmt, ...);

extern void  KB_debuglog(int mod, char *fmt, ...); 

/*
 * Match a name in an asciz-array, return word_index+1 on success, 0 on failure
 */
extern int KB_strlistcmp(const char *list, const char *needle);

#ifdef HAVE_STRCASECMP

#define KB_strcasecmp strcasecmp

#else

extern int KB_strcasecmp(const char *left, const char *right);

#endif 

#define KB_strlcat strlcat
#define KB_strlcpy strlcpy

extern void KB_strncat(char *dst, const char *src, unsigned int n);
extern void KB_strncpy(char *dst, const char *src, unsigned int n);

extern void KB_strncat_dbg(char *dst, const char *src, unsigned int n, const char *dst_name, const char *src_name, const char *filename, unsigned int line);
extern void KB_strncpy_dbg(char *dst, const char *src, unsigned int n, const char *dst_name, const char *src_name, const char *filename, unsigned int line);

#if 0

#define KB_strcpy(DST, SRC) KB_strncpy(DST, SRC, sizeof(DST))
#define KB_strcat(DST, SRC) KB_strncat(DST, SRC, sizeof(DST))

#else

#define KB_strcpy(DST, SRC) KB_strncpy_dbg(DST, SRC, sizeof(DST), # DST, # SRC, __FILE__, __LINE__)
#define KB_strcat(DST, SRC) KB_strncat_dbg(DST, SRC, sizeof(DST), # DST, # SRC, __FILE__, __LINE__)

#endif

extern void KB_dirnsep(char *dst, unsigned int n);
extern void KB_dirncpy(char *dst, const char *src, unsigned int n);

#define KB_dirsep(DST) KB_dirnsep(DST, sizeof(DST))
#define KB_dircpy(DST, SRC) KB_dirncpy(DST, SRC, sizeof(DST))


extern int file_size(const char *filename);
extern int test_directory(const char *path, int make); 

#endif /* _OPENKB_LIBKB_STD */
