/*
 *  kbsys.h -- basic datatypes and byte-level operations
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
#ifndef _OPENKB_LIBKB_SYS
#define _OPENKB_LIBKB_SYS

#include "../config.h"

#ifdef HAVE_LIBSDL

#include "SDL/SDL.h"

#define KB_BYTE_ORDER SDL_BYTEORDER
#define KB_LIL_ENDIAN SDL_LIL_ENDIAN
#define KB_BIG_ENDIAN SDL_BIG_ENDIAN

typedef Uint32  	sword ; //sesqui-word (24 bit)
typedef Uint32  	dword ;
typedef Uint16  	word  ;
typedef Uint8   	byte  ;

#else

#include "endian.h" 
#include "stdint.h"

#define KB_BYTE_ORDER __BYTE_ORDER
#define KB_LIL_ENDIAN __LITTLE_ENDIAN
#define KB_BIG_ENDIAN __BIG_ENDIAN

typedef uint32_t	dword ;
typedef uint32_t  	sword ; //sesqui-word (24 bit)
typedef uint16_t	word  ;
typedef uint8_t 	byte  ;

#endif

#define SHIFT_LEFT_LE(VAL, N) ( VAL << N )
#define SHIFT_RIGHT_LE(VAL, N) ( VAL >> N )

#define SHIFT_LEFT_BE SHIFT_RIGHT_LE
#define SHIFT_RIGHT_BE SHIFT_LEFT_LE

#define ROTATE_LEFT_16_LE(WORD, N) (\
		(WORD << N) | \
		((WORD >> (16 - N)) & N) \
	)

#define ROTATE_LEFT_16_BE ROTATE_RIGHT_16_LE

#define UNPACK_16_LE(C1, C2) (\
		((C2 & 0xFF) << 8) | (C1 & 0xFF) \
	)

#define UNPACK_24_LE(C1, C2, C3) (\
		((C3 & 0xFF) << 16)	| \
		((C2 & 0xFF) << 8)	| \
		 (C1 & 0xFF) \
	)

#define UNPACK_32_LE(C1, C2, C3, C4) (\
		((C4 & 0xFF) << 24)	| \
		((C3 & 0xFF) << 16)	| \
		((C2 & 0xFF) << 8)	| \
		 (C1 & 0xFF) \
	)

#define UNPACK_16_BE(C1, C2) \
        UNPACK_16_LE(C2, C1)
#define UNPACK_24_BE(C1, C2, C3) \
        UNPACK_24_LE(C3, C2, C1)
#define UNPACK_32_BE(C1, C2, C3, C4) \
        UNPACK_32_LE(C4, C3, C2, C1)

#if (KB_BYTE_ORDER == KB_LIL_ENDIAN)

#define KB_SHIFT_RIGHT_WORD SHIFT_RIGHT_LE
#define KB_SHIFT_LEFT_WORD SHIFT_LEFT_LE
#define KB_SHIFT_RIGHT_WORDBE SHIFT_RIGHT_BE
#define KB_SHIFT_LEFT_WORDBE SHIFT_LEFT_BE

//#define KB_ROTATE_RIGHT_WORD ROTATE_RIGHT_16_LE
#define KB_ROTATE_LEFT_WORD ROTATE_LEFT_16_LE
//#define KB_ROTATE_RIGHT_WORDBE ROTATE_RIGHT_16_BE
#define KB_ROTATE_LEFT_WORDBE ROTATE_LEFT_16_BE

#define KB_UNPACK_WORD UNPACK_16_LE
#define KB_UNPACK_SWORD UNPACK_24_LE
#define KB_UNPACK_DWORD UNPACK_32_LE
#define KB_UNPACK_WORDBE UNPACK_16_BE
#define KB_UNPACK_SWORDBE UNPACK_24_BE
#define KB_UNPACK_DWORDBE UNPACK_32_BE

#else

//#define KB_SHIFT_RIGHT_WORD SHIFT_RIGHT_BE
#define KB_SHIFT_LEFT_WORD SHIFT_LEFT_BE
//#define KB_SHIFT_RIGHT_WORDBE SHIFT_RIGHT_LE
#define KB_SHIFT_LEFT_WORDBE SHIFT_LEFT_LE

#define KB_ROTATE_RIGHT_WORD ROTATE_RIGHT_16_BE
#define KB_ROTATE_LEFT_WORD ROTATE_LEFT_16_BE
#define KB_ROTATE_RIGHT_WORDBE ROTATE_RIGHT_16_LE
#define KB_ROTATE_LEFT_WORDBE ROTATE_LEFT_16_LE

#define KB_UNPACK_WORD UNPACK_16_BE
#define KB_UNPACK_SWORD UNPACK_24_BE
#define KB_UNPACK_DWORD UNPACK_32_BE
#define KB_UNPACK_WORDBE UNPACK_16_LE
#define KB_UNPACK_SWORDBE UNPACK_24_LE
#define KB_UNPACK_DWORDBE UNPACK_32_LE

#endif

#define READ_BYTE(PTR) *PTR++
#define READ_WORD(PTR) KB_UNPACK_WORD(*PTR, *(PTR+1)); PTR+=2
#define READ_SWORD(PTR) KB_UNPACK_SWORD(*PTR, *(PTR+1), *(PTR+2)); PTR+=3
#define READ_DWORD(PTR) KB_UNPACK_DWORD(*PTR, *(PTR+1), *(PTR+2), *(PTR+3)); PTR+=4

#endif /* _OPENKB_LIBKB_SYS */
