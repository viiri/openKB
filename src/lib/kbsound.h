/*
 *  kbsound.h -- Sound format and playing routines definitions
 *  Copyright (C) 2012 Vitaly Driedfruit
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
#ifndef _OPENKB_LIBKB_SOUND
#define _OPENKB_LIBKB_SOUND

#define KBSND_DOS	0x01

typedef struct KBsound {

	byte type;

	union {

		struct {

		};
		struct {

		};

	};

	void *data;

} KBsound;


#define AUDIO_16BIT
#ifndef AUDIO_16BIT
typedef unsigned char aword;
#define AUDIO_FORMAT AUDIO_U8; 
#else
typedef unsigned short aword;
#define AUDIO_FORMAT AUDIO_U16LSB;
#endif

#endif /* _OPENKB_LIBKB_SOUND */
