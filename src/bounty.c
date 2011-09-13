/*
 *  bounty.c -- tables and static data needed in the game
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

#define _FLY ABIL_FLY
#define _ABSORB ABIL_ABSORB
#define _LEECH ABIL_LEECH
#define _REGEN ABIL_REGEN

#define _CASTLE DWELLING_CASTLE
#define _PLAINS DWELLING_PLAINS
#define _FOREST DWELLING_FOREST
#define _HILL DWELLING_HILLCAVE 
#define _CAVE DWELLING_HILLCAVE
#define _DUNGEON DWELLING_DUNGEON

#define _A 0
#define _B 1
#define _C 2
#define _D 3
#define _E 4

	/*  Name		   SL, HP, MV   	Melee	Ranged	Recruitment,Weekly Costs	*
	 *	(ABILITIES)		Dwelling	MoraleGroup      								*/
KBtroop troops[MAX_TROOPS] = {
	{	"Peasants",		1, 1, 1,   		1,1,	0,0,0,  	10,	0,
		(0),        	_PLAINS, 250,	_A,
	},
	{	"Sprites",		1, 1, 1,   		1,2,	0,0,0,  	15,	0,
		(_FLY),			_FOREST, 250,	_C,
	},
	{	"Militia",		2, 2, 2,   		1,2,	0,0,0,  	50,	0,
		(0),        	_CASTLE, 250,	_A,
	},
	{	"Wolves",		2, 3, 3,   		1,3,	0,0,0,  	40,0,	
		(0),        	_PLAINS, 250,	_D,
	},
	{	"Skeletons",	2, 3, 2,   		1,2,	0,0,0,  	40,0,	
		(0),        	_DUNGEON, 250,	_E,
	},
	{	"Zombies",		2, 5, 1,   		2,2,	0,0,0,  	50,0,	
		(0),        	_DUNGEON, 250,	_E,
	},
	{	"Gnomes",   	2, 5, 1,   		1,3,	0,0,0,  	60,0,	
		(0),        	_FOREST, 250,	_C,
	},
	{	"Orcs",			2, 5, 2,   		2,3,	1,2,10, 	75,0,	
		(0),        	_HILL, 250, 	_D,
	},
	{	"Archers",		2, 10, 2,   	1,2,	1,3,12, 	250,0,
		(0),        	_CASTLE, 250,	_B,
	},
	{	"Elves",		3, 10, 3,   	1,2,	2,4,24, 	200,0,	
		(0),        	_FOREST, 250,	_C,
	},
	{	"Pikemen",		3, 10, 2,   	2,4,	0,0,0,  	800,0,
		(0),        	_CASTLE, 250,	_B,
	},
	{	"Nomads",   	3, 15, 2,   	2,4,	0,0,0,  	300,0,	
		(0),        	_PLAINS, 250,	_C,
	},
	{	"Dwarves",		3, 20, 1,   	2,4,	0,0,0,  	350,0,	
		(0),        	_HILL, 250, 	_C,
	},
	{	"Ghosts",		4, 10, 3,   	3,4,	0,0,0,  	400,0,	
		(_ABSORB),  	_DUNGEON, 250,	_E,
	},
	{	"Knights",		5, 35, 1,   	6,10,	0,0,0,  	1000,0,	
		(0),        	_CASTLE, 250,	_B,
	},
	{	"Ogres",		4, 40, 1,   	3,5,	0,0,0,  	750,0,	
		(0),        	_HILL, 250,  	_D,
	},
	{	"Barbarians",	4, 40, 3,   	1,6,	0,0,0,  	750,0,	
		(0),    		_PLAINS, 250,	_C,
	},
	{	"Trolls",		4, 50, 1,   	2,5,	0,0,0,  	1000,0,
		(_REGEN), 		_FOREST, 250,	_D,
	},
	{	"Cavalry",		4, 20, 4,   	3,5,	0,0,0,  	800,0,	
		(0),        	_CASTLE, 250,	_B,
	},
	{	"Druids",		5, 25, 2,    	2,3,	0,0,0,  	700,0,	
		(0),        	_FOREST, 250,	_C,
	},
	{	"Archmages",	5, 25, 1,   	2,3,	0,0,0,  	1200,0,	
		(_FLY),     	_PLAINS, 250,	_C,
	},
	{	"Vampires",		5, 30, 1,   	3,6,	0,0,0,  	1500,0,	
		(_LEECH | _FLY),_DUNGEON, 250,	_E,
	},
	{	"Giants",		5, 60, 3,   	10,20,	5,10,6, 	2000,0,	
		(0),			_HILL, 250,  	_C,
	},
	{	"Demons",		6, 50, 1,   	5,7,	0,0,0,  	3000,0,	
		(_FLY),			_DUNGEON, 250,	_E,
	},
	{	"Dragons",		6, 200, 1,  	25,50,	0,0,0,  	5000,0,	
		(_FLY), 		_HILL, 250,  	_D,
	},
};
#undef _FLY
#undef _ABSORB
#undef _LEECH
#undef _REGEN

#undef _CASTLE
#undef _PLAINS
#undef _FOREST
#undef _HILL 
#undef _CAVE
#undef _DUNGEON

#undef _A
#undef _B
#undef _C
#undef _D
#undef _E

