/*
 *  play.c -- gameplay mechanics
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
#include "lib/kbstd.h"

/* Actual KBgame* allocator */
KBgame *spawn_game(char *name, int pclass, int difficulty) {

	KBgame* game;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	KB_strcpy(game->name, name);

	game->class = pclass;
	game->rank = 0;
	game->difficulty = difficulty;

	return game;
}

/* Calculate how many days has passed by inverting the "days left" variable */
word passed_days(KBgame *game) {
	word max_days_diff[] = { 900, 600, 400, 200 };

	word max_days = max_days_diff[game->difficulty];

	word pass_days = max_days - game->days_left; 

	return pass_days;
}

/* Calculate and return current week number */
word week_id(KBgame *game) {

	word w_id = passed_days(game) / WEEK_DAYS;

	return w_id;
}

void end_week(KBgame *game) {
	printf("END OF WEEK!\n");
}

void end_day(KBgame *game) {
	game->days_left -= 1;
	game->steps_left = DAY_STEPS;
	if (!(game->days_left % WEEK_DAYS)) end_week(game);
}

/* Spend some ammount of game days */
void spend_days(KBgame *game, word days) {
	word i;
	for (i = 0; i < days; i++) {
		end_day(game);
	}
}

/* Spend remaining days in the week */
void spend_week(KBgame *game) {
	word w_id = week_id(game); 
	word pass_days = passed_days(game);

	word week_days = pass_days - (w_id * WEEK_DAYS);
	word end_week_days = WEEK_DAYS - week_days;

	spend_days(game, end_week_days);
}

/* Close contract on villain */
void fullfill_contract(KBgame *game, byte villain_id) {
	int i;
	int slot = -1;

	/** Fullfill contract **/
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

/* Return total number of spells known */ 
int known_spells(KBgame *game) {
	int i;
	int spells = 0;
	for (i = 0; i < MAX_SPELLS; i++) {
		spells += game->spells[i];
	}
	return spells;
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
	return score;
}

int buy_troop(KBgame *game, byte troop_id, word number) {

	int slot = -1;
	
	int i;

	int cost = troops[troop_id].recruit_cost * number;
	
	if (cost > game->gold) {
		return 1;
	}
	
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
		return 2;
	}

	game->player_troops[slot] = troop_id;
	game->player_numbers[slot] += number;

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

void temp_death(KBgame *game) {
	game->continent = HOME_CONTINENT;
	game->x = HOME_X;
	game->y = HOME_Y - 1;

	game->gold += game->commission;

	//redraw screen somehow...
	//KB_BottomBox("After being disgraced...", "", 1);
	//TODO
}


/** Spell effects **/

void raise_control(KBgame *game) {
	game->leadership += game->spell_power * 100;
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
		if (war->units[1][i].count) return 1;
	}
	return 0;
}

void prepare_units_player(KBcombat *war, int side, KBgame *game) {
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	{
		war->units[side][i].troop_id = game->player_troops[i];
		war->units[side][i].count = game->player_numbers[i];
		war->units[side][i].y = i;
		war->units[side][i].x = side * (CLEVEL_W - 1);
	}
} 

void prepare_units_foe(KBcombat *war, int side, KBgame *game, int continent_id, int foe_id) {
	int i;
	int max_troops = MAX_UNITS;
	if (max_troops > 3) max_troops = 3;
	for (i = 0; i < max_troops; i++)
	{
		war->units[side][i].troop_id = game->follower_troops[continent_id][foe_id][i];
		war->units[side][i].count = game->follower_numbers[continent_id][foe_id][i];
		war->units[side][i].y = i;
		war->units[side][i].x = side * (CLEVEL_W - 1);
	}
} 

void prepare_units_castle(KBcombat *war, int side, KBgame *game, int castle_id) {
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	{
		war->units[side][i].troop_id = game->castle_troops[castle_id][i];
		war->units[side][i].count = game->castle_numbers[castle_id][i];
		war->units[side][i].y = i;
		war->units[side][i].x = 0;
	}
} 

void accept_units_player(KBgame *game, int side, KBcombat *war) {
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	{
		game->player_troops[i] = war->units[side][i].troop_id;
		game->player_numbers[i] = war->units[side][i].count;
	}
}

void reset_turn(KBcombat *war) {
	int i, j;
	for (j = 0; j < MAX_SIDES; j++)
	for (i = 0; i < MAX_UNITS; i++) {
		war->units[j][i].acted = 0;
		war->units[j][i].moves = troops[war->units[j][i].troop_id].move_rate;
	}
	war->phase = 0;
} 

void wipe_battlefield(KBcombat *war) {
	int i, j;
	for (j = 0; j < CLEVEL_H; j++)
	for (i = 0; i < CLEVEL_W; i++)
			war->omap[j][i] = 0;
		
}

void reset_match(KBcombat *war) {

	int i, j;

	/* Place units */
	for (j = 0; j < MAX_SIDES; j++) 
	for (i = 0; i < MAX_UNITS; i++)
	{
		KBunit *u = &war->units[j][i];
		if (!u->count) continue;
		war->units[j][i].shots = troops[war->units[j][i].troop_id].ranged_ammo;
		KB_debuglog(0, "Unit: %d, %d, ID: %d\n", u->x, u->y, (j * MAX_UNITS) + i + 1);
		war->umap[u->y][u->x] = (j * MAX_UNITS) + i + 1;
	}

	wipe_battlefield(war);

	/* Generate obstacles */
	{
		/* Each tile has a 1 in 20 chance of having an obstacle in it
		 * A) That IS NOT how it was done in original KB
		 * B) That has a FAIR CHANCE of flooding whole level
		 * //TODO: Fixit ofcourse.
		 */
		for (j = 0; j < CLEVEL_H; j++)
		for (i = 1; i < CLEVEL_W - 2; i++)
			war->omap[j][i] = 0; 
			if (!(rand()%10))
				war->omap[j][i] = rand()%3 + 1;
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
	if (diff_y > 2 || diff_x > 2) return 0; 

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

