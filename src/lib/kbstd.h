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
#ifndef HAVE_MALLOC
    #error "Your system doesn't have malloc() ..?!"
#else
    #ifdef HAVE_MALLOC_H
	#include <malloc.h>
    #else
	#include <memory.h>
    #endif
#endif


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

#define GROUP_SEP "#"
#define GROUP_SEP_SYM '#'

extern void KB_stdlog(char *fmt, ...);
extern void KB_errlog(char *fmt, ...);

extern void  KB_debuglog(int mod, char *fmt, ...); 

/*
 * Generic RND
 */
extern int KB_rand(int min, int max);

/*
 * Clear strlist
 */
extern int KB_strlist(char *list);

/*
 * Append value to strlist
 */
extern int KB_strlist_append(char *list, const char *value);
extern int KB_strlist_debug(const char *list);

/*
 * Return Nth word in an asciiz-list
 */
extern char* KB_strlist_ind(const char *list, int id);
extern char* KB_strlist_ind_dbg(const char *list, int id, const char *list_name, const char *filename, unsigned int line); /* debug version */
/*
 * Return a new asciiz-list made from provided list, but with an 'ind' word
 * replaced with 'new_value'. If 'freesrc' is true, original list is free()d.
 */
extern char* KB_strlist_replace(char *list, int ind, const char *new_value, int freesrc);
/*
 * Compact strlist into a string, using 'join' character as separator
 */
extern int KB_strlist_join(char *list, const char join);
/*
 * Match a string in an asciiz-list, return word_index+1 on success, 0 on failure
 */
extern int KB_strlistcmp(const char *list, const char *needle);
/* 
 * Return byte length of asciiz-list 
 */
extern int KB_strlist_len(const char *list);

#ifdef HAVE_STRCASECMP

#define KB_strcasecmp strcasecmp

#else

extern int KB_strcasecmp(const char *left, const char *right);

#endif 

extern char* KB_strtoupper(char *src);

#define KB_strlcat strlcat
#define KB_strlcpy strlcpy

extern void KB_strncat(char *dst, const char *src, unsigned int n);
extern void KB_strncpy(char *dst, const char *src, unsigned int n);

extern void KB_strncat_dbg(char *dst, const char *src, unsigned int n, const char *dst_name, const char *src_name, const char *filename, unsigned int line);
extern void KB_strncpy_dbg(char *dst, const char *src, unsigned int n, const char *dst_name, const char *src_name, const char *filename, unsigned int line);

#ifdef HAVE_STRDUP
#define KB_strdup strdup
#else
extern char* KB_strdup(const char *src);
#endif

#if 0

#define KB_strcpy(DST, SRC) KB_strncpy(DST, SRC, sizeof(DST))
#define KB_strcat(DST, SRC) KB_strncat(DST, SRC, sizeof(DST))
#define KB_strlist_peek(LST, ID) KB_strlist_ind(LST, ID)

#else

#define KB_strcpy(DST, SRC) KB_strncpy_dbg(DST, SRC, sizeof(DST), # DST, # SRC, __FILE__, __LINE__)
#define KB_strcat(DST, SRC) KB_strncat_dbg(DST, SRC, sizeof(DST), # DST, # SRC, __FILE__, __LINE__)
#define KB_strlist_peek(LST, ID) KB_strlist_ind_dbg(LST, ID, # LST, __FILE__, __LINE__)

#endif

extern char *KB_makepath(int *len, ...); /* Allocate new string and concat varargs into it, len is resulting sizeof() */

#define KB_fastpath(ARGS...) KB_makepath(NULL, ARGS, NULL);

extern void KB_grpnsep(char *dst, unsigned int n);
extern void KB_dirnsep(char *dst, unsigned int n);
extern void KB_dirncpy(char *dst, const char *src, unsigned int n);

#define KB_grpsep(DST) KB_grpnsep(DST, sizeof(DST))
#define KB_dirsep(DST) KB_dirnsep(DST, sizeof(DST))
#define KB_dircpy(DST, SRC) KB_dirncpy(DST, SRC, sizeof(DST))


extern int KB_mkdir(const char *dir, int mode);
extern int hex2dec(const char *hex_str);
extern int file_size(const char *filename);
extern int test_directory(const char *path, int make); 

extern void name_nsplit(const char *name, char *base, int base_max, char *ext, int ext_man);
#define name_split(NAME, BASE, EXT) name_nsplit(NAME, BASE, sizeof(BASE), EXT, sizeof(EXT));

#endif /* _OPENKB_LIBKB_STD */
