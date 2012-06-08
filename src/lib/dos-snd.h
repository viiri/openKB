/*
 *  dos-snd.h -- DOS PC Speaker "tunes" reader/player interface
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
#ifndef _OPENKB_LIBKB_DOSSOUND
#define _OPENKB_LIBKB_DOSSOUND

#define MAX_TUN_NOTES	255 /* There's actually no hard limit, it should be malloced at run-time */

#define MAX_TUN_FILES	10

#define MAX_TUN_FREQS	88	/* Each palette has 88 "notes" and */
#define MAX_TUN_DELAYS	16	/* 16 "delays". */

/* OFFSETS: */
#define TUNE_NOTES_OFFSET	0x3347	/* Freq. palette */
#define TUNE_DELAY_OFFSET	0x33F7	/* Delay palette */
#define TUNE_PTR_OFFSET 	0x3417	/* Pointers into "tunes" */ 
#define TUNE_TUNE_OFFSET 	0x330d	/* "Tunes" (freq,delay,...,0x0FF) */

struct tunFile {

	struct tunPalette {

		word freq[MAX_TUN_FREQS];
		word duration[MAX_TUN_DELAYS];

	} palette;

	byte notes[MAX_TUN_NOTES];
	byte delay[MAX_TUN_NOTES];

	word num_notes;

	/* ----------------------------------------------------------- */
	int sampled;		/* Those run-time variables are not part */
	int note_sampled;	/* of the actual sound, they are used */
	int cur_note;		/* to digitize it into a PCM stream */
	int cone_pos;
	int cone_dir;
	int move_f;	/* 8 for LSB, 0 for MSB */
	int move_l;	/* 0 for LSB, 8 for MSB */
};

struct tunGroup {

	struct tunPalette palette;

	struct tunFile files[MAX_TUN_FILES];

	word offsets[MAX_TUN_FILES];

	word num_files;

};

extern int tunPalette_load(struct tunPalette* pal, KB_File *f);
extern struct tunGroup* tunGroup_load(KB_File *f);
extern void tunGroup_loadfiles(struct tunGroup* grp, KB_File *f, int base);

extern int tunFile_play(struct tunFile *tun, Uint8 *stream, int len, int freq);
extern int tunFile_reset(struct tunFile *tun, Uint16 format);


#endif
