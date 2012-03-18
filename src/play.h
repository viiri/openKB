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

extern void end_day(KBgame *game);

extern void end_week(KBgame *game);

extern void spend_days(KBgame *game, word days);

extern void spend_week(KBgame *game);

extern void fullfill_contract(KBgame *game, byte villain_id);

extern void temp_death(KBgame *game);

/** Spell effects **/
extern void raise_control(KBgame *game);
extern int clone_troop(KBgame *game, KBcombat *war, int unit_id);

/** Combat **/
extern int test_defeat(KBgame *game, KBcombat *war);
extern int test_victory(KBcombat *war);
extern void prepare_units_player(KBcombat *war, int side, KBgame *game);
extern void prepare_units_foe(KBcombat *war, int side, KBgame *game, int continent_id, int foe_id);
extern void prepare_units_castle(KBcombat *war, int side, KBgame *game, int castle_id);
extern void accept_units_player(KBgame *game, int side, KBcombat *war);
extern void reset_turn(KBcombat *war);
extern void wipe_battlefield(KBcombat *war);
extern void reset_match(KBcombat *war);
extern int next_turn(KBcombat *war);
extern int next_unit(KBcombat *war);
extern int unit_touching(KBcombat *war, int side, int id, int other_id);
extern int unit_surrounded(KBcombat *war, int side, int id);
extern int unit_ranged_shot(KBcombat *war, int side, int id, int other_side, int other_id);
extern void unit_relocate(KBcombat *war, int side, int id, int nx, int ny);