#define _N MORALE_NORMAL
#define _L MORALE_LOW
#define _H MORALE_HIGH
byte morale_chart[5][5] = {
		/*	 A	 B	 C	 D	 E	 */
/* A */{	_N,	_N,	_N,	_N,	_N	},
/* B */{	_N,	_N,	_N,	_N,	_N	},
/* C */{	_N,	_N,	_H,	_N,	_N	},
/* D */{	_L,	_N,	_L,	_H,	_N	},
/* E */{	_L,	_L,	_L,	_N,	_N	},
};
#undef _N
#undef _L
#undef _H

	/*		TITLE            LEADERSHIP        COMMISSION       *
	 *			    VILLAINS_NEEDED	               KNOWS_MAGIC  *
	 *                               MAX_SPELLS                 *
	 *                                 SPELL_POWER              */
KBclass classes[4][4] = {
	{
		{	"Knight",   	0, 	100,   	2, 	1, 	1000,	0 },
		{	"General",  	2, 	+100,  	+3,	+1,	+1000,	0 },
		{	"Marshal",  	8, 	+300,  	+4,	+1,	+2000,	0 },
		{	"Lord",     	14,	+500,  	+5,	+2,	+4000,	0 },
	},
	{
		{	"Paladin",  	0, 	80, 	3,	1,	1000,	0 },
		{	"Crusader", 	2, 	+80,	+3,	+1,	+1000,	0 },
		{	"Avenger",  	7, 	+240,	+6,	+2,	+2000,	0 },
		{	"Champion", 	13,	+400,	+5,	+2,	+4000,	0 },
	},
	{
		{	"Sorceress",	0,	60, 	5,	2,	3000,	1 },
		{	"Magician", 	3,	+60,	+8,	+3,	+1000,	0 },
		{	"Mage",     	6,	+180,	+10,+5,	+1000,	0 },
		{	"Archmage", 	12,	+300,	+12,+5,	+1000,	0 },
	},
	{
		{	"Barbarian",	0,	100, 	2,	0,	2000,	0 },
		{	"Chieftain",	1,	+100,	+2,	+1,	+2000,	0 },
		{	"Warlord",  	5,	+300,	+3,	+1,	+2000,	0 },
		{	"Overlord", 	10,	+500,	+3,	+1,	+2000,	0 },
	},
};
  
char *dwelling_names[MAX_DWELLINGS] = {
	"Plains",
	"Forest",
	"Hill",	/* Also known as "Cave" */
	"Dungeon",
	"Castle",
};

char *town_names[MAX_TOWNS] = {
	"Riverton",     	//00 (11)
	"Underfoot",    	//01 (14)
	"Path's End",   	//02 (0F)
	"Anomaly",      	//03 (00)
	"Topshore",     	//04 (13)
	"Lakeview",     	//05 (0B)
	"Simpleton",    	//06 (12)
	"Centrapf",     	//07 (02)
	"Quiln Point",  	//08 (10)
	"Midland",      	//09 (0C)
	"Xoctan",       	//0A (17)
	"Overthere",    	//0B (0E)
	"Elan's Landing",	//0C (04)
	"King's Haven", 	//0D (0A)
	"Bayside",      	//0E (01)
	"Nyre",         	//0F (0D)
	"Dark Corner",  	//10 (03)
	"Isla Vista",   	//11 (08)
	"Grimwold",     	//12 (06)
	"Japper",       	//13 (09)
	"Vengeance",    	//14 (15)
	"Hunterville",  	//15 (07)
	"Fjord",        	//16 (05)
	"Yakonia",      	//17 (18)
	"Woods End",    	//18 (16)
	"Zaezoizu",     	//19 (19)
};

char *castle_names[MAX_CASTLES] = {
	"Azram",    	//00
	"Basefit",  	//01
	"Cancomar", 	//02
	"Duvock",   	//03
	"Endryx",   	//04
	"Faxis",    	//05
	"Goobare",  	//06
	"Hyppus",   	//07
	"Irok",	    	//08
	"Jhan",     	//09
	"Kookamunga",	//0A
	"Lorsche",  	//0B
	"Mooseweigh",	//0C
	"Nilslag",  	//0D
	"Ophiraund",	//0E
	"Portalis", 	//0F
	"Quinderwitch",	//10
	"Rythacon", 	//11
	"Spockana", 	//12
	"Tylitch",  	//13
	"Uzare",    	//14
	"Vutar",    	//15
	"Wankelforte",	//16
	"Xelox",    	//17
	"Yeneverre",	//18
	"Zyzzarzaz",	//19
};

char *spell_names[MAX_SPELLS] = {
	"Clone",    	//00
	"Teleport", 	//01
	"Fireball",	//02
	"Lightning",	//03
	"Freeze",	//04
	"Resurrect",	//05
	"Turn Undead",	//06

	"Bridge",   	//07
	"Time Stop",	//08
	"Find Villain",	//09
	"Castle Gate",	//0A
	"Town Gate",	//0B
	"Instant Army",	//0C
	"Raise Control",//0D
};

char *number_names[6] = {
	"A multitude of",	/* 0	(500+) */
	"A horde of",   	/* 1	(100-499) */
	"A lot of",     	/* 2	(50-99) */
	"Many",         	/* 3	(20-49) */
	"Some",         	/* 4	(10-19) */
	"A few",        	/* 5	(1-9) */
};
word number_mins[6] = { 500, 100, 50, 20, 10, 1 };

char *number_name(word num) {
	int i;
	for (i = 0; i < 5; i++)
		if (num >= number_mins[i]) break;
	return number_names[i];
}

char *morale_names[3] = {
	"Norm",
	"Low",
	"High",
};

char *continent_names[4] = {
	"Continentia",
	"Forestria",
	"Archipelia",
	"Saharia",
};
