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

#include "kbconf.h"
#include "kbfile.h"
#include "kbdir.h"

extern void discover_modules(const char *path, KBconfig *conf);

extern void init_module(KBmodule *mod);
extern void init_modules(KBconfig *conf);
extern void stop_module(KBmodule *mod);
extern void stop_modules(KBconfig *conf);

extern void wipe_module(KBmodule *mod);

extern void register_module(KBconfig *conf, KBmodule *mod);

//KBstd material?
extern void name_split(const char *name, char *base, char *ext);

KB_DIR  * KB_opendir_with(const char *filename, KBmodule *mod);
KB_File * KB_fopen_with(const char *filename, char *mode, KBmodule *mod);

#endif /* _OPENKB_LIBKB_AUTO */
