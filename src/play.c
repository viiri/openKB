/*
 *  play.c -- gameplay mechanics
 *  Copyright (C) 2011-2014 Vitaly Driedfruit
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
#include "lib/kbstd.h"

/* forward-declare: */
void player_accept_rank(KBgame *game);

void roll_creature(int difficulty, byte *id, word *number) {
	byte index = 0;
	byte troop_id;
	word troop_count;
	byte dwelling = KB_rand(0, 3);
	byte chance = KB_rand(1, 100);

	while (chance > troop_chance_table[difficulty][index]) {
		index++;
		/* Safety: */
		if (index >= MAX_TROOP_CHANCE_CURVE-1) break;
	}

	troop_id = dwelling_to_troop[dwelling][index];
	troop_count = troop_numbers[troop_id][difficulty];

	if (troop_count <= 1) troop_count = 2;

	*id = troop_id;
	*number = troop_count;
}

void enforce_dwelling(KBgame *game, int continent, int dwelling_id, byte troop_id) {
	/* Populate! */
	game->dwelling_troop[continent][dwelling_id] = troop_id;
	game->dwelling_population[continent][dwelling_id] = troops[troop_id].max_population;
}

byte populate_dwelling(KBgame *game, int continent, int dwelling_id) {
	byte troop_id;

	/* Figure troop id */
	if (dwelling_id >= MAX_DWELLINGS
	|| continent_dwellings[continent][dwelling_id] == 0xFF) {
		/* Roll randomly */
		troop_id = KB_rand(
			dwelling_ranges[continent][0],
			dwelling_ranges[continent][1]);
	}	else {
		/* Take from list */
		troop_id = continent_dwellings[continent][dwelling_id];
	}

	/* Populate! */
	enforce_dwelling(game, continent, dwelling_id, troop_id);

	/* Convert troop id to dwelling id */
	return troops[troop_id].dwells;
}
void repopulate_castle(KBgame *game, int castle_id) {
	int i;
	for (i = 0; i < 5; i++) {
		byte troop_id;
		word troop_count;

		roll_creature(castle_difficulty[castle_id], &troop_id, &troop_count);

		game->castle_troops[castle_id][i] = troop_id;
		game->castle_numbers[castle_id][i] = troop_count;
	}
}
void repopulate_foe(KBgame *game, int continent, int foe_id) {
	int i;
	for (i = 0; i < 3; i++) {
		byte troop_id;
		word troop_count;

		roll_creature(continent, &troop_id, &troop_count);

		game->foe_troops[continent][foe_id][i] = troop_id;
		game->foe_numbers[continent][foe_id][i] = troop_count;
	}
}

word chests_on_continent(KBgame *game, int continent) {
	int i, j;
	word count = 0;
	for (j = 0; j < LEVEL_H; j++) {
		for (i = 0; i < LEVEL_W; i++) {
			if (game->map[continent][j][i] == 0x8B)
				count++;
		}
	}
	return count;
}
word grass_on_continent(KBgame *game, int continent) {
	int i, j;
	word count = 0;
	for (j = 0; j < LEVEL_H; j++) {
		for (i = 0; i < LEVEL_W; i++) {
			if (game->map[continent][j][i] == 0x00
			|| game->map[continent][j][i] == 0x80)
				count++;
		}
	}
	return count;
}
byte num_castles(int continent) {
	byte count;
	int i;
	count = 0;
	for (i = 0; i < MAX_CASTLES; i++) {
		if (castle_coords[i][0] == continent) {
			count++;
		}
	}
	return count;
}

void bury_scepter(KBgame *game, int continent, word grass) {
	int i, j;
	word count = 0;
	for (j = 0; j < LEVEL_H; j++) {
		for (i = 0; i < LEVEL_W; i++) {
			if (game->map[continent][j][i] == 0x00
			|| game->map[continent][j][i] == 0x80) {
				if (count == grass) {
					game->scepter_x = i;
					game->scepter_y = i;
					return;		
				}
				count++;
			}
		}
	}
}

int salt_villains(KBgame *game, int continent, int base_id) {
	int i, j;
	int max = num_castles(continent);
	if (max < villains_per_continent[continent]) {
		KB_errlog("Not enough (%d) castles to put %d villains in!\n", max,villains_per_continent[continent]);
		return base_id;
	}
	for (j = 0; j < villains_per_continent[continent];) {
		int villain_id = base_id + j;
		int castle_id = KB_rand(0, MAX_CASTLES-1);
		/* Castle on this continent and is unoccupied */
		if (castle_coords[castle_id][0] == continent
		 && game->castle_owner[castle_id] == 0x7F) {
			/* Give ownership to villain */
			game->castle_owner[castle_id] = villain_id;
			/* Put his troops */
			for (i = 0; i < 5; i++) {
				game->castle_troops[castle_id][i] = villain_army_troops[villain_id][i];
				game->castle_numbers[castle_id][i] = villain_army_numbers[villain_id][i];
			}
			j++;
		}
	}
	return base_id + j;
}


