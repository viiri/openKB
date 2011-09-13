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

extern void KB_stdlog(char *fmt, ...);
extern void KB_errlog(char *fmt, ...);

extern void  KB_debuglog(int mod, char *fmt, va_list argptr); 

/*
 * Match a name in an asciz-array, return word_index+1 on success, 0 on failure
 */
extern int KB_strlistcmp(const char *list, const char *needle);

#ifdef HAVE_STRCASECMP

#define KB_strcasecmp strcasecmp

#else

extern int KB_strcasecmp(const char *left, const char *right);

#endif 

#ifdef HAVE_STRLCAT

#define KB_strcat strlcat
#define KB_strncat strlncat

#else

extern void KB_strcat(char *dst, const char *src);
extern void KB_strncat(char *dst, const char *src, unsigned int n);

#endif

#endif /* _OPENKB_LIBKB_STD */
