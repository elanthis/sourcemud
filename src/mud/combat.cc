/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2003  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/player.h"
#include "mud/object.h"
#include "mud/server.h"
#include "mud/eventids.h"
#include "mud/room.h"
#include "common/rand.h"
#include "mud/parse.h"
#include "mud/action.h"

// attack command
void
command_attack (Creature* attacker, String argv[])
{
	// check status
	if (!attacker->check_alive() || !attacker->check_move() || !attacker->check_rt())
		return;

	// find victim
	Entity* evictim = attacker->cl_find_any(argv[1]);
	if (!evictim)
		return;
	Creature* victim = CHARACTER(evictim);
	if (!victim) {
		*attacker << "You can't attack " << StreamName(evictim, DEFINITE) << ".\n";
		return;
	}

	// do attack
	if (argv[0][0] == 'k') // KILL
		attacker->do_kill (victim);
	else // ATTACK
		attacker->do_attack (victim);
}

class ActionAttackCreature : public IAction
{
	public:
	ActionAttackCreature (Creature* s_ch, Creature* s_victim, bool s_repeat) : IAction(s_ch), victim(s_victim), rounds(5), repeat(s_repeat) {}

	virtual uint get_rounds (void) const { return rounds; }
	virtual void describe (const StreamControl& stream) const { stream << "attacking " << StreamName(victim, INDEFINITE); }

	virtual int start (void)
	{
		Creature* attacker = get_actor();

		// checks
		if (!attacker->check_move())
			return 1;

		// attacking self?  haha
		if (victim == attacker) {
			*attacker << "You can't attack yourself.\n";
			return -1;
		}

		// anti-PvP
		if (PLAYER(attacker) && PLAYER(victim)) {
			*attacker << "You may not engage in player vs. player combat.\n";
			return -1;
		}

		// safe room?
		if (attacker->get_room() && attacker->get_room()->is_safe()) {
			*attacker << "This room is combat free.\n";
			return -1;
		}

		// reset rounds
		rounds = 0;

		// base skills
		int dodge_skill = victim->get_combat_dodge();
		int defend_strength = dodge_skill +  10; // FIXME

		int attack_skill = 5; // FIXME
		int attack_strength = attack_skill; // FIXME

		// accounting
		int attacks = 0;
		int total_damage = 0;
		int hits = 0;

		// loop thru held objects (looking for weapons)
		Object* weapon;
		for (uint wi = 0; (weapon = attacker->get_held_at(wi)) != NULL; ++wi) {
			// is a weapon - calculate stuff:
			// speed: (weight in kilograms) * 2 = (weight / 100) * 2 = weight / 50
			int attack_speed = weapon->get_weight() / 50;
			if (attack_speed < 3)
				attack_speed = 3;

			// roll
			int attack_roll = roll_dice(2, 10);

			// roll for damage
			int damage = attack_roll * 2; // FIXME
			if (damage < 1)
				damage = 1;

			// hit?
			if (attack_roll + attack_strength >= defend_strength) {
				// message to attacker
				*attacker << "You hit " << StreamName(victim, DEFINITE) << " with " << StreamName(weapon, YOUR) << "!\n";

				// message to victim
				*victim << StreamName(attacker, DEFINITE, true) << " hit YOU with " << StreamName(weapon, INDEFINITE) << "!\n";
				
				// message to room
				*attacker->get_room() << StreamIgnore(attacker) << StreamIgnore(victim) << StreamName(attacker, DEFINITE, true) << " hit " << StreamName(victim, DEFINITE) << " with " << StreamName(weapon, INDEFINITE) << "!\n";

				// accounting
				total_damage += damage;
				++ hits;
			// miss...
			} else {
				// message to attacker
				*attacker << "You attack " << StreamName(victim, DEFINITE) << " with " << StreamName(weapon, YOUR) << ", but miss!\n";

				// message to victim
				*victim << StreamName(attacker, DEFINITE, true) << " attacks YOU with " << StreamName(weapon, INDEFINITE) << ", but missed!\n";
				
				// message to room
				*attacker->get_room() << StreamIgnore(attacker) << StreamIgnore(victim) << StreamName(attacker, DEFINITE, true) << " attacks " << StreamName(victim, DEFINITE) << " with " << StreamName(weapon, INDEFINITE) << ", but misses!\n";
			}

			// combat info
			*attacker->get_room() << "  Atk:" << attack_strength << " + 2d10:" << attack_roll << " VS Def:" << defend_strength << " = " << attack_strength + attack_roll << " VS " << defend_strength;
			if (attack_roll + attack_strength >= defend_strength)
				*attacker->get_room() << " = HIT, Dmg:" << damage << "\n";
			else
				*attacker->get_room() << " = MISS\n";

			// FIXME EVENT

			// accounting
			++ attacks;
			rounds += attack_speed;

			// dead?  quit
			if (victim->is_dead())
				break;
		}

		// cause damage
		if (total_damage > 0)
			victim->damage(total_damage, attacker);

		// fix round time
		if (rounds < 5)
			rounds = 5;

		// no attacks
		if (!attacks) {
			// message
			*attacker << "You have nothing to attack with.\n";

			// cancel command
			return -1;
		}

		// success
		return 0;
	}

	virtual void finish (void)
	{
		// re-attack if repeat is on
		if (repeat && !victim->is_dead() && victim->get_room() == get_actor()->get_room())
			get_actor()->add_action(this);
	}

	private:
	Creature* victim;
	int rounds;
	bool repeat;
};

// attack a creature
void
Creature::do_attack (Creature* victim)
{
	// no NULL
	assert(victim != NULL);

	add_action(new ActionAttackCreature(this, victim, false));
}

// attack a creature until dead
void
Creature::do_kill (Creature* victim)
{
	// no NULL
	assert(victim != NULL);

	add_action(new ActionAttackCreature(this, victim, true));
}

// handle player combat abilities
uint
Player::get_combat_dodge (void) const
{
	return 10; // FIXME
}

uint
Player::get_combat_attack (void) const
{
	return 10; // FIXME
}

uint
Player::get_combat_damage (void) const
{
	return 10; // FIXME
}