/* Ensure 2 artifacts, 1 navmap, 1 orb, 2 telecaves, 10 dwellings and 5 friendly foes. */
void salt_continent(KBgame *game, int continent, int min_artifacts, int min_navmaps, int min_orbs, int min_telecaves, int min_dwellings, int min_friendly) {
	int i, j;

	enum {
		SALT_NONE,
		SALT_ARTIFACT,
		SALT_NAVMAP,
		SALT_ORB,
		SALT_TELECAVE,
		SALT_DWELLING,
		SALT_FRIENDLY,
	};

	int num_foe = 0;
	int num_artifacts, num_dwellings, num_navmaps, num_orbs, num_telecaves, num_friendly;
	int min_len = min_artifacts + min_navmaps + min_orbs + min_telecaves + min_dwellings + min_friendly;

	byte *barrel;
	int barrel_index = 0;
	int barrel_len = chests_on_continent(game, continent);

	if (barrel_len < min_len) {
		KB_errlog("Unable to salt continent %d, have %d minimum objects and only %d slots!\n", continent, min_len, barrel_len);
		return;
	}

	/* Replace start marker with water tile */
	if (game->map[continent][0][0] == 0xFF) {
		game->map[continent][0][0] = 0x20;
	}

	barrel = malloc(sizeof(byte) * barrel_len);
	if (barrel == NULL) return; /* error */

	memset(barrel, SALT_NONE, barrel_len);
	num_artifacts = num_dwellings = num_navmaps = num_orbs = num_telecaves = num_friendly = 0;

	for (i = 0; i < min_artifacts; ) {
		barrel_index = KB_rand(0, barrel_len - 1);
		if (barrel[barrel_index] == SALT_NONE) {
			barrel[barrel_index] = SALT_ARTIFACT;
			i++;
		}
	}
	for (i = 0; i < min_navmaps; ) {
		barrel_index = KB_rand(0, barrel_len - 1);
		if (barrel[barrel_index] == SALT_NONE) {
			barrel[barrel_index] = SALT_NAVMAP;
			i++;
		}
	}
	for (i = 0; i < min_orbs; ) {
		barrel_index = KB_rand(0, barrel_len - 1);
		if (barrel[barrel_index] == SALT_NONE) {
			barrel[barrel_index] = SALT_ORB;
			i++;
		}
	}
	for (i = 0; i < min_telecaves; i++) {
		barrel_index = KB_rand(0, barrel_len - 1);
		if (barrel[barrel_index] == SALT_NONE) {
			barrel[barrel_index] = SALT_TELECAVE;
		}
	}
	for (i = 0; i < min_dwellings; i++) {
		barrel_index = KB_rand(0, barrel_len - 1);
		if (barrel[barrel_index] == SALT_NONE) {
			barrel[barrel_index] = SALT_DWELLING;
		}
	}
	for (i = 0; i < min_friendly; i++) {
		barrel_index = KB_rand(0, barrel_len - 1);
		if (barrel[barrel_index] == SALT_NONE) {
			barrel[barrel_index] = SALT_FRIENDLY;
		}
	}

	num_foe = min_friendly;
	barrel_index = 0;
	for (j = 0; j < LEVEL_H; j++) {
		for (i = 0; i < LEVEL_W; i++) {
			if (game->map[continent][j][i] == 0x91) {
				game->foe_coords[continent][num_foe][0] = i;
				game->foe_coords[continent][num_foe][1] = j;
				repopulate_foe(game, continent, num_foe);
				num_foe++;
			}
			else if (game->map[continent][j][i] == 0x8B) {

				if (barrel_index < barrel_len) {
					switch (barrel[barrel_index]) {
						case SALT_ARTIFACT:
							game->map[continent][j][i] = 0x92 + num_artifacts;
							num_artifacts++;
						break;
						case SALT_ORB:
							game->orb_coords[continent][0] = i;
							game->orb_coords[continent][1] = j;
							num_orbs++;
						break;
						case SALT_NAVMAP:
							game->map_coords[continent][0] = i;
							game->map_coords[continent][1] = j;
							num_navmaps++;
						break;
						case SALT_TELECAVE:
							game->map[continent][j][i] = 0x8E;
							game->teleport_coords[continent][num_telecaves][0] = i;
							game->teleport_coords[continent][num_telecaves][1] = j;
							num_telecaves++;
						break;
						case SALT_DWELLING:
							game->map[continent][j][i] = 0x8C
								+ populate_dwelling(game, continent, num_dwellings);
							game->dwelling_coords[continent][num_dwellings][0] = i;
							game->dwelling_coords[continent][num_dwellings][1] = j;
							num_dwellings++;
						break;
						case SALT_FRIENDLY:
							game->map[continent][j][i] = 0x91;
							game->foe_coords[continent][num_friendly][0] = i;
							game->foe_coords[continent][num_friendly][1] = j;
							//game->foe_troops[MAX_CONTINENTS][MAX_FOES][5];
							//game->foe_numbers[MAX_CONTINENTS][MAX_FOES][5];
							//populate_foe(game, continent, ??
							num_friendly++;
						break;
						case SALT_NONE:
						default:
							/* Do nothing (leave as chest) */
						break;
					}
					barrel_index++;
				}
				else if (game->map[continent][j][i] == 0x8C) {
					/* Plains */
					game->dwelling_coords[continent][num_dwellings][0] = i;
					game->dwelling_coords[continent][num_dwellings][1] = j;
					enforce_dwelling(game, continent, num_dwellings, 0);
					num_dwellings++;
				}
				else
				{
					/* Do nothing (leave as chest) */
				}
			}
		}
	} 
}

void salt_spells(KBgame *game) {
	int i, j;

	if (MAX_SPELLS > MAX_CASTLES) {
		KB_errlog("Can't fit %d spells in %d towns!\n", MAX_SPELLS, MAX_CASTLES);
		return;
	}

	/* Set spells for all towns to 0xFF */
	for (i = 0; i < MAX_CASTLES; i++) {
		game->town_spell[i] = 0xFF;
	}

	/* Set spell for town[0x15] to 0x07 */
	/* huntersville sells bridge! */
	/* TODO: make this less hardcoded */
	game->town_spell[0x15] = 0x07; 

	/* Foreach Spell (except 0x07) */
	for (i = 0; i < MAX_SPELLS; ) {
		if (i == 0x07) { i++; continue; }
		j = KB_rand(0, MAX_CASTLES-1);
		if (game->town_spell[j] == 0xFF) {
			game->town_spell[j] = i;
			i++;
		}
	}   
	
	/* Foreach Town that still sells 0xFF: */
	for (i = 0; i < MAX_CASTLES; i++) {
		if (game->town_spell[i] == 0xFF) {
			/* Random spell */
			game->town_spell[i] = KB_rand(0, MAX_SPELLS-1);
		}
	}
}

/* Actual KBgame* allocator */
KBgame *spawn_game(char *name, int pclass, int difficulty, byte *land) {
	int i;
	KBgame* game;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;
	memset(game, 0, sizeof(KBgame));

	/* Load base map */
	memcpy(game->map, land, LEVEL_W * LEVEL_H * MAX_CONTINENTS); 

	/* Hide scepter */
	game->scepter_key = KB_rand(0x00, 0xFF);
	game->scepter_continent = KB_rand(100, 400) / 100 - 1;
	i = KB_rand(0, grass_on_continent(game, game->scepter_continent) - 1);
	bury_scepter(game, game->scepter_continent, i);

	/* Initialize character */
	KB_strcpy(game->name, name);
	game->difficulty = difficulty;
	game->class = pclass;
	game->days_left = days_per_difficulty[difficulty];
	game->steps_left = DAY_STEPS;

	game->gold = starting_gold[pclass];

	game->continent = special_coords[SP_HOME][0];
	game->x = special_coords[SP_HOME][1];
	game->y = special_coords[SP_HOME][2] - 2;
	game->continent_found[HOME_CONTINENT] = 1;

	game->mount = KBMOUNT_RIDE;
	game->boat = 0xFF;
	game->last_x = game->x;
	game->last_y = game->y;

	game->rank = 0;
	player_accept_rank(game);
	game->leadership = game->base_leadership;
	game->time_stop = 0;

	game->contract = 0xFF;
	game->last_contract = 0x04;
	game->max_contract = 0x05;
	game->contract_cycle[0] = 0;
	game->contract_cycle[1] = 1;
	game->contract_cycle[2] = 2;
	game->contract_cycle[3] = 3;
	game->contract_cycle[4] = 4;

	for (i = 0; i < 2; i++) {
		game->player_troops[i] = starting_army_troop[pclass][i];
		game->player_numbers[i] = starting_army_numbers[pclass][i];
	}
	for (i = 2; i < 5; i++) {
		game->player_troops[i] = 0xFF;
		game->player_numbers[i] = 0;
	}

	/* Randomize spells sold in towns */
	salt_spells(game);

	/* Remove magic alcove */
	if (game->knows_magic) {
		game->map[ALCOVE_CONTINENT][ALCOVE_Y][ALCOVE_X] = 0;
	}

	/* Salt each continent */
	for (i = 0; i < MAX_CONTINENTS; i++) {
		/* This also populates Foes encountered on the map. */
		/* This also popluates all dwellings */	
		/* This also populates dwelling at 0,1b,b with peasants (hard coded). */
		/* Ensure 2 artifacts, 1 navmap, 1 orb, 2 telecaves, 10 dwellings and 5 friendly foes. */
		salt_continent(game, i, 2, 1, 1, 2, 10, 5);
	}

	/* Castles */
	for (i = 0; i < MAX_CASTLES; i++) {
		/* Initially, give castle to monsters */
		game->castle_owner[i] = 0x7F;
	}

	/* Villains! */
	i = salt_villains(game, 0, 0);
	i = salt_villains(game, 1, i);
	i = salt_villains(game, 2, i);
	i = salt_villains(game, 3, i);

	/* Populate all castles owned by monsters */
	for (i = 0; i < MAX_CASTLES; i++) {
		if (game->castle_owner[i] == 0x7F) {
			repopulate_castle(game, i);
		}
	}

	return game;
}

