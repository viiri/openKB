/*
 *  rogue.c -- extra gameplay mechanics, not part of original game
 *  Copyright (C) 2014 Vitaly Driedfruit
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
#include "lib/kbres.h" /* For TILE_* defines */
#include "bounty.h"

/* "Furnishning" routines to make tile transitions prettier. 
 * Those are not part of the original game mechanics. */
enum {
	TYPE_OTHER = -1,
	TYPE_WATER = 0,
	TYPE_TREE,
	TYPE_SAND,
	TYPE_ROCK,
};
int tile_type(byte tile) {
	int base;
	     if (IS_WATER(tile))    	base = TYPE_WATER;					
	else if (IS_BRIDGE(tile))   	base = TYPE_WATER;	     
	else if (IS_DEEP_WATER(tile))	base = TYPE_WATER;	     
	else if (IS_TREE(tile))     	base = TYPE_TREE;
	else if (IS_DESERT(tile))   	base = TYPE_SAND;
	else if (IS_ROCK(tile))     	base = TYPE_ROCK;				
	else                        	base = TYPE_OTHER;
	return base;
}
void furnish_map(KBgame* game) {
	static const byte base_offset[4] = { 20, 33, 46, 59 };
	static const byte tile_offset[4][12] = { 
		{ 0, 1, 3, 2, 8, 9, 11, 10, 5, 7, 4, 6 }, /* Water */
		{ 2, 0, 3, 1, 8, 9, 11, 10, 5, 7, 4, 6 }, /* Trees */
		{ 2, 0, 3, 1, 8, 9, 11, 10, 5, 7, 4, 6 }, /* Sand */
		{ 2, 0, 3, 1, 8, 9, 11, 10, 5, 7, 4, 6 }, /* Rock */
	};

	int cont, y, x;

#define NW_DIFF	0x0001
#define N_DIFF	0x0002
#define NE_DIFF	0x0004
#define W_DIFF	0x0008
#define E_DIFF	0x0010
#define SW_DIFF	0x0020
#define S_DIFF	0x0040
#define SE_DIFF	0x0080

	for (cont = 0; cont < MAX_CONTINENTS; cont++) {
		for (y = 0; y < LEVEL_H; y++) {
			for (x = 0; x < LEVEL_W; x++) {

				int base, tile = game->map[cont][y][x];
				int ox, oy, j = 0;

				int flag = 0;

				/* Those tiles don't need furnishing */
				if (IS_INTERACTIVE(tile)
				|| IS_CASTLE(tile)
				|| IS_MAPOBJECT(tile)
				|| IS_BRIDGE(tile)) continue;

				base = tile_type(tile);

				if (base == TYPE_OTHER) continue;

				/* Look around base tile */
				for (oy = -1; oy < 2; oy++) {
					for (ox = -1; ox < 2; ox++) {
						int ntile, nbase;

						/* Don't compare with itself */
						if (ox == 0 && oy == 0) continue;
						
						/* Out of map bounds == water */
						if (y - oy < 0 || y - oy >= LEVEL_H
						|| x + ox < 0 || x + ox >= LEVEL_W)

						ntile = TILE_DEEP_WATER;

						else ntile = game->map[cont][y - oy][x + ox];						

						nbase = tile_type(ntile);

						/* Tile differs */
						if (nbase != base) flag |= (0x01 << j);

						j++;
					}
				}

				/* Ok, let's furnish */
				if (flag) {

					if ((flag & N_DIFF) && (flag & E_DIFF))
						tile = base_offset[base] + tile_offset[base][0];
					else if ((flag & N_DIFF) && (flag & W_DIFF))
						tile = base_offset[base] + tile_offset[base][1];
					else if ((flag & S_DIFF) && (flag & E_DIFF))
						tile = base_offset[base] + tile_offset[base][2];
					else if ((flag & S_DIFF) && (flag & W_DIFF))
						tile = base_offset[base] + tile_offset[base][3];	
					else if ((flag & E_DIFF))
						tile = base_offset[base] + tile_offset[base][4];	
					else if ((flag & W_DIFF))
						tile = base_offset[base] + tile_offset[base][5];	
					else if ((flag & S_DIFF))
						tile = base_offset[base] + tile_offset[base][6];	
					else if ((flag & N_DIFF))
						tile = base_offset[base] + tile_offset[base][7];	
					else if ((flag == NE_DIFF))
						tile = base_offset[base] + tile_offset[base][8];	
					else if ((flag == NW_DIFF))
						tile = base_offset[base] + tile_offset[base][9];	
					else if ((flag == SE_DIFF))
						tile = base_offset[base] + tile_offset[base][10];
					else if ((flag == SW_DIFF))
						tile = base_offset[base] + tile_offset[base][11];	

					game->map[cont][y][x] = tile;
				}


			}
		}
	}
#undef NW_DIFF
#undef N_DIFF
#undef NE_DIFF
#undef W_DIFF
#undef E_DIFF
#undef SW_DIFF
#undef S_DIFF
#undef SE_DIFF
}
/* End of furnishing routines */
