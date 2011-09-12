/*
 *  save.c -- routines to load/save KB games
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
#include "bounty.h"
#include "malloc.h"

#define DAT_SIZE 20421

// TODO: i recall theres a global way for those now?
#define _UN_BYTE(PTR) *PTR++
#define _UN_WORD(PTR) UNPACK_16_LE(*PTR++, *PTR++)
#define _UN_SWORD(PTR) UNPACK_24_LE(*PTR++, *PTR++, *PTR++)
#define _UN_DWORD(PTR) UNPACK_32_LE(*PTR++, *PTR++, *PTR++, *PTR++)

#define RPOS(STR) printf("%s Pos: %06x\n", STR, p - &buf[0])

KBgame* KB_loadDAT(const char* filename) {

	char buf[DAT_SIZE];
	KBgame *game;
	FILE *f;
	int n, k, map_size;

	f = fopen(filename, "rb");
	if (f == NULL) return NULL;
	
	n = fread(buf, sizeof(char), DAT_SIZE, f);
	fclose(f);
	if (n != DAT_SIZE) return NULL;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	/* Read out the save from "buf" */
	char *p = &buf[0];

	/* Player name */
	memcpy(game->name, p, 10);
	for (n = 9; n > 0; n--) {
		if (game->name[n] != ' ') break;
		game->name[n] = '\0';
	}
	p += 11;

	/* Main stats */
	game->class = _UN_BYTE(p);
	game->rank = _UN_BYTE(p);
	game->spell_power = _UN_BYTE(p);
	game->max_spells = _UN_BYTE(p);

	/* Quest progress */
	for (n = 0; n < MAX_VILLAINS; n++)
		game->villain_caught[n] = _UN_BYTE(p);

	for (n = 0; n < MAX_ARTIFACTS; n++)
		game->artifact_found[n] = _UN_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		game->continent_found[n] = _UN_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		game->orb_found[n] = _UN_BYTE(p);

	/* Spell book */
	for (n = 0; n < MAX_SPELLS; n++)
		game->spells[n] = _UN_BYTE(p);

	/* More stats */
	game->knows_magic = _UN_BYTE(p);
	game->siege_weapons = _UN_BYTE(p);
	game->contract = _UN_BYTE(p);

	for (n = 0; n < 5; n++)
		game->player_troops[n] = _UN_BYTE(p);

	/* Options and difficulty setting */
	game->options[0] = _UN_BYTE(p);//delay

	game->difficulty = _UN_BYTE(p);

 	game->options[1] = _UN_BYTE(p);//sounds
 	game->options[2] = _UN_BYTE(p);//walk beep
 	game->options[3] = _UN_BYTE(p);//animation
 	game->options[4] = _UN_BYTE(p);//show army size

	/* Player location */
	game->continent = _UN_BYTE(p);
	game->x = _UN_BYTE(p);
	game->y = _UN_BYTE(p);
	game->last_x = _UN_BYTE(p);
	game->last_y = _UN_BYTE(p);
	game->boat_x = _UN_BYTE(p);
	game->boat_y = _UN_BYTE(p);
	game->boat = _UN_BYTE(p);
	game->mount = _UN_BYTE(p);

	game->options[5] = _UN_BYTE(p);//CGA palette

	/* Spells sold in towns */
	for (n = 0; n < MAX_TOWNS; n++)
		game->town_spell[n] = _UN_BYTE(p);

	/* Town/Contract */
	for (n = 0; n < 5; n++)
		game->contract_cycle[n] = _UN_BYTE(p);
	game->last_contract =  _UN_BYTE(p);
	game->max_contract = _UN_BYTE(p);

	/* Steps left till end of day */
	game->steps_left = _UN_BYTE(p);

	/* Castle ownership */
	for (n = 0; n < MAX_CASTLES; n++)
		game->castle_owner[n] = _UN_BYTE(p);

	/* Visited castles and towns */
	for (n = 0; n < MAX_CASTLES; n++)
		game->castle_visited[n] = _UN_BYTE(p);

	for (n = 0; n < MAX_TOWNS; n++)
		game->town_visited[n] = _UN_BYTE(p);

	/* Scepter location (encrypted) */
	game->scepter_continent = _UN_BYTE(p);
	game->scepter_x = _UN_BYTE(p);
	game->scepter_y = _UN_BYTE(p);

	/* Fog of war */
	map_size = ( MAX_CONTINENTS * LEVEL_W * LEVEL_H );
	n = map_size / 8 + 1; 
	memcpy(game->fog, p, n);
	p += n;

	/* Castle garrison troops */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			game->castle_troops[n][k] = _UN_BYTE(p);

	/* Read friendly followers' coords */
	for (n = 0; n < 20; n++) {
		game->follower_coords[n][0] = _UN_BYTE(p);//X
		game->follower_coords[n][1] = _UN_BYTE(p);//Y	
	}

	/* Map chests */
	for (n = 0; n < MAX_CONTINENTS - 1; n++) {
		game->map_coords[n][0] = _UN_BYTE(p);//X
		game->map_coords[n][1] = _UN_BYTE(p);//Y
	}

	/* Orb chests */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		game->orb_coords[n][0] = _UN_BYTE(p);//X
		game->orb_coords[n][1] = _UN_BYTE(p);//Y
	}

	/* Teleporting caves */
	for (n = 0; n < MAX_CONTINENTS * 2; n++) {
		game->teleport_coords[n][0] = _UN_BYTE(p);//X
		game->teleport_coords[n][1] = _UN_BYTE(p);//Y
	}

	/* Dwellings locations */
	for (n = 0; n < MAX_DWELLINGS; n++) {
		game->dwelling_coords[n][0] = _UN_BYTE(p);//X
		game->dwelling_coords[n][1] = _UN_BYTE(p);//Y
	}

	/* Read hostile followers' coords */
	for (n = 20; n < MAX_FOLLOWERS; n++) {
		game->follower_coords[n][0] = _UN_BYTE(p);//X
		game->follower_coords[n][1] = _UN_BYTE(p);//Y	
	}

	/* Read hostile followers' troops */
	for (n = 20; n < MAX_FOLLOWERS; n++)
		for (k = 0; k < 3; k++)
			game->follower_troops[n][k] = _UN_BYTE(p);

	/* Read follower numbers */
	for (n = 20; n < MAX_FOLLOWERS; n++)
		for (k = 0; k < 3; k++)
			game->follower_numbers[n][k] = _UN_BYTE(p);

	/* Read dwelling troop type and population count */
	for (n = 0; n < MAX_DWELLINGS; n++)
			game->dwelling_troop[n] = _UN_BYTE(p);	

	for (n = 0; n < MAX_DWELLINGS; n++)
			game->dwelling_population[n] = _UN_BYTE(p);

	/* Read scepter key and un-encrypt the coordinates */
	game->scepter_key = _UN_BYTE(p);
	game->scepter_continent ^ game->scepter_key;
	game->scepter_x ^ game->scepter_key;
	game->scepter_y ^ game->scepter_key;

	/* More stats */
	game->base_leadership = _UN_WORD(p);
	game->leadership = _UN_WORD(p);
	game->commission = _UN_WORD(p);
	game->followers_killed = _UN_WORD(p);

	/* Player army numbers */
	for (n = 0; n < 5; n++)
		game->player_numbers[n] = _UN_WORD(p);

	/* Castle garrison numbers */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			game->castle_numbers[n][k] = _UN_WORD(p);

	/* More stats */
	game->time_stop = _UN_WORD(p);
	game->days_left = _UN_WORD(p);

	/* Score (unused) */
	game->score = _UN_WORD(p);

	/* Skip 4 bytes */
	game->unknown1 = _UN_BYTE(p);
	game->unknown2 = _UN_BYTE(p);

	/* Player's gold (32 bit, can get quite rich!) */
	game->gold = _UN_DWORD(p);

	/* Map dump */
	n = map_size; 
	memcpy(game->map, p, n);
	p += n;

	fprintf(stdout, "Save file: %d bytes (needed %d)\n", p - &buf[0], DAT_SIZE);
	if (p - &buf[0] != DAT_SIZE) sleep(1);
	return game;
}

int KB_saveDAT(const char* filename, KBgame *game) {

	char buf[DAT_SIZE];
	FILE *f;
	int n;

	f = fopen(filename, "wb");
	if (f == NULL) return 1;

	/* Layout the save in "buf" */
	//TODO......

	n = fwrite(buf, sizeof(char), DAT_SIZE, f);
	fclose(f);

	if (n != DAT_SIZE) return 2;
}