/* Calculate how many days have passed by inverting the "days left" variable */
word passed_days(KBgame *game) {

	word max_days = days_per_difficulty[game->difficulty];

	word pass_days = max_days - game->days_left; 

	return pass_days;
}

/* Calculate and return current week number */
word week_id(KBgame *game) {

	word w_id = passed_days(game) / WEEK_DAYS;

	return w_id;
}

/* End the day; return 1 if week ended, 0 otherwise */
int end_day(KBgame *game) {
	game->days_left -= 1;
	game->steps_left = DAY_STEPS;
	game->time_stop = 0;
	if (!(game->days_left % WEEK_DAYS)) return 1;
	return 0;
}

/* Spend some ammount of game days, return number of weeks passed */
int spend_days(KBgame *game, word days) {
	int i, weeks_passed = 0;
	if (days > game->days_left) days = game->days_left;
	for (i = 0; i < days; i++) {
		weeks_passed += end_day(game);
	}
	return weeks_passed;
}

/* Spend remaining days in the week */
void spend_week(KBgame *game) {
	word w_id = week_id(game); 
	word pass_days = passed_days(game);

	word week_days = pass_days - (w_id * WEEK_DAYS);
	word end_week_days = WEEK_DAYS - week_days;

	spend_days(game, end_week_days);
}

/* Spend some ammount of gold (and watch for overflows) */
void spend_gold(KBgame *game, word amount) {
	if (amount <= game->gold)
		game->gold -= amount;
	else
		game->gold = 0;
}

/* Close contract on villain */
void fullfill_contract(KBgame *game, byte villain_id) {
	int i;
	int slot = -1;

	/** Fullfill contract **/
	game->gold += villain_rewards[villain_id];
	game->villain_caught[villain_id] = 1;
	game->contract = 0xFF;

	/** Cycle contracts available in towns **/
	/* Find slot */
	for (i = 0; i < 5; i++)
		if (game->contract_cycle[i] == villain_id) slot = i;
	if (slot == -1) return;

	/* Clear slot */
	game->contract_cycle[slot] = 0xFF;

	/* Find new villain */
	for (i = game->max_contract; i < MAX_VILLAINS; i++) {
		if (game->villain_caught[i]) continue;
		/* Save villain in slot */
		game->contract_cycle[slot] = i;
		break;
	}
	/* Start further next time */
	game->max_contract++;
}

/* Calculate and return army leadership */
int army_leadership(KBgame *game, byte troop_id) {
	int free_leadership = game->leadership;

	int i;
	for (i = 0; i < 5; i++) {
		if (game->player_troops[i] == troop_id) {
			free_leadership -= troops[troop_id].hit_points * game->player_numbers[i];
			break;	
		}
	}

	return free_leadership;
}

/* Calculate and return morale for troop */
byte troop_morale(KBgame *game, byte slot) {
	int j;
	byte morale_cnv[3] = { MORALE_LOW, MORALE_NORMAL, MORALE_HIGH };
	byte morale = MORALE_HIGH;
	//if (game->player_numbers[i] == 0) break;
	byte troop_id = game->player_troops[slot];
	byte groupI = troops[ troop_id ].morale_group;
	for (j = 0; j < 5; j++) {
		if (game->player_numbers[j] == 0) break;
		//if (i == j) continue;
		byte ctroop_id = game->player_troops[j];
		byte groupJ = troops[ ctroop_id ].morale_group;
		byte nm = morale_chart[groupJ][groupI];
		if (morale_cnv[nm] < morale_cnv[morale]) morale = nm;
	}
	return morale;
}

/* Return total number of spells known */ 
int known_spells(KBgame *game) {
	int i;
	int spells = 0;
	for (i = 0; i < MAX_SPELLS; i++) {
		spells += game->spells[i];
	}
	return spells;
}

/* Return 1 if any of the artifacts in player's possession have "power"; 0 otherwise */
int has_power(KBgame *game, byte power) {
	int i;
	for (i = 0; i < MAX_ARTIFACTS; i++) {
		if (game->artifact_found[i] && (artifact_powers[i] & power))
			return 1;
	}
	return 0;
}

/* Return cost of boat rental, in gold */
word boat_cost(KBgame *game) {
	if (has_power(game, POWER_CHEAPER_BOAT_RENTAL)) return COST_BOAT_CHEAP;
	return COST_BOAT_EXPENSIVE;
}

/* Return 1 if player has boat, 0 if not */
int player_has_boat(KBgame *game) {
	if (game->boat == 0xFF) return 0;
	return 1;
}

/* Return 1 if player can fly, 0 if not */
int player_can_fly(KBgame *game) {
	int i;
	int slots = 0;
	for (i = 0; i < 5; i++) {
		if (game->player_numbers[i] == 0) break;
		/* Non-flying troop */
		if (!(troops[game->player_troops[i]].abilities & ABIL_FLY)) {
			KB_stdlog("Can'f fly because %s do not fly\n", troops[game->player_troops[i]].name);
			return 0;
		}
		/* Flying, but unskilled troop */
		if (troops[game->player_troops[i]].skill_level < 2) {
			KB_stdlog("Can'f fly because %s are not skilled enough\n", troops[game->player_troops[i]].name);
			return 0;
		}
	}
	return 1;
}

/* Return number of free player army slots, 0 if full */
int player_army_slots(KBgame *game) {
	int i;
	int slots = 0;
	for (i = 0; i < 5; i++) {
		if (game->player_numbers[i] == 0) {
			slots++;
		}
	}

	return slots;
}

/* Return total number of troops in player army */
int player_army(KBgame *game) {
	int followers = 0;
	int i;
	for (i = 0; i < 5; i++) {
		followers += game->player_numbers[i];
	}
	return followers;
}

/* Return number of castles currently owned by player */
int player_castles(KBgame *game) {
	int castles = 0;
	int i;
	for (i = 0; i < MAX_CASTLES; i++) {
		if (game->castle_owner[i] == 0xFF)
			castles++;
	}
	return castles;
}

/* Return total number of villains captured by player */
int player_captured(KBgame *game) {
	int num = 0;
	int i;
	for (i = 0; i < MAX_VILLAINS; i++) {
		if (game->villain_caught[i])
			num++;
	}
	return num;
}

