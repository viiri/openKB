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
#ifndef _OPENKB_PLAY_H
#define _OPENKB_PLAY_H

#include "bounty.h"
#include "lib/kbstd.h"

/** High-level game functions **/
/* Create new game. */
extern KBgame* spawn_game(char *name, int pclass, int difficulty, byte *land);

/** Calculators and tests **/

extern int army_leadership(KBgame *game, byte troop_id);

extern byte troop_morale(KBgame *game, byte slot);

extern int known_spells(KBgame *game);

extern int has_power(KBgame *game, byte power);

extern int player_army_slots(KBgame *game);

extern int player_army(KBgame *game);

extern int player_castles(KBgame *game);

extern int player_captured(KBgame *game);

extern int player_num_artifacts(KBgame *game);

extern int player_commission(KBgame *game);

extern int player_score(KBgame *game);

/** Player actions **/

extern void promote_player(KBgame *game);

extern void clear_fog(KBgame *game);

extern void sail_to(KBgame *game, byte continent);

extern int buy_troop(KBgame *game, byte troop_id, word number);

extern void dismiss_troop(KBgame *game, byte slot);

extern int garrison_troop(KBgame *game, int castle_id, byte slot);

extern int ungarrison_troop(KBgame *game, int castle_id, byte slot);

/** Game events **/

extern int end_day(KBgame *game);

extern byte end_week(KBgame *game, dword *spent);

extern int spend_days(KBgame *game, word days);

extern void spend_week(KBgame *game);

extern void spend_gold(KBgame *game, word amount);

extern void fullfill_contract(KBgame *game, byte villain_id);

extern void temp_death(KBgame *game);

/** Spell effects **/
extern void time_stop(KBgame *game);
extern void raise_control(KBgame *game);
extern int magic_damage(KBgame *game, KBcombat *war, int side, int id, word base_damage, byte filter);
extern int clone_troop(KBgame *game, KBcombat *war, int unit_id);
extern int instant_troop(KBgame *game, byte *troop_id);
extern int find_villain(KBgame *game);

/** Combat **/
extern int test_defeat(KBgame *game, KBcombat *war);
extern int test_victory(KBcombat *war);
extern void prepare_units_player(KBcombat *war, int side, KBgame *game);
extern void prepare_units_foe(KBcombat *war, int side, KBgame *game, int continent_id, int foe_id);
extern void prepare_units_castle(KBcombat *war, int side, KBgame *game, int castle_id);
extern void accept_units_player(KBgame *game, int side, KBcombat *war);
extern void reset_turn(KBcombat *war);
extern void wipe_battlefield(KBcombat *war);
extern void reset_match(KBcombat *war, int castle);
extern int next_turn(KBcombat *war);
extern int next_unit(KBcombat *war);
extern int unit_touching(KBcombat *war, int side, int id, int other_id);
extern int unit_surrounded(KBcombat *war, int side, int id);
extern int unit_ranged_shot(KBcombat *war, int side, int id, int other_side, int other_id);
extern int unit_hit_unit(KBcombat *war, int side, int id, int other_side, int other_id);
extern void unit_relocate(KBcombat *war, int side, int id, int nx, int ny);
extern void unit_move_offset(KBcombat *war, int side, int id, int other_full_id, int *ox, int *oy);
extern void unit_fly_offset(KBcombat *war, int side, int id, int other_full_id, int *tx, int *ty);

#endif
