/*
 *  kbauto.h -- Global "automatic" modules for your convinience.
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
#ifndef _OPENKB_LIBKB_AUTO
#define _OPENKB_LIBKB_AUTO

#define MAX_MODULES	16
#define MAX_SLOTS	4

#include "kbsys.h"
#include "kbdir.h"

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

} KBmodule;

extern KBmodule *main_module;

#endif /* _OPENKB_LIBKB_AUTO */