/* Return total number of artifacts found by player */
int player_num_artifacts(KBgame *game) {
	int num = 0;
	int i;
	for (i = 0; i < MAX_ARTIFACTS; i++) {
		if (game->artifact_found[i])
			num++;
	}
	return num;
}

/* Return player's commission */
int player_commission(KBgame *game) {
	return game->commission;
}

/* Calculate and return player's score */
int player_score(KBgame *game) {
	int difficulty_modifier[5] = { 0, 1, 2, 4, 8 };
	int score = 
	((player_captured(game) * 500) 
	+ (player_num_artifacts(game) * 250)
	+ (player_castles(game) * 100) 
	- (game->followers_killed * 1));
	if (!game->difficulty)	/* to avoid float math */
		score /= 2;
	else
		score *= difficulty_modifier[game->difficulty];
	if (score < 0) score = 0;
	return score;
}

void player_accept_rank(KBgame *game) {
	game->base_leadership += classes[game->class][game->rank].leadership;
	game->max_spells += classes[game->class][game->rank].max_spell;
	game->spell_power += classes[game->class][game->rank].spell_power;
	game->commission += classes[game->class][game->rank].commission;
	game->knows_magic += classes[game->class][game->rank].knows_magic;
}

void promote_player(KBgame *game) {
	if (game->rank >= MAX_RANKS - 1) return;

	game->rank += 1;

	player_accept_rank(game);
}

void clear_fog(KBgame *game) {
	int i;
	int j;
	for (j = -2; j < 3; j++) {
		for (i = -2; i < 3; i++) {
			int x = game->x + i;
			int y = game->y + j;
			if (x < 0) x = 0;
			if (x > LEVEL_W - 1) x = LEVEL_W - 1;
			if (y < 0) y = 0;
			if (y > LEVEL_H - 1) y = LEVEL_H - 1;
			game->fog[game->continent][y][x] = 1;
		}
	}
}

void sail_to(KBgame *game, byte continent) {
	game->continent = continent;
	game->x = continent_entry[game->continent][0];
	game->y = continent_entry[game->continent][1];

	game->last_x = game->x;
	game->last_y = game->y;

	game->boat = game->continent;
	game->boat_x = game->x;
	game->boat_y = game->y;
}

/* Move foe "foe_id" to a new location "nx,ny" */
void foe_relocate(KBgame *game, int foe_id, int nx, int ny) {

	int old_x = game->foe_coords[game->continent][foe_id][0];
	int old_y = game->foe_coords[game->continent][foe_id][1];

	game->map[game->continent][old_y][old_x] = 0;//TILE_GRASS;
	game->map[game->continent][ny][nx] = 0x91;//TILE_FOE;

	game->foe_coords[game->continent][foe_id][0] = nx;
	game->foe_coords[game->continent][foe_id][1] = ny;
}

void foe_closest_offset(KBgame *game, int foe_id, int position_x, int position_y, int target_x, int target_y, int *ox, int *oy);

void foe_follow(KBgame *game, int foe_id) {
	int foe_x = game->foe_coords[game->continent][foe_id][0];
	int foe_y = game->foe_coords[game->continent][foe_id][1];

	int move_x;
	int move_y;
	foe_closest_offset(game, foe_id,
		foe_x, // foe x
		foe_y, // foe y
		game->last_x, // player x
		game->last_y, // player y
		&move_x, &move_y);

	foe_relocate(game, foe_id, foe_x + move_x, foe_y + move_y);
}

/* Note: foes in radius of player origin move to player origin (last_x,last_y).
 * They really do follow. */
void foes_follow(KBgame *game) {
	int id = 0;
	int i;
	/* For each foe, that is ON-SCREEN */
	for (i = 0; i < MAX_FOES; i++) {
		/* If foe is dead (no tile), ignore it */
		{
			int x = game->foe_coords[game->continent][i][0];
			int y = game->foe_coords[game->continent][i][1];
			if (game->map[game->continent][y][x] != 0x91) continue;
		}
		int diff_x = game->foe_coords[game->continent][i][0] - game->last_x;
		int diff_y = game->foe_coords[game->continent][i][1] - game->last_y;
		/* Poor man's abs() */
		if (diff_x < 0) diff_x = -diff_x;
		if (diff_y < 0) diff_y = -diff_y;
		/* Too far */
		if (diff_y > 2 || diff_x > 2) continue;

		/* Do follow */
		foe_follow(game, i);
	}
}

/* Returns 0 on success, 1 if no slots were found */
int add_troop(KBgame *game, byte troop_id, word number) {
	int i;
	int slot = -1;

	for (i = 0; i < 5; i++) {
		if (game->player_troops[i] == troop_id) {
			slot = i;
			break;
		}
		if (game->player_numbers[i] == 0) {
			slot = i;
			break;
		} 
	}

	if (slot == -1) {
		return 1;
	}

	game->player_troops[slot] = troop_id;
	game->player_numbers[slot] += number;
	return 0;
}

/* Returns 1 if not enough gold, 2 if no slots left */
int buy_troop(KBgame *game, byte troop_id, word number) {
	int i;

	int cost = troops[troop_id].recruit_cost * number;
	
	if (cost > game->gold) {
		return 1;
	}

	if (add_troop(game, troop_id, number)) {
		/* Failed to add */
		return 2;
	}

	game->gold -= troops[troop_id].recruit_cost * number;

	return 0;
}

void dismiss_troop(KBgame *game, byte slot) {
	int i;
	for (i = slot; i < 4; i++) {
		game->player_troops[i] = game->player_troops[i + 1];
		game->player_numbers[i] = game->player_numbers[i + 1];
	}
	game->player_troops[4] = 0xFF;
	game->player_numbers[4] = 0;
}

/* Return 0 on success, 1 if no slots left, 2 if players last army */
int garrison_troop(KBgame *game, int castle_id, byte slot) {
	int i;
	int dst_slot = -1;

	/* If it's player's last troop, fail */
	if (game->player_troops[1] == 0xFF
	 || game->player_numbers[1] == 0) {
		return 2;
	}

	/* Find same troop OR empty slot */
	for (i = 0; i < 5; i++) {
		if (game->castle_troops[castle_id][i] == game->player_troops[slot]) {
			dst_slot = i;
			break;
		}
		if (game->castle_numbers[castle_id][i] == 0) {
			dst_slot = i;
			break;
		}
	}

	/* No slot found */
	if (dst_slot == -1) {
		return 1;
	}

	/* Add to castle */
	game->castle_troops[castle_id][dst_slot] = game->player_troops[slot];
	game->castle_numbers[castle_id][dst_slot] += game->player_numbers[slot];
	/* Remove from player */
	dismiss_troop(game, slot);

	return 0;
}
/* Return 0 on success, 1 if no slots left */
int ungarrison_troop(KBgame *game, int castle_id, byte slot) {
	int i;

	/* Add to player */
	if (add_troop(game,
		game->castle_troops[castle_id][slot],
		game->castle_numbers[castle_id][slot])) {
		/* Failed to add */
		return 1;
	}

	/* Remove from castle */
	for (i = slot; i < 4; i++) {
		game->castle_troops[castle_id][i] = game->castle_troops[castle_id][i + 1];
		game->castle_numbers[castle_id][i] = game->castle_numbers[castle_id][i + 1];
	}
	game->castle_troops[castle_id][4] = 0xFF;
	game->castle_numbers[castle_id][4] = 0;

	return 0;
}

void temp_death(KBgame *game) {
	int i;

	/* Teleport back home */
	game->continent = special_coords[SP_HOME][0];
	game->last_x = game->x = special_coords[SP_HOME][1];
	game->last_y = game->y = special_coords[SP_HOME][2] - 1;
	game->mount = KBMOUNT_RIDE;

	/* Wipe army */
	for (i = 0; i < 5; i++) {
		game->player_troops[i] = 0xFF;
		game->player_numbers[i] = 0;
	}

	/* Take away siege weapons */
	game->siege_weapons = 0;

	/* Give free peasants */
	game->player_troops[0] = 0; /* MAGIC NUMBER for "Peasants" */
	game->player_numbers[0] = 20;
}

byte end_week(KBgame *game) {
	int i, j, cont;
	dword credit;
	byte creature;

	/* Reset player */
	game->time_stop = 0;
	game->leadership = game->base_leadership;

	/* Pick creature */
	if (week_id(game) % 4 == 0)
		creature = 0;
	else
		creature = KB_rand(1, MAX_TROOPS - 1);

	/** Budget **/
	credit = 0;

	/* Add commission */
	game->gold += game->commission;

	/* Count Boat */
	credit += player_has_boat(game) ? boat_cost(game) : 0;

	/* Count Army */
	for (i = 0; i < 5; i++) {
		if (game->player_numbers[i] == 0) break;
		credit +=
			game->player_numbers[i] * (troops[ game->player_troops[i] ].recruit_cost / 10);
	}

	/* Spend gold */
	spend_gold(game, credit);

	/** Turn ghosts into peasants **/
	if (creature == 0)
		for (i = 0; i < 5; i++)
			if (troops[game->player_troops[i]].abilities & ABIL_ABSORB)
				game->player_troops[i] = creature;

	/** World events **/

	/* Repopulate castles */
	for (j = 0; j < MAX_CASTLES; j++)
		if (game->castle_owner[j] == 0xFF) /* Owned by player */
			if (game->castle_numbers[j][0] == 0) /* and is empty */
				repopulate_castle(game, j);

	/* Unit growth (dwellings, castles and foes) */
	for (cont = 0; cont < MAX_CONTINENTS; cont++) 
	{
		for (i = 0; i < MAX_DWELLINGS; i++)
			if (game->dwelling_troop[cont][i] == creature)
				game->dwelling_population[cont][i] = troops[creature].max_population;

		for (j = 0; j < MAX_FOES; j++)
			for (i = 0; i < 3; i++)
				if (game->foe_troops[cont][j][i] == creature)
					game->foe_numbers[cont][j][i] += troops[creature].growth;
	}
	for (j = 0; j < MAX_CASTLES; j++)
		if (game->castle_owner[j] != 0xFF) /* Not owned by player */
			for (i = 0; i < 5; i++)
				if (game->castle_troops[j][i] == creature)
					game->castle_numbers[j][i] += troops[creature].growth;

	return creature;
}

/** Combat **/

/* Return 0 when player is out of his army, 1 otherwise */
int test_defeat(KBgame *game, KBcombat *war) {
	if (player_army(game) == 0) {
		return 0;
	}
	return 1;
}
/* Return 0 when computer is out of his army, 1 otherwise */
int test_victory(KBcombat *war) {
	int i;
	int side = 1;
	for (i = 0; i < MAX_UNITS; i++) {
		if (war->units[side][i].count) return 1;
	}
	return 0;
}

void prepare_units_player(KBcombat *war, int side, KBgame *game) {
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	{
		war->units[side][i].troop_id = game->player_troops[i];
		war->units[side][i].count = game->player_numbers[i];
		war->units[side][i].max_count = war->units[side][i].count;
		war->units[side][i].y = i;
		war->units[side][i].x = side * (CLEVEL_W - 1);

		war->spoils[side] += (troops[war->units[side][i].troop_id].spoils_factor * 5) * war->units[side][i].count;
	}
	war->heroes[side] = game;
	war->powers[side] = 0;
	for (i = 0; i < MAX_ARTIFACTS; i++) {
		if (game->artifact_found[i]) {
			war->powers[side] |= artifact_powers[i];
		}
	}
} 

void prepare_units_foe(KBcombat *war, int side, KBgame *game, int continent_id, int foe_id) {
	int i;
	int max_troops = MAX_UNITS;
	if (max_troops > 3) max_troops = 3;
	war->spoils[side] = 0;
	for (i = 0; i < max_troops; i++)
	{
		war->units[side][i].troop_id = game->foe_troops[continent_id][foe_id][i];
		war->units[side][i].count = game->foe_numbers[continent_id][foe_id][i];
		war->units[side][i].max_count = war->units[side][i].count;

		war->units[side][i].y = i;
		war->units[side][i].x = side * (CLEVEL_W - 1);
		
		war->spoils[side] += (troops[war->units[side][i].troop_id].spoils_factor * 5) * war->units[side][i].count;
	}
	war->heroes[side] = NULL;
	war->powers[side] = 0;
} 

void prepare_units_castle(KBcombat *war, int side, KBgame *game, int castle_id) {
	int i;
	war->spoils[side] = 0;
	for (i = 0; i < MAX_UNITS; i++)
	{
		war->units[side][i].troop_id = game->castle_troops[castle_id][i];
		war->units[side][i].count = game->castle_numbers[castle_id][i];
		war->units[side][i].max_count = war->units[side][i].count;
		war->units[side][i].y = i;
		war->units[side][i].x = 0;

		war->spoils[side] += (troops[war->units[side][i].troop_id].spoils_factor * 5) * war->units[side][i].count;
	}
	war->heroes[side] = NULL;
	war->powers[side] = 0;
} 

void accept_units_player(KBgame *game, int side, KBcombat *war) {
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	{
		game->player_troops[i] = war->units[side][i].troop_id;
		game->player_numbers[i] = war->units[side][i].count;
	}
}

void accept_units_foe(KBgame *game, int side, KBcombat *war, int continent_id, int foe_id) {
	int i;
	int max_troops = MAX_UNITS;
	if (max_troops > 3) max_troops = 3;
	for (i = 0; i < max_troops; i++)
	{
		game->foe_troops[continent_id][foe_id][i] = war->units[side][i].troop_id; 
		game->foe_numbers[continent_id][foe_id][i] = war->units[side][i].count;
	}
}

void accept_units_castle(KBgame *game, int side, KBcombat *war, int castle_id) {
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	{
		game->castle_troops[castle_id][i] = war->units[side][i].troop_id;
		game->castle_numbers[castle_id][i] = war->units[side][i].count;
	}
}


