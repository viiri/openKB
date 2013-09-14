/*
 *  dos-snd.c -- DOS PC Speaker "tunes" reader/player
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

#include "kbsys.h"
#include "kbfile.h"
#include "kbres.h"

#include "kbsound.h"

#include "dos-snd.h"

	/* "2" is '2 bytes per variable' here */
#define HEADER_SIZE_TUN_PAL	((MAX_TUN_FREQS + MAX_TUN_DELAYS) * 2)
#define HEADER_SIZE_TUN_OFF	(MAX_TUN_FILES * 2)
#define HEADER_SIZE_TUN	(MAX_TUN_NOTES * 2)

/* DOS tunes are kept in the exe file, with each tune looking like
 *	freq, delay, ...., 0xFF
 *	where freq and delay are indexes into freq and delay palettes,
 *	and 0xFF is an end marker.
 * Offsets to tunes are kept in "TUNE_PTR" 'array'. Palettes are
 * kept separately and are global for all the tunes.
 */

int tunPalette_load(struct tunPalette* pal, KB_File *f) {

	char buf[HEADER_SIZE_TUN_PAL];

	int i, n;

	//KB_fseek(f, TUNE_BASE_OFFSET + TUNE_NOTES_OFFSET), or KB_fseek(f, TUNE_NOTES_OFFSET) is implied

	n = KB_fread(buf, sizeof(char), HEADER_SIZE_TUN_PAL, f);

	if (n < HEADER_SIZE_TUN_PAL) { KB_errlog("[tunpal] Can't read header! (%d bytes)\n", HEADER_SIZE_TUN_PAL); return -1; }

	/* Parse data */
	char *p = &buf[0];

	KB_debuglog(0, "================== Frequency Palette ==================\n");
	for (i = 0; i < MAX_TUN_FREQS; i++) {

		word freq;
		freq = READ_WORD(p);

		pal->freq[i] = freq;

		KB_debuglog(0, " [%02x] Note %d -- %d hz\n", i, i, freq);
	}
	KB_debuglog(0, "================== Duration Palette ==================\n");
	for (i = 0; i < MAX_TUN_DELAYS; i++) {

		word delay;

		delay = READ_WORD(p);

		pal->duration[i] = delay;

		KB_debuglog(0, " Index %02x -- %d ms\n", i, delay);
	}

	return 0;
};

struct tunGroup* tunGroup_load(KB_File *f) {

	char buf[HEADER_SIZE_TUN_OFF];

	int i, j;

	struct tunGroup *grp;

	int n;

	n = KB_fread(buf, sizeof(char), HEADER_SIZE_TUN_OFF, f);

	if (n < HEADER_SIZE_TUN_OFF) { KB_errlog("[tungrp] Can't read offsets! (%d bytes)\n", HEADER_SIZE_TUN_OFF); return NULL; }

	grp = malloc(sizeof(struct tunGroup));

	if (grp == NULL) return NULL;

	/* Parse data */
	char *p = &buf[0];

	for (i = 0; i < MAX_TUN_FILES; i++) {

		word offset;

		offset = READ_WORD(p);

		grp->offsets[i] = offset;
		
		grp->files[i].num_notes = 0; // init as empty
	}

	grp->num_files = i;

	return grp;
};

int tunFile_read_BUF(struct tunFile* tun, const char *buf, int len) {

	int i;

	/* Parse data */
	const byte *tune = (byte*) &buf[0];

	i = 0;
	while (len > 1) {
		if (*tune == 0xff) break;

		tun->notes[i] = READ_BYTE(tune);
		tun->delay[i] = READ_BYTE(tune);

		i++;
		len -= 2;
	}
	tun->num_notes = i;

	return i;
};

void tunGroup_loadfiles(struct tunGroup* grp, KB_File *f, int base) {

	int i;

	for (i = 0; i < grp->num_files; i++) {
		char buf[HEADER_SIZE_TUN];

		int n;

		KB_fseek(f, base + grp->offsets[i], 0); 

		n = KB_fread(buf, sizeof(char), HEADER_SIZE_TUN, f);

		if (n < HEADER_SIZE_TUN) { KB_errlog("[tune] Can't read data! (%d bytes)\n", HEADER_SIZE_TUN); return; }

		tunFile_read_BUF(&grp->files[i], buf, n);

		memcpy(&grp->files[i].palette, &grp->palette, sizeof(struct tunPalette));
	}

}

struct tunFile* tunFile_load_BUF(const char *buf, int len) {
	int i;

	struct tunFile *fil;

	fil = malloc(sizeof(struct tunFile));

	if (fil == NULL) return NULL;

	i = tunFile_read_BUF(fil, buf, len);

	return fil;
}

