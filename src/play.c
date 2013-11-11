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

void repopulate_castle(KBgame *game, int castle_id) {

}

/* Actual KBgame* allocator */
KBgame *spawn_game(char *name, int pclass, int difficulty) {

	KBgame* game;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	memset(game, 0, sizeof(KBgame));

	KB_strcpy(game->name, name);

	game->class = pclass;
	game->rank = 0;
	game->difficulty = difficulty;
	
	game->contract = 0xFF;

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
	if (has_power(game, POWER_CHEAPER_BOAT_RENTAL)) return 100;
	return 500;
}

/* Return 1 if player has boat, 0 if not */
int player_has_boat(KBgame *game) {
	if (game->boat == 0xFF) return 0;
	return 1;
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
	int i;

	/* Teleport back home */
	game->continent = HOME_CONTINENT;
	game->last_x = game->x = HOME_X;
	game->last_y = game->y = HOME_Y - 1;
	game->mount = KBMOUNT_RIDE;

	/* Wipe army */
	for (i = 1; i < 5; i++) {
		game->player_troops[i] = game->player_troops[i + 1];
		game->player_numbers[i] = game->player_numbers[i + 1];
	}

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

	/* Add commission */
	game->gold += game->commission;

	/* Count Boat */
	credit += player_has_boat(game) ? boat_cost(game) : 0;

	/* Count Army */
	for (i = 0; i < 5; i++) {
		if (game->player_numbers[i] == 0) break;
		credit +=
			game->player_numbers[i] * troops[ game->player_troops[i] ].recruit_cost;
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

		for (j = 0; j < MAX_FOLLOWERS; j++)
			for (i = 0; i < 3; i++)
				if (game->follower_troops[cont][j][i] == creature)
					game->follower_numbers[cont][j][i] += troops[creature].growth;
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
		war->units[side][i].max_count = war->units[side][i].count;
		war->units[side][i].y = i;
		war->units[side][i].x = side * (CLEVEL_W - 1);
	}
	war->heroes[side] = game;
} 

void prepare_units_foe(KBcombat *war, int side, KBgame *game, int continent_id, int foe_id) {
	int i;
	int max_troops = MAX_UNITS;
	if (max_troops > 3) max_troops = 3;
	for (i = 0; i < max_troops; i++)
	{
		war->units[side][i].troop_id = game->follower_troops[continent_id][foe_id][i];
		war->units[side][i].count = game->follower_numbers[continent_id][foe_id][i];
		war->units[side][i].max_count = war->units[side][i].count; 
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
		war->units[j][i].flights = 0;
		if (troops[war->units[j][i].troop_id].abilities & ABIL_FLY)
			war->units[j][i].flights = 2;
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
	{ 0, 0, 0, 0, 0, 0 },
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
				//if (troops[t->troop_id].abilities & ABIL_IMMUNE) //cancel_attack...?
				//dmg = 10 druid, 25 archmage
				dmg = 0;
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
			game->castle_owner[i] |= 0x40; /* Known */
			return i;
		}

	}
	return -1;
}