void reset_turn(KBcombat *war) {
	int i, j;
	for (j = 0; j < MAX_SIDES; j++)
	for (i = 0; i < MAX_UNITS; i++) {
		war->units[j][i].turn_count = war->units[j][i].count;
		war->units[j][i].retaliated = 0;
		war->units[j][i].acted = 0;
		war->units[j][i].moves = troops[war->units[j][i].troop_id].move_rate;
		war->units[j][i].flights = 0;
		if (troops[war->units[j][i].troop_id].abilities & ABIL_FLY)
			war->units[j][i].flights = 2;
		/* End-of-turn abilities */
		if (war->side == 1) {
			/* Regen */
			if (troops[war->units[j][i].troop_id].abilities & ABIL_REGEN)
				war->units[j][i].injury = 0;
		}
	}
	war->phase = 0;
	war->spells = 0;
} 

void wipe_battlefield(KBcombat *war) {
	int i, j;
	for (j = 0; j < CLEVEL_H; j++)
	for (i = 0; i < CLEVEL_W; i++)
			war->omap[j][i] = 0;
		
}

int castle_umap[5][6] = {
	{ 0, 8, 6, 7, 9, 0 },
	{ 0, 0,10, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 5, 3, 4, 0, 0 },
	{ 0, 0, 1, 2, 0, 0 },
};

int castle_omap[5][6] = {
	{ 8, 0, 0, 0, 0, 9 },
	{ 8, 0, 0, 0, 0, 9 },
	{ 8, 0, 0, 0, 0, 9 },
	{ 8, 0, 0, 0, 0, 9 },
	{ 5, 7, 0, 0,10, 6 },
};

void reset_match(KBcombat *war, int castle) {

	int i, j;

	/* Move units to their castle positions? */
	if (castle) {
		for (j = 0; j < CLEVEL_H; j++)
		for (i = 0; i < CLEVEL_W; i++) {
			int id, side;
			id = castle_umap[j][i] - 1;
			side = 0;
			if (id < 0) continue;
			if (id >= MAX_UNITS) {
				id -= MAX_UNITS;
				side = 1;
			}
			KBunit *u = &war->units[side][id];
			u->x = i;
			u->y = j;
		}
	}

	/* Place units */
	for (j = 0; j < MAX_SIDES; j++) 
	for (i = 0; i < MAX_UNITS; i++)
	{
		KBunit *u = &war->units[j][i];
		if (!u->count) continue;
		u->shots = troops[u->troop_id].ranged_ammo;
		KB_debuglog(0, "Unit: %d, %d, ID: %d\n", u->x, u->y, (j * MAX_UNITS) + i + 1);
		war->umap[u->y][u->x] = (j * MAX_UNITS) + i + 1;
	}

	wipe_battlefield(war);

	/* Use predefined layout */
	if (castle)
	{
		for (j = 0; j < CLEVEL_H; j++)
		for (i = 0; i < CLEVEL_W; i++) {
			war->omap[j][i] = castle_omap[j][i];
		}	
	}
	else
	/* Generate obstacles */
	{
		/* Each tile has a 1 in 20 chance of having an obstacle in it
		 * A) That IS NOT how it was done in original KB
		 * B) That has a FAIR CHANCE of flooding whole level
		 * //TODO: Fixit ofcourse.
		 */
		for (j = 0; j < CLEVEL_H; j++)
		for (i = 1; i < CLEVEL_W - 2; i++) {
			war->omap[j][i] = 0; 
			if (!(rand()%10))
				war->omap[j][i] = rand()%3 + 1;
		}
	}

	/* Also reset turn */
	reset_turn(war);
}

/* Switch sides */
int next_turn(KBcombat *war) {
	int i, ok = 0;
	war->turn++;
	reset_turn(war);
	for (i = 0; i < MAX_UNITS; i++) {
		KBunit *u = &war->units[war->side][i];
		if (u->count) {
			war->unit_id = i;
			war->side = 1 - war->side;
			ok = 1;
			break;
		}
	}
	return ok;
}

/* Find next suitable unit */
int next_unit(KBcombat *war) {

	int i;
	for (i = war->unit_id + 1; i < MAX_UNITS; i++) {
		KBunit *u = &war->units[war->side][i];
		if (!u->count) continue;
		if (u->acted) continue;
		return i;
	}
	war->phase++;
	for (i = 0; i < MAX_UNITS; i++) {
		KBunit *u = &war->units[war->side][i];
		if (!u->count) continue;
		if (u->acted) continue;
		return i;
	}

	return -1;
}

/* Returns 1 if unit "other_id" is touching unit "side/id", 0 otherwise */
int unit_touching(KBcombat *war, int side, int id, int other_id) {

	KBunit *u = &war->units[side][id];
	KBunit *other = &war->units[1 - side][other_id];

	int diff_x = u->x - other->x;
	int diff_y = u->y - other->y;
	/* Poor man's abs() */
	if (diff_x < 0) diff_x = -diff_x;
	if (diff_y < 0) diff_y = -diff_y;
	/* Too far */
	if (diff_y > 1 || diff_x > 1) return 0;

	return 1;
}

/* See if any enemy units are touching this one */
int unit_surrounded(KBcombat *war, int side, int id) {
	int i;
	for (i = 0; i < MAX_UNITS; i++) {
		if (!war->units[1-side][i].count) continue;
		if (unit_touching(war, side, id, i)) return 1;
	}
	return 0;
}

word units_killed(dword damage, byte hp) {
	return damage / hp;
}

word damage_remainder(dword damage, byte hp) {
	return damage % hp;
}

/* Compact battlefield, after a unit has died */
int compact_units(KBcombat *war) {
	KBunit *u;
	int i, j, k;

	/* a) Wipe unit map */
	for (j = 0; j < CLEVEL_H; j++) {
		for (i = 0; i < CLEVEL_W; i++) {
			war->umap[j][i] = 0;
		}
	}

	/* b) Compact! */
	for (j = 0; j < MAX_SIDES; j++) {
		for (i = 0; i < MAX_UNITS - 1; i++) {
			if (!war->units[j][i].count) {
				//printf("Unit %c, %d is dead!\n", j + 'A', i);
				for (k = i; k < MAX_UNITS - 1; k++) {

					u = &war->units[j][k];

					memcpy(&war->units[j][k], &war->units[j][k+1], sizeof(KBunit));
					//printf("Copying %d to %d\n", k+1, k);
					war->units[j][k+1].count = 0;
				}
			}
		}
		if (war->heroes[j]) {
			accept_units_player(war->heroes[j], j, war);
		}
	}

	/* c) Rewrite unit map */
	for (j = 0; j < MAX_SIDES; j++) {
		for (i = 0; i < MAX_UNITS; i++) {
			if (!war->units[j][i].count) break;
			u = &war->units[j][i];
			war->umap[u->y][u->x] = j * MAX_UNITS + i + 1;
		}
	}

	return 1;
}

