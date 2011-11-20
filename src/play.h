/*
 *  play.h -- gameplay mechanics
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

/** High-level game functions **/
/* Create new game. */
extern KBgame* spawn_game(char *name, int pclass, int difficulty);

/** Calculators and tests **/

extern int army_leadership(KBgame *game, byte troop_id);

extern int known_spells(KBgame *game);

extern int player_army(KBgame *game);

extern int player_castles(KBgame *game);

extern int player_captured(KBgame *game);

extern int player_num_artifacts(KBgame *game);

extern int player_commission(KBgame *game);

extern int player_score(KBgame *game);

/** Player actions **/

extern int buy_troop(KBgame *game, byte troop_id, word number);

extern void dismiss_troop(KBgame *game, byte slot);

/** Game events **/

extern void fullfill_contract(KBgame *game, byte villain_id);

extern void temp_death(KBgame *game);

extern void test_defeat(KBgame *game);
