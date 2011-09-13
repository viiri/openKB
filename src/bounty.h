/*
 *  bounty.h -- high-level KB abstractions
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
#ifndef _OPENKB_BOUNTY_H
#define _OPENKB_BOUNTY_H

#include "lib/kbsys.h"

#define KBMOUNT_SAIL	0
#define KBMOUNT_FLY 	4
#define KBMOUNT_RIDE	8

#define MAX_CONTINENTS 4
#define LEVEL_W	64
#define LEVEL_H	64

#define MAX_SPELLS (7 * 2)
#define MAX_ARTIFACTS 8

#define MAX_VILLAINS 17
#define MAX_FOLLOWERS (40*4)

#define MAX_DWELLINGS 44
#define MAX_CASTLES 26
#define MAX_TOWNS 26

#define ALCOVE_CONTINENT 0
#define ALCOVE_X 11
#define ALCOVE_Y 19

typedef struct KBgame KBgame;

struct KBgame {
	char name[11];

	byte class;	/* Character class */
	byte rank;	/* Rank, player character's level */
	byte mount;	/* One of KBMOUNT_ defines */

	byte difficulty;

	byte options[6];

	byte spell_power;
	byte max_spells;

	byte continent;
	byte y;
	byte x;

	byte last_y;
	byte last_x;

	byte boat_y;
	byte boat_x;

	byte contract;
	byte siege_weapons;
	byte knows_magic;
	byte boat;

	byte contract_cycle[5];	/* Five villains you can get contract for */
	byte last_contract;	/* Last contract */
	byte max_contract;

	byte artifact_found[MAX_ARTIFACTS];
	byte villain_caught[MAX_VILLAINS];
	byte continent_found[MAX_CONTINENTS];//can sail to continent
	byte orb_found[MAX_CONTINENTS];//can view whole map for continent

	byte spells[MAX_SPELLS];	//player has spells

	byte town_visited[MAX_TOWNS];
	byte castle_visited[MAX_CASTLES];

	byte town_spell[MAX_TOWNS]; /* Which spell is sold in town N ? */

	byte teleport_coords[MAX_CONTINENTS][2];

	byte dwelling_coords[MAX_DWELLINGS][2];
	byte dwelling_troop[MAX_DWELLINGS];	/* Which creature lives in dwelling N ? */ 
	byte dwelling_population[MAX_DWELLINGS]; /* How much of those */

	byte castle_owner[MAX_CASTLES];
	byte castle_troops[MAX_CASTLES][5];	// creature types
	word castle_numbers[MAX_CASTLES][5];// their count

	byte map_coords[MAX_CONTINENTS][2];	/* Special chests  */
	byte orb_coords[MAX_CONTINENTS][2];	/* 4 per continent */

	byte follower_coords[MAX_FOLLOWERS][2];//0 is X and 1 is Y
	byte follower_troops[MAX_FOLLOWERS][5];//creature types
	word follower_numbers[MAX_FOLLOWERS][5];//their count

	byte steps_left;	/* You can make that much steps before day runs out */
	word time_stop;
	word days_left;

	byte player_troops[5];	/* Player army (creature types)	*/
	word player_numbers[5];	/* Player army (troop count) */

	word followers_killed;

	word base_leadership; /* That much leadership * 1,3,5 is awarded on each level up */
	word leadership;

	word commission; /* That much gold + base_commission is awarded each week */
	dword gold;
	
	word score;	/* Current score */

	byte scepter_continent;	/* Find the scepter to win the game! */
	byte scepter_y;	
	byte scepter_x;
	byte scepter_key; /* (un)encrypts those 3 values on load/save */

	byte fog[MAX_CONTINENTS][LEVEL_H][LEVEL_W]; /* Fog of war (i.e. visited map locations) */
	byte map[MAX_CONTINENTS][LEVEL_H][LEVEL_W];

	byte unknown1;	/* Unknown values from the original game */
	byte unknown2;	/* We carry them over to maintain savefile compatibilitity */
};

#define MAX_TROOPS	25

#define MORALE_NORMAL	0
#define MORALE_LOW  	1
#define MORALE_HIGH 	2

#define DWELLING_PLAINS 	0
#define DWELLING_FOREST 	1
#define DWELLING_HILLCAVE  	2
#define DWELLING_DUNGEON	3
#define DWELLING_CASTLE 	4

#define MGROUP_A 	0	/* Smallfolk */
#define MGROUP_B 	1	/* Lords */
#define MGROUP_C	2	/* Creatures */
#define MGROUP_D	3	/* Monsters */
#define MGROUP_E	4	/* Undead */

#define ABIL_FLY	0x01
#define ABIL_REGEN	0x02
#define ABIL_XXX1	0x04
#define ABIL_XXX2	0x08
#define ABIL_ABSORB	0x10
#define ABIL_LEECH	0x20
#define ABIL_XXX3	0x40
#define ABIL_XXX4	0x80

typedef struct KBtroop {

	char name[16];

	byte	skill_level;	/* Base stats */
	byte	hit_points;
	byte	move_rate;

	byte	melee_min;
	byte	melee_max;

	byte	ranged_min;
	byte	ranged_max;
	byte	ranged_ammo;

	word 	recruit_cost;
	word	weekly_cost;

	byte	abilities;		/* Bit-flags, see ABIL_ defines */

	byte	dwells;			/* Which dwelling is it's home */
	byte	max_population;	/* Maximum number per dwelling */
	byte	morale_group;	/* Morale group, use special chart to determine final troop morale */
} KBtroop;

typedef struct KBclass {

	char title[16];

	byte villains_needed;
	word leadership;

	byte max_spell;
	byte spell_power;

	word commission;
	byte knows_magic;
} KBclass;

/* Data provided by bounty.c */
extern KBtroop troops[MAX_TROOPS];
extern byte morale_chart[5][5];

extern KBclass classes[4][4];

extern char *dwelling_names[MAX_DWELLINGS];
extern char *town_names[MAX_TOWNS];
extern char *castle_names[MAX_CASTLES];
extern char *spell_names[MAX_SPELLS];
extern char *number_names[6];
extern word number_mins[6];
extern char *number_name(word num);
extern char *morale_names[3];
extern char *continent_names[4];

#endif /* _OPENKB_BOUNTY_H */