/* Deal damage */
int deal_damage(KBcombat *war, int a_side, int a_id, int t_side, int t_id, int is_ranged, int is_external, int external_damage, int retaliation)
{
	/* NOTE:
		Some, commented, parts of this function are missing functionality.
		(Player stats update, druid/archmage damage?)
		
		Otherwise, this is 100% faithfull damage function.
	*/
	KBunit *u = &war->units[a_side][a_id];//attacking unit
	KBunit *t = &war->units[t_side][t_id];//   target unit

	int demon_kills = 0;	/* Special stack-halving ability */

	int final_damage = 0;
	int kills = 0;
	int injury = 0;

	if (!retaliation && !is_external) {
	//	u->turn_count = u->count;
	//	t->turn_count = t->count;
	}

	if (is_external) {
		//magic-vs-unit
		u = NULL;
		final_damage = external_damage;
	}
	else {
		//unit-vs-unit
		int dmg;
		int total;
		int skill_diff;

		if (troops[u->troop_id].abilities & ABIL_SCYTHE) {
			if (KB_rand(1, 100) > 89) //10% chance
				demon_kills = t->count / 2 + (t->count % 2 ? 1 : 0); //ceiled /2
		}

		if (is_ranged) {
			retaliation = 1;
			u->shots--;

			if (troops[u->troop_id].abilities & ABIL_MAGIC) {
				if (troops[t->troop_id].abilities & ABIL_IMMUNE) //cancel_attack
					return -1;
				//dmg = 10 druid, 25 archmage
				dmg = troops[u->troop_id].ranged_min;
			}
			else {
				dmg = KB_rand(troops[u->troop_id].ranged_min, troops[u->troop_id].ranged_max);
			}
		}
		else {
			dmg = KB_rand(troops[u->troop_id].melee_min, troops[u->troop_id].melee_max);
		}

		total = dmg * u->turn_count;
		skill_diff = troops[u->troop_id].skill_level + 5 - troops[t->troop_id].skill_level;
		final_damage = (total * skill_diff) / 10;

		if (war->heroes[a_side]) {
			if (army_leadership(war->heroes[a_side], a_id) > 0) {//unit_under_control(war, a_side, a_id)) {
				byte morale = troop_morale(war->heroes[a_side], a_id); // 0, 1, 2
				if (morale == 1) { //low == * 0.5f
					final_damage /= 2;
				}
				if (morale == 2) { //high == * 1.5f
					final_damage +=
					final_damage / 2;
				}
			}
		}

		if (war->powers[a_side] & POWER_INCREASED_DAMAGE) {
			final_damage +=
			final_damage / 2;
			//same as * 1.5f
		}

		if (war->powers[t_side] & POWER_QUARTER_PROTECTION) {
			final_damage /= 4;
			final_damage *= 3;
			//almost same as multiplying 0.75, but more brutal, as div by 4 can yield 0
		}
	}

	final_damage += t->injury; /* Old damage */
	final_damage += troops[t->troop_id].hit_points * demon_kills; /* Demonic kills */

	kills = units_killed(final_damage, troops[t->troop_id].hit_points);
	injury = damage_remainder(final_damage, troops[t->troop_id].hit_points);

	t->injury = injury;

	if (kills < t->count) {
		//stack survives

		t->count -= kills;

	}
	else {
		//stack dies

		t->dead = 1;
		t->count = 0;

		kills = t->turn_count;
		final_damage = kills * troops[t->troop_id].hit_points;

	}

	if (war->heroes[t_side])
		war->heroes[t_side]->followers_killed += kills;

	if (!is_external)
	{
		//undead powers:
		if (troops[u->troop_id].abilities & ABIL_ABSORB) { //ghosts

			u->count += kills;

		}
		if (troops[u->troop_id].abilities & ABIL_LEECH) { //vampires

			u->count += units_killed(final_damage, troops[u->troop_id].hit_points);

			if (u->count > u->max_count) {
				u->count = u->max_count;
				u->injury = 0;
			}
		}

		if (!retaliation) {
			if (!t->retaliated) {
				t->retaliated = 1;
				deal_damage(war, t_side, t_id, a_side, a_id, 0, 0, 0, 1); //recursive
			}
		}
	}

	//compact_units(war);

	/* Returns number of wholesome kills */
	return kills;
}

int unit_hit_unit(KBcombat *war, int side, int id, int other_side, int other_id)
{
//	KBunit *u = &war->units[side][id];
//	KBunit *other = &war->units[other_side][other_id];

	int kills = deal_damage(war, side, id, other_side, other_id, 0, 0, 0, 1);

	return kills;
}

/* Calculate and deal "ranged" damage, return number of kills */
int unit_ranged_shot(KBcombat *war, int side, int id, int other_side, int other_id) {

	int kills = deal_damage(war, side, id, other_side, other_id, 1, 0, 0, 0);

	return kills;
}

/* Move unit "side/id" to a new location "nx,ny" */
void unit_relocate(KBcombat *war, int side, int id, int nx, int ny) {
	KBunit *u = &war->units[side][id];

	war->umap[u->y][u->x] = 0;
	war->umap[ny][nx] = (side * MAX_UNITS) + id + 1;

	u->x = nx;
	u->y = ny;
}

/* NOTE: DOS version did a different sort of distance calculation.
 * TODO: fix it to match! */

/* Calculate square root with integer-only math. */
/* from: http://www.dsprelated.com/showmessage/65265/1.php message by Steve */
/* NOTE: this code is probably not portable, we should see to it */
dword isqrt32(dword h) {
	dword x;
	dword y;
	int i;

	/* The answer is calculated as a 32 bit value, where the last
	   16 bits are fractional. */
	x =	y = 0;
	for (i = 0; i < 32;  i++)
	{
		x = (x << 1) | 1;
		if (y < x) x -= 2;
		else       y -= x;
		x++;
		y <<= 1;
		if ((h & 0x80000000)) y |= 1;
		h <<= 1;
		y <<= 1;
		if ((h & 0x80000000)) y |= 1;
		h <<= 1;
	}
	return x;
}

/* Integer POWER-OF-TWO "function" */
#define ipow2(a) ((a) * (a))

/* Shorthand Pythagorean distance formula (integer-based) */
#define calc_distance(x1, y1, x2, y2) isqrt32(ipow2((x2) - (x1)) + ipow2((y2) - (y1)))

/* Find a tile around position_x/position_y, which has the minimal distance
 * between target_x/target_y and origin_x/origin_y.
 * Thus:
 * For walking units, position_x/position_y must be == origin_x/origin_y
 * For flying units, position_x/position_y must be == target_x/target_y
 *
 * Returns offset between picked_x/picked_y and origin_x/origin_y
 * or 0, 0 if nothing suitable was found.
 * */ 