int tunFile_reset(struct tunFile *tun, Uint16 format) {

	tun->sampled = 0;

	tun->note_sampled = 0;
	tun->cur_note = 0;
	tun->cone_pos = 0;
	tun->cone_dir = 1;

	tun->move_f = 8;
	tun->move_l = 0;
	if (format & 0x1000) {	/* Hack -- SDL sets bit 12 for MSB. */
		tun->move_f = 0;
		tun->move_l = 8;
	}

	return 0;
}

#ifdef AUDIO_16BIT
#define SAMPLE_BYTE_SIZE 2
#else
#define SAMPLE_BYTE_SIZE 1
#endif

/*
 * DOS KB "tunes" are kept in "frequency/duration" format. The rest of this
 * file deals with trivialities (i.e. loading frequency and duration "palettes"
 * from appropriate offsets, etc), here's when we actually digitize this 
 * information into a PCM stream.
 *
 * An entry of 277/10 in "freq/duration" format means "play 277 hz tone for 10 ms".
 *
 * The conversion is straight-forward. Given the global PCM Frequency ("freq"), 
 * like 11025 [samples per second], we can both determine the needed rate
 * ( PCMfreq / NOTEfreq ) to move our plotter at, and the needed duration (in samples)
 * ( PCMfreq * NOTEdelay / 1000 ).
 *
 * Then, we just plot the cone at /that/ speed for /that much/ needed samples for each
 * "note".
 *
 * For example, 11025 / 277 = 39 -- number of samples needed for 1 cone of the wave
 *	11025 * 10 / 1000 = 110 -- number of samples needed to fill 10 ms
 *  110 / 39 = 2.8f -- number of cones we must plot (not used directly, but could be)
 *
 * --------------------
 *    W      W      W
 *   W W    W W    W W
 *  W   W  W   W  W   W
 * W     WW     WW 
 * ------|-------------
 *       | one "cone" of the wave
 */
int tunFile_play(struct tunFile *tun, Uint8 *stream, int len, int freq) {

	int i, n, samples = 0;

	int req_samples = len / SAMPLE_BYTE_SIZE;

	//TODO: to avoid clicking sound at the end, lead the cone to "silence",
	//instead of ending abruptply like this:
	if (tun->cur_note >= tun->num_notes) return 0;

	word delay_index = tun->delay[ tun->cur_note ]; 

	word note_index = tun->notes[ tun->cur_note ]; 

	aword ms_note_delay = tun->palette.duration[ delay_index ]; // i.e. 20 ms 

	word samples_delay = freq * ms_note_delay / 1000;

	aword X = tun->palette.freq[ note_index ];	//i.e. 233 hz 

	//KB_debuglog(0, "%d/%d) play note [%d] %d hz for [%d] %d ms - %d samples\n", tun->cur_note, tun->num_notes, note_index, X, delay_index, ms_note_delay, samples_delay - tun->note_sampled);

	samples = samples_delay < req_samples ? samples_delay : req_samples; // crude MIN()

	//TODO: using the powers of MATH we can actually determine cone_pos and cone_dir
	// each time we need them, by something like
	//sin(x * bay) * PEAK + LEAK
	aword LEAK = 0x000F;	// min value for audio format
	aword PEAK = 0xFFF0;	// max value for audio format

	aword bay = (X == 0 ? 0 : freq / X);

	word speed = (bay == 0 ? 0 : (PEAK - LEAK) / bay);

	//those values swap depending on audio format endianess:
	int moveH = tun->move_f; // ammount of shift-right needed for First byte
	int moveL = tun->move_l; // ammount of shift-right needed for Last  byte

	for (i = 0; i < samples; i++) {

		int step = tun->cone_dir * speed;

		if (tun->cone_pos + step <= LEAK) {
			tun->cone_pos = LEAK;
			tun->cone_dir = 1;
		}
		else
		if (tun->cone_pos + step >= PEAK) {
			tun->cone_pos = PEAK;
			tun->cone_dir = -1;
		}
		else
			tun->cone_pos += step;

		aword sample = tun->cone_pos;
#ifdef AUDIO_16BIT
		/* Be pedantic about byte order */
		aword H = (sample >> moveH) & 0x00FF;
		aword L = (sample >> moveL) & 0x00FF;
		*stream++ = (Uint8)L;
		*stream++ = (Uint8)H;
#else
		/* 1-to-1 copy */
		*stream++ = (Uint8)sample;
#endif
	}

	tun->sampled += samples;
	tun->note_sampled += samples;

	/* Next "note" */
	if (tun->note_sampled >= samples_delay) {
		tun->note_sampled = 0;
		tun->cur_note++;
	}

	return samples * SAMPLE_BYTE_SIZE;
};

