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

#define COST_ALCOVE     	5000
#define COST_BOAT_CHEAP 	100
#define COST_BOAT_EXPENSIVE	500
#define COST_SIEGE_WEAPONS	3000

#define KBMOUNT_SAIL	0
#define KBMOUNT_FLY 	4
#define KBMOUNT_RIDE	8

#define KBCASTLE_KNOWN  	0x40
#define KBCASTLE_PLAYER 	0xFF
#define KBCASTLE_MONSTERS	0x7F
#define KBCASTLE_VILLAIN	0x1F

#define MAX_CONTINENTS 4
#define LEVEL_W	64
#define LEVEL_H	64

#define MAX_SPELLS (7 * 2)
#define MAX_ARTIFACTS 8

#define MAX_VILLAINS 17
#define MAX_FOES 40

#define MAX_DWELLINGS 11
#define MAX_CASTLES 26
#define MAX_TOWNS 26

#define MAX_TELECAVES 2

#define MAX_TROOP_DIFFICULTY 4
#define MAX_TROOP_CHANCE_CURVE 5

#define HOME_CONTINENT 0
#define HOME_X 11
#define HOME_Y 7

#define ALCOVE_CONTINENT 0
#define ALCOVE_X 11
#define ALCOVE_Y 19

#define MAX_SPECIAL_PLACES 2
#define SP_HOME 0
#define SP_ALCOVE 1

#define FRIENDLY_FOES 5

#define DAY_STEPS	40
#define WEEK_DAYS	5

#define PUZZLEMAP_W	5
#define PUZZLEMAP_H	5

#define MAX_PLAYER_ARMY 5
#define MAX_CASTLE_ARMY 5
#define MAX_FOE_ARMY 5

typedef struct KBgame KBgame;

struct KBgame {
	char savefile[64];	/* Name of the savefile, not really part of the game */ 
	char name[11]; /* Player name */

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

	byte teleport_coords[MAX_CONTINENTS][MAX_TELECAVES][2];

	byte dwelling_coords[MAX_CONTINENTS][MAX_DWELLINGS][2];
	byte dwelling_troop[MAX_CONTINENTS][MAX_DWELLINGS];	/* Which creature lives in dwelling N ? */ 
	byte dwelling_population[MAX_CONTINENTS][MAX_DWELLINGS]; /* How much of those */

	byte castle_owner[MAX_CASTLES]; /* 0x7F = no one, 0xFF = you, LOW 5 bits = villain */
	byte castle_troops[MAX_CASTLES][5];	// creature types
	word castle_numbers[MAX_CASTLES][5];// their count

	byte map_coords[MAX_CONTINENTS][2];	/* Special chests  */
	byte orb_coords[MAX_CONTINENTS][2];	/* 4 per continent */

	byte foe_coords[MAX_CONTINENTS][MAX_FOES][2];//0 is X and 1 is Y
	byte foe_troops[MAX_CONTINENTS][MAX_FOES][5];//creature types
	word foe_numbers[MAX_CONTINENTS][MAX_FOES][5];//their count

	byte steps_left;	/* You can make that much steps before day runs out */
	word time_stop;
	word days_left;

	byte player_troops[5];	/* Player army (creature types)	*/
	word player_numbers[5];	/* Player army (troop count) */

	word followers_killed; /* Total number of friendly troops lost in combat */

	word base_leadership; /* That much leadership * 1,3,5 is awarded on each level up */
	word leadership;

	word commission; /* That much gold is awarded each week */
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
	byte unknown3;
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
#define ABIL_MAGIC	0x04
#define ABIL_IMMUNE	0x08
#define ABIL_ABSORB	0x10
#define ABIL_LEECH	0x20
#define ABIL_SCYTHE	0x40
#define ABIL_UNDEAD	0x80

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

	word 	recruit_cost;	/* In gold */
	word	spoils_factor;

	byte	abilities;		/* Bit-flags, see ABIL_ defines */

	byte	dwells;			/* Which dwelling is it's home */
	byte	max_population;	/* Maximum number per dwelling */
	byte	growth;			/* Castle and foe troop growth on chosen week */
	byte	morale_group;	/* Morale group, use special chart to determine final troop morale */
} KBtroop;

#define MAX_CLASSES 4
#define MAX_RANKS 4

typedef struct KBclass {

	char title[16];

	byte villains_needed;
	word leadership;

	byte max_spell;
	byte spell_power;

	word commission;
	byte knows_magic;

	byte instant_army;
} KBclass;

#define CLEVEL_H 5
#define CLEVEL_W 6

#define MAX_SIDES 2
#define MAX_UNITS 5

#define MAX_PHASES 2

