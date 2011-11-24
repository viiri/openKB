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
	return 9999;
}

int buy_troop(KBgame *game, byte troop_id, word number) {

	int slot = -1;
	
	int i;

	int cost = troops[troop_id].recruit_cost * number;
	
	if (cost > game->gold) {
		KB_BottomBox("\n\n\nYou don't have enough gold!", "", 1);
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
		KB_BottomBox("", "No troop slots left!", 1);
		return;
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

	KB_BottomBox("After being disgraced...", "", 1);
}

void test_defeat(KBgame *game) {
	if (player_army(game) == 0) {
		temp_death(game);
	}
}

/** Spell effects **/

void raise_control(KBgame *game) {
	game->leadership += game->spell_power * 100;
}

