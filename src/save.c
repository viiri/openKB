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
	game->class = READ_BYTE(p);
	game->rank = READ_BYTE(p);
	game->spell_power = READ_BYTE(p);
	game->max_spells = READ_BYTE(p);

	/* Quest progress */
	for (n = 0; n < MAX_VILLAINS; n++)
		game->villain_caught[n] = READ_BYTE(p);

	for (n = 0; n < MAX_ARTIFACTS; n++)
		game->artifact_found[n] = READ_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		game->continent_found[n] = READ_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		game->orb_found[n] = READ_BYTE(p);

	/* Spell book */
	for (n = 0; n < MAX_SPELLS; n++)
		game->spells[n] = READ_BYTE(p);

	/* More stats */
	game->knows_magic = READ_BYTE(p);
	game->siege_weapons = READ_BYTE(p);
	game->contract = READ_BYTE(p);

	for (n = 0; n < 5; n++)
		game->player_troops[n] = READ_BYTE(p);

	/* Options and difficulty setting */
	game->options[0] = READ_BYTE(p);//delay

	game->difficulty = READ_BYTE(p);

 	game->options[1] = READ_BYTE(p);//sounds
 	game->options[2] = READ_BYTE(p);//walk beep
 	game->options[3] = READ_BYTE(p);//animation
 	game->options[4] = READ_BYTE(p);//show army size

	/* Player location */
	game->continent = READ_BYTE(p);
	game->x = READ_BYTE(p);
	game->y = READ_BYTE(p);
	game->last_x = READ_BYTE(p);
	game->last_y = READ_BYTE(p);
	game->boat_x = READ_BYTE(p);
	game->boat_y = READ_BYTE(p);
	game->boat = READ_BYTE(p);
	game->mount = READ_BYTE(p);

	game->options[5] = READ_BYTE(p);//CGA palette

	/* Spells sold in towns */
	for (n = 0; n < MAX_TOWNS; n++)
		game->town_spell[n] = READ_BYTE(p);

	/* Town/Contract */
	for (n = 0; n < 5; n++)
		game->contract_cycle[n] = READ_BYTE(p);
	game->last_contract =  READ_BYTE(p);
	game->max_contract = READ_BYTE(p);

	/* Steps left till end of day */
	game->steps_left = READ_BYTE(p);

	/* Castle ownership */
	for (n = 0; n < MAX_CASTLES; n++)
		game->castle_owner[n] = READ_BYTE(p);

	/* Visited castles and towns */
	for (n = 0; n < MAX_CASTLES; n++)
		game->castle_visited[n] = READ_BYTE(p);

	for (n = 0; n < MAX_TOWNS; n++)
		game->town_visited[n] = READ_BYTE(p);

	/* Scepter location (encrypted) */
	game->scepter_continent = READ_BYTE(p);
	game->scepter_x = READ_BYTE(p);
	game->scepter_y = READ_BYTE(p);

	/* Fog of war */
	map_size = ( MAX_CONTINENTS * LEVEL_W * LEVEL_H );
	n = map_size / 8 + 1; 
	memcpy(game->fog, p, n);
	p += n;

	/* Castle garrison troops */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			game->castle_troops[n][k] = READ_BYTE(p);

	/* Read friendly followers' coords */
	for (n = 0; n < 20; n++) {
		game->follower_coords[n][0] = READ_BYTE(p);//X
		game->follower_coords[n][1] = READ_BYTE(p);//Y	
	}

	/* Map chests */
	for (n = 0; n < MAX_CONTINENTS - 1; n++) {
		game->map_coords[n][0] = READ_BYTE(p);//X
		game->map_coords[n][1] = READ_BYTE(p);//Y
	}

	/* Orb chests */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		game->orb_coords[n][0] = READ_BYTE(p);//X
		game->orb_coords[n][1] = READ_BYTE(p);//Y
	}

	/* Teleporting caves */
	for (n = 0; n < MAX_CONTINENTS * 2; n++) {
		game->teleport_coords[n][0] = READ_BYTE(p);//X
		game->teleport_coords[n][1] = READ_BYTE(p);//Y
	}

	/* Dwellings locations */
	for (n = 0; n < MAX_DWELLINGS; n++) {
		game->dwelling_coords[n][0] = READ_BYTE(p);//X
		game->dwelling_coords[n][1] = READ_BYTE(p);//Y
	}

	/* Read hostile followers' coords */
	for (n = 20; n < MAX_FOLLOWERS; n++) {
		game->follower_coords[n][0] = READ_BYTE(p);//X
		game->follower_coords[n][1] = READ_BYTE(p);//Y	
	}

	/* Read hostile followers' troops */
	for (n = 20; n < MAX_FOLLOWERS; n++)
		for (k = 0; k < 3; k++)
			game->follower_troops[n][k] = READ_BYTE(p);

	/* Read follower numbers */
	for (n = 20; n < MAX_FOLLOWERS; n++)
		for (k = 0; k < 3; k++)
			game->follower_numbers[n][k] = READ_BYTE(p);

	/* Read dwelling troop type and population count */
	for (n = 0; n < MAX_DWELLINGS; n++)
			game->dwelling_troop[n] = READ_BYTE(p);	

	for (n = 0; n < MAX_DWELLINGS; n++)
			game->dwelling_population[n] = READ_BYTE(p);

	/* Read scepter key and un-encrypt the coordinates */
	game->scepter_key = READ_BYTE(p);
	game->scepter_continent ^= game->scepter_key;
	game->scepter_x ^= game->scepter_key;
	game->scepter_y ^= game->scepter_key;

	/* More stats */
	game->base_leadership = READ_WORD(p);
	game->leadership = READ_WORD(p);
	game->commission = READ_WORD(p);
	game->followers_killed = READ_WORD(p);

	/* Player army numbers */
	for (n = 0; n < 5; n++)
		game->player_numbers[n] = READ_WORD(p);

	/* Castle garrison numbers */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			game->castle_numbers[n][k] = READ_WORD(p);

	/* More stats */
	game->time_stop = READ_WORD(p);
	game->days_left = READ_WORD(p);

	/* Score (unused) */
	game->score = READ_WORD(p);

	/* Skip 4 bytes */
	game->unknown1 = READ_BYTE(p);
	game->unknown2 = READ_BYTE(p);

	/* Player's gold (32 bit, can get quite rich!) */
	game->gold = READ_DWORD(p);

	/* Map dump */
	n = map_size; 
	memcpy(game->map, p, n);
	p += n;

	if (p - &buf[0] != DAT_SIZE)
	fprintf(stdout, "Save file: %d bytes (needed %d)\n", p - &buf[0], DAT_SIZE);

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