typedef struct KBunit {
	byte troop_id;
	word count; 	/* number of units in stack */
	word turn_count;/* number of units in stack before damage */
	word max_count;	/* number of units in stack before combat */

	byte dead;
	byte frame;

	word injury;	/* damage counter */
	byte acted;
	byte retaliated;

	byte moves;
	byte shots;
	byte flights;

	byte frozen;

	byte y;
	byte x;
} KBunit;

typedef struct KBcombat {
	KBunit units[MAX_SIDES][MAX_UNITS];

	byte omap[CLEVEL_H + 1][CLEVEL_W + 1];
	byte umap[CLEVEL_H + 1][CLEVEL_W + 1];

	word spoils[MAX_SIDES];
	byte powers[MAX_SIDES];
	KBgame *heroes[MAX_SIDES];

	int your_turn;//TODO: replace with a macro

	byte turn;		/* Turn counter, not really used */
	byte phase;		/* Each player/AI turn might take up to 2 phases */

	byte spells;	/* Spells casted this round */

	byte side;		/* Indexes into units array */
	byte unit_id;	/* units[side][unit_id], obviously */
} KBcombat;

/* Artifact powers */
#define POWER_UNKNOWN_XXX1      	0x01
#define POWER_QUARTER_PROTECTION	0x02
#define POWER_DOUBLE_LEADERSHIP 	0x04
#define POWER_DOUBLE_SPELL_POWER	0x08
#define POWER_INCREASE_COMMISSION	0x10
#define POWER_CHEAPER_BOAT_RENTAL	0x20
#define POWER_DOUBLE_MAX_SPELLS 	0x40
#define POWER_INCREASED_DAMAGE     	0x80

/* Data provided by bounty.c */
extern KBtroop troops[MAX_TROOPS];
extern byte morale_chart[5][5];

extern KBclass classes[MAX_CLASSES][MAX_RANKS];
extern word starting_gold[MAX_CLASSES];
extern byte starting_army_troop[MAX_CLASSES][2];
extern byte starting_army_numbers[MAX_CLASSES][2];
extern byte instant_army_multiplier[MAX_RANKS];

extern byte villains_per_continent[MAX_CONTINENTS];
extern word villain_rewards[MAX_VILLAINS];
extern byte villain_army_troops[MAX_VILLAINS][5];
extern word villain_army_numbers[MAX_VILLAINS][5];

extern word days_per_difficulty[4];
extern char *dwelling_names[MAX_DWELLINGS][32];
extern char *town_names[MAX_TOWNS][32];
extern char *castle_names[MAX_CASTLES + 1][32];
extern byte town_inversion[MAX_CASTLES];
extern char *spell_names[MAX_SPELLS][32];
extern word spell_costs[MAX_SPELLS];
extern char *artifact_names[MAX_ARTIFACTS][32];
extern byte artifact_powers[MAX_ARTIFACTS];
extern byte artifact_inversion[MAX_ARTIFACTS];
extern char *number_names[6];
extern word number_mins[6];
extern char *number_name(word num);
extern byte chance_for_gold[MAX_CONTINENTS];
extern byte chance_for_commission[MAX_CONTINENTS];
extern byte chance_for_spellpower[MAX_CONTINENTS];
extern byte chance_for_maxspell[MAX_CONTINENTS];
extern byte chance_for_newspell[MAX_CONTINENTS];
extern word min_gold[MAX_CONTINENTS];
extern word max_gold[MAX_CONTINENTS];
extern word min_commission[MAX_CONTINENTS];
extern word max_commission[MAX_CONTINENTS];
extern word base_maxspell[MAX_CONTINENTS];
extern char *morale_names[3];
extern char *continent_names[MAX_CONTINENTS][32];

extern byte continent_entry[MAX_CONTINENTS][2];	/* [x][y] */
extern byte castle_difficulty[MAX_CASTLES];
extern byte special_coords[MAX_SPECIAL_PLACES][3]; /* [continent][x][y] */
extern byte castle_coords[MAX_CASTLES][3];	/* [continent][x][y] */
extern byte town_coords[MAX_CASTLES][3];
extern byte towngate_coords[MAX_CASTLES][3];
extern byte boat_coords[MAX_CASTLES][3];

extern byte troop_chance_table[MAX_TROOP_DIFFICULTY][MAX_TROOP_CHANCE_CURVE-1];
extern byte dwelling_to_troop[MAX_TROOP_DIFFICULTY][MAX_TROOP_CHANCE_CURVE];
extern byte troop_numbers[MAX_TROOPS][MAX_TROOP_DIFFICULTY];
extern byte continent_dwellings[MAX_CONTINENTS][MAX_DWELLINGS];
extern byte dwelling_ranges[MAX_CONTINENTS][2];

extern signed char puzzle_map[PUZZLEMAP_H][PUZZLEMAP_W]; /* each piece is covered by villain face or artifact */

#endif /* _OPENKB_BOUNTY_H */