void unit_closest_offset(KBcombat *war, int side, int id, int position_x, int position_y, int origin_x, int origin_y, int target_x, int target_y, int *ox, int *oy) {
	int i, j;

	//KBunit *u = &war->units[side][id];
	//char *name = troops[u->troop_id].name;

	dword max_dist = calc_distance(CLEVEL_W + 1, CLEVEL_H + 1, 0, 0); 

	dword picked_dist = max_dist;
	int picked_x;
	int picked_y;
	for (j = -1; j < 2; j++) {
		for (i = -1; i < 2; i++) {
			if (position_x + i < 0) continue;
			if (position_y + j < 0) continue;
			if (position_x + i >= CLEVEL_W) continue;
			if (position_y + j >= CLEVEL_H) continue;

			//printf("%s probe %d, %d\n", name, i, j);

			dword dist = calc_distance(position_x + i, position_y + j, target_x, target_y);
			//float dist = sqrt( pow(other->x - (start_x+i), 2) + pow(other->y - (start_y+j), 2) );

			if (war->omap[position_y + j][position_x + i]) dist = max_dist; /* Obstacle */
			if ((position_x != origin_x || position_y != origin_y)
			&& war->umap[position_y + j][position_x + i]) dist = max_dist; /* Enemy, but we're not interested */
			if (war->umap[position_y + j][position_x + i] - 1 >= MAX_UNITS
			&& war->umap[position_y + j][position_x + i] - 1 != side * MAX_UNITS + id) dist = max_dist; /* Friend */

			//printf("Dist is: %d (%08x) vs [%08x]\n", dist, dist, picked_dist);

			if (dist < picked_dist) {
				picked_dist = dist;
				picked_x = position_x + i;
				picked_y = position_y + j;
			}
		}
	}

	if (picked_dist < max_dist) {
		/* Tile is good */
		*ox = picked_x - origin_x;
		*oy = picked_y - origin_y;
	} else {
		/* Tile is unwalkable */
		*ox = 0;
		*oy = 0;
	}

}

/* Find a tile that can move unit closer to it's target */
void unit_move_offset(KBcombat *war, int side, int id, int other_full_id, int *ox, int *oy) {
	int other_side;
	int other_id;
	int i, j;

	KBunit *u, *other;

	other_side = 0;
	other_id = other_full_id;

	if (other_id >= MAX_UNITS) {
		other_id -= MAX_UNITS;
		other_side = 1;
	}

 	u = &war->units[side][id];
 	other = &war->units[other_side][other_id];
 	char *name = troops[u->troop_id].name;

	unit_closest_offset(war, side, id, u->x, u->y, u->x, u->y, other->x, other->y, ox, oy);
}

/* Find a tile that can fly a unit closer to it's target */
void unit_fly_offset(KBcombat *war, int side, int id, int other_full_id, int *tx, int *ty) {
	int nx, ny;
	int other_side;
	int other_id;
	int i;

	KBunit *u, *other;

	other_side = 0;
	other_id = other_full_id;

	if (other_id >= MAX_UNITS) {
		other_id -= MAX_UNITS;
		other_side = 1;
	}

 	u = &war->units[side][id];
 	other = &war->units[other_side][other_id];

	int ox, oy;
	
	ox = 0;
	oy = 0;

	unit_closest_offset(war, side, id, other->x, other->y, u->x, u->y, other->x, other->y, &ox, &oy);

	nx = u->x + ox;
	ny = u->y + oy;

	/* Tile is still occupied */
	if (war->omap[ny][nx] || war->umap[ny][nx]) {
		*tx = u->x;
		*ty = u->y;
	} else {
	/* Tile is good */
		*tx = nx;
		*ty = ny;
	}
}

/** For offset calculator **/
/* This is copy-pasted from unit_closets_offset... */
void foe_closest_offset(KBgame *game, int foe_id, int position_x, int position_y, int target_x, int target_y, int *ox, int *oy) {
	int i, j;

	dword max_dist = calc_distance(LEVEL_W + 1, LEVEL_H + 1, 0, 0);

	dword picked_dist = max_dist;
	int picked_x;
	int picked_y;
//printf("--------------------------------------\n");
	for (j = -1; j < 2; j++) {
		for (i = -1; i < 2; i++) {
			if (position_x + i < 0) continue;
			if (position_y + j < 0) continue;
			if (position_x + i >= LEVEL_W) continue;
			if (position_y + j >= LEVEL_H) continue;

			//printf("%s probe %d, %d\n", name, i, j);

			dword dist = calc_distance(position_x + i, position_y + j, target_x, target_y);
			//float dist = sqrt( pow(other->x - (start_x+i), 2) + pow(other->y - (start_y+j), 2) );

			/* Check for obstacles, only when moving */
			if (i || j)
			if (game->map[game->continent][position_y + j][position_x + i]) dist = max_dist; /* Obstacle */
//printf("%d,%d=%d (current picked dist: %d)\n",i,j,dist,picked_dist);
			//printf("Dist is: %d (%08x) vs [%08x]\n", dist, dist, picked_dist);

			if (dist < picked_dist) {
//printf("Picked\n");
				picked_dist = dist;
				picked_x = position_x + i;
				picked_y = position_y + j;
			}
		}
	}

	if (picked_dist < max_dist) {
		/* Tile is good */
		*ox = picked_x - position_x;
		*oy = picked_y - position_y;
	} else {
		/* Tile is unwalkable */
		*ox = position_x;
		*oy = position_y;
	}

}



/** Spell effects **/

void time_stop(KBgame *game) {
	game->time_stop += game->spell_power * 10;
}

void raise_control(KBgame *game) {
	game->leadership += game->spell_power * 100;
}

int magic_damage(KBgame *game, KBcombat *war, int side, int id, word base_damage, byte filter) {

	KBunit *u = &war->units[side][id];
	word damage = base_damage * game->spell_power;

	/* Must be an undead, but is not */
	if (filter && !(troops[u->troop_id].abilities & ABIL_UNDEAD)) {
		return -1;
	}
	/* Immune to magic */
	if (troops[u->troop_id].abilities & ABIL_IMMUNE) {
		return -1;
	}

	int kills = deal_damage(war, side, id, 0, 0, 0, 1, damage, 1);

	return kills;
}

int clone_troop(KBgame *game, KBcombat *war, int unit_id) {
	/* #Clones = SpellPower * 10 + Injury) / Troop.HP, with damage remainder */
	byte hp = troops[war->units[0][unit_id].troop_id].hit_points;
	dword damage = game->spell_power * 10 + war->units[0][unit_id].injury;

	word clones = units_killed(damage, hp);
	word injury = damage_remainder(damage, hp);

	war->units[0][unit_id].count += clones;
	war->units[0][unit_id].max_count += clones;
	war->units[0][unit_id].injury = injury;
	return (int)clones;
}

int instant_troop(KBgame *game, byte *w_troop_id) {
	int i, slot = -1;
	byte troop_id;
	word number;

	troop_id = classes[game->class][game->rank].instant_army;

	for (i = 0; i < 5; i++) {
		if (game->player_troops[i] == troop_id
		|| game->player_numbers[i] == 0) {
			slot = i;
			break;
		}
	}

	if (slot == -1) return 0;

	number = (game->spell_power + 1) * instant_army_multiplier[game->rank];

	game->player_troops[slot] = troop_id;
	game->player_numbers[slot] += number;
	*w_troop_id = troop_id;

	return (int)number;
}

int find_villain(KBgame *game) {
	int i;
	for (i = 0; i < MAX_CASTLES; i++) {

		if ((game->castle_owner[i] & 0x3f) == game->last_contract) {
			game->castle_owner[i] |= KBCASTLE_KNOWN; /* Known */
			return i;
		}

	}
	return -1;
}
