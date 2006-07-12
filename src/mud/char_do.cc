/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/char.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/social.h"
#include "common/streams.h"
#include "mud/parse.h"
#include "mud/eventids.h"
#include "mud/action.h"
#include "mud/object.h"
#include "mud/npc.h"

void
Character::do_emote (StringArg action)
{
	if (get_room())
		*get_room() << "(S(" << StreamName(this, DEFINITE, true) << ") " << action << ")\n";
}

void
Character::do_social (const SocialAdverb* social, Entity* target)
{
	// yourself?  you ijit
	if (target == this) {
		*this << "You can't do that to yourself.\n";
		return;
	}

	// is a ghost?
	if (PLAYER(this) && is_dead()) {
		// no target?
		if (target != NULL) {
			*this << "Ghosts cannot do that.\n";
			return;
		}

		// have we a ghost mode for social?
		if (!social->ghost.self) {
			*this << "Ghosts cannot do that.\n";
			return;
		}

		// do social
		*this << StreamParse(social->ghost.self, S("actor"), this) << "\n";
		if (get_room()) *get_room() << StreamIgnore(this) << StreamParse(social->ghost.others, S("actor"), this) << "\n";
		return;
	}

	// can't talk and need to?
	if (social->social->need_speech() && !can_talk()) {
		*this << "You cannot talk.\n";
		return;
	}
	// can't move and need to?
	if (social->social->need_movement() && !check_move()) return;

	// have we a target?
	if (target == NULL) {
		if (social->action.self) {
			*this << StreamParse(social->action.self, S("actor"), this) << "\n";
			if (get_room()) *get_room() << StreamIgnore(this) << StreamParse(social->action.others, S("actor"), this) << "\n";
		} else {
			*this << "You can't do that without a target person or object.\n";
		}
	// target a character?
	} else if (social->person.self && CHARACTER(target)) {
		*this << StreamParse(social->person.self, S("actor"), this, S("target"), target) << "\n";
		if (PLAYER(target)) {
			*PLAYER(target) << StreamParse(social->person.target, S("actor"), this, S("target"), target) << "\n";
		}
		if (get_room()) *get_room() << StreamIgnore(this) << StreamIgnore(CHARACTER(target)) << StreamParse(social->person.others, S("actor"), this, S("target"), target) << "\n";
	// target an object?
	} else if (social->thing.self && OBJECT(target)) {
		if (!((Object*)(target))->is_touchable() && social->social->need_touch()) {
			*this << "You cannot touch " << StreamName(*target, DEFINITE, false) << ".\n";
		} else {
			*this << StreamParse(social->thing.self, S("actor"), this, S("target"), target) << "\n";
			if (get_room()) *get_room() << StreamIgnore(this) << StreamParse(social->thing.others, S("actor"), this, S("target"), target) << "\n";
		}
	// um...
	} else {
		*this << "You can't do that with " << StreamName(*target, DEFINITE) << ".\n";
	}
}

void
Character::do_say (StringArg text)
{
	// don't say nothing
	if (text.empty())
		return;

	// last character of text
	char last_char = text[text.size() - 1];

	// you say...
	if (last_char == '?')
		*this << "You ask, \"" CTALK;
	else if (last_char == '!')
		*this << "You exclaim, \"" CTALK;
	else
		*this << "You say, \"" CTALK;
	*this << text << CNORMAL "\"\n";

	// blah says...
	{
		StreamControl stream (*get_room()); // ends at second bracket, sends text
		stream << StreamIgnore(this);
		if (is_dead())
			stream << "The ghostly voice of " << StreamName(this, INDEFINITE, false);
		else
			stream << StreamName(this, INDEFINITE, true);

		if (last_char == '?')
			stream << " asks, \"";
		else if (last_char == '!')
			stream << " exclaims, \"";
		else
			stream << " says, \"";

		stream << CTALK << text << CNORMAL "\"\n";
	}

	// send out an event
	Events::send_say(this->get_room(), this, text);
}

void
Character::do_sing (StringArg text)
{
	// split into lines
	StringList lines;
	explode(lines, text, ';');
	if (lines.size() > 4) {
		*this << "You may only sing up to four lines at a time.\n";
		return;
	}

	// trim lines
	StringList::iterator li = lines.begin();
	while (li != lines.end()) {
		*li = strip(*li);
		if (li->empty())
			li = lines.erase(li);
		else
			++li;
	}

	// any lines at all?
	if (lines.empty()) {
		*this << "Sing what?\n";
		return;
	}

	// form output
	StringBuffer output;
	output << "  " CTALK << lines[0];
	for (size_t i = 1; i < lines.size(); ++i) {
		if (i % 2 == 0)
			output << "\n   ";
		else
			output << "\n     ";
		output << lines[i];
	}
	output << CNORMAL "\n";

	// you sing...
	*this << "You sing:\n" << output.str();

	// blah sings...
	StreamControl stream (*get_room());
	stream << StreamIgnore(this);
	if (is_dead())
		stream << "The ghostly voice of " << StreamName(this, INDEFINITE, false);
	else
		stream << StreamName(this, INDEFINITE, true);
	stream << " sings:\n" << output;
}

void
Character::do_look (void)
{
	// check
	if (!check_see()) return;
	if (!PLAYER(this)) return;

	Room *r = get_room();
	if (r) {
		r->show (StreamControl(*this), this);
		Events::send_look(r, this, NULL);
	}
}

void
Character::do_look (Character *ch)
{
	assert (ch != NULL);

	// check
	if (!check_see()) return;

	// send message to receiver
	if (this != ch && PLAYER(ch) != NULL)
		*PLAYER(ch) << StreamName(*this, NONE, true) << " glances at you.\n";

	// description
	*this << StreamCharDesc(ch);

	// inventory
	ch->display_equip(StreamControl(this));

	// finish
	*this << "\n";

	// event
	if (this != ch)
		Events::send_look(this->get_room(), this, ch);
}

void
Character::do_look (const Object *obj, const ContainerType& type)
{
	assert (obj != NULL);

	// check
	if (!check_see()) return;

	// specific container type
	if (type != ContainerType::NONE) {
		if (obj->has_container (type)) 
			obj->show_contents (PLAYER(this), type);
		else
			*this << StreamName(*obj, DEFINITE, true) << " cannot be looked " << type.get_name() << ".\n";
	} else {
		// generic - description and on or in contents
		if (obj->get_desc())
			*this << StreamParse(obj->get_desc(), S("object"), obj, S("actor"), this) << "  ";
		// on contents?
		if (obj->has_container (ContainerType::ON))
			obj->show_contents(PLAYER(this), ContainerType::ON);
		else
			*this << "\n";
	}
}

void
Character::do_look (RoomExit *exit)
{
	assert (exit != NULL);

	// get target room
	Room* target_room = NULL;
	if (!exit->is_closed() && !exit->is_nolook())
		target_room = exit->get_target_room();

	// basic description
	if (exit->get_desc() && strlen(exit->get_desc()))
		*this << StreamParse(exit->get_desc(), S("exit"), exit, S("actor"), this) << "  ";
	else if (target_room == NULL)
		*this << "There is nothing remarkable about " << StreamName(*exit, DEFINITE) << ".  ";

	// show direction
	if (exit->get_dir().valid() && exit->get_name().get_text() != exit->get_dir().get_name())
		*this << StreamName(*exit, DEFINITE, true) << " heads " << exit->get_dir().get_name() << ".  ";

	// closed exit?
	if (exit->is_closed())
		*this << StreamName(*exit, DEFINITE, true) << " is closed.";
	// open and is a door?
	else if (exit->is_door())
		*this << StreamName(*exit, DEFINITE, true) << " is open.";

	// finish off line
	*this << "\n";

	// display target room if possible
	if (target_room)
		target_room->show(*this, this);

	// send look event
	Events::send_look(this->get_room(), this, exit);
}

class ActionChangePosition : public IAction
{
	public:
	ActionChangePosition (Character* s_ch, CharPosition s_position) : IAction(s_ch), position(s_position) {}

	virtual uint get_rounds (void) const { return 1; }
	virtual void describe (const StreamControl& stream) const { stream << position.get_verbing(); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;

		if (position == get_actor()->get_pos()) {
			*get_actor() << "You are already " << position.get_verbing() << ".\n";
			return 1;
		}

		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " " << position.get_sverb() << ".\n";
		*get_actor() << "You " << position.get_verb() << ".\n";
		get_actor()->set_pos(position);

		return 0;
	}

	private:
	CharPosition position;
};

void
Character::do_position (CharPosition position)
{
	add_action(new ActionChangePosition(this, position));
}

class ActionGet : public IAction
{
	public:
	ActionGet (Character* s_ch, Object* s_obj, Object* s_container, const ContainerType& s_type) :
		IAction(s_ch), obj(s_obj), container(s_container), type(s_type) {}

	virtual uint get_rounds (void) const { return 2; }
	virtual void describe (const StreamControl& stream) const { stream << "getting " << StreamName(obj, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void) {
		if (!get_actor()->check_alive() || !get_actor()->check_move())
			return 1;

		// FIXME: check the container is still accessible

		if (!obj->is_touchable()) {
			*get_actor() << "You cannot reach " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		} else if (!obj->is_gettable()) {
			*get_actor() << "You cannot pickup " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		} else if (get_actor()->hold (obj) < 0) {
			*get_actor() << "Your hands are full.\n";
			return 1;
		} else {
			if (container) {
				*get_actor() << "You get " << StreamName(*obj, DEFINITE) << " from " << StreamName(container, DEFINITE) << ".\n";
				if (get_actor()->get_room()) *get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " gets " << StreamName(obj, INDEFINITE) << " from " << StreamName(container, INDEFINITE) << ".\n";
			} else {
				*get_actor() << "You pick up " << StreamName(*obj, DEFINITE) << ".\n";
				if (get_actor()->get_room()) *get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " picks up " << StreamName(obj, INDEFINITE) << ".\n";
			}

			Events::send_get(get_actor()->get_room(), get_actor(), obj, container);
			return 0;
		}
	}

	private:
	Object* obj;
	Object* container;
	const ContainerType type;
};

void
Character::do_get (Object *obj, Object *contain, const ContainerType& type)
{
	assert (obj != NULL);

	add_action(new ActionGet(this, obj, contain, type));
}

class ActionPut : public IAction
{
	public:
	ActionPut (Character* s_ch, Object* s_obj, Object* s_container, const ContainerType& s_type) :
		IAction(s_ch), obj(s_obj), container(s_container), type(s_type) {}

	virtual uint get_rounds (void) const { return 2; }
	virtual void describe (const StreamControl& stream) const { stream << "putting " << StreamName(obj, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void) {
		if (!get_actor()->check_alive() || !get_actor()->check_move())
			return 1;

		// FIXME: boy is this incomplete!!

		if (!container->has_container (type)) {
			*get_actor() << "You cannot do that with " << StreamName(*container) << ".\n";
			return 1;
		} else {
			// should force let go
			container->add_object (obj, type);
			*get_actor() << "You put " << StreamName(*obj, DEFINITE) << " " << type.get_name() << " " << StreamName(container, DEFINITE) << ".\n";
			return 0;
		}
	}

	private:
	Object* obj;
	Object* container;
	const ContainerType type;
};

void
Character::do_put (Object *obj, Object *contain, const ContainerType& type)
{
	assert (obj != NULL);
	assert (contain != NULL);
	assert (type.valid());

	add_action(new ActionPut(this, obj, contain, type));
}

class ActionGiveCoins : public IAction
{
	public:
	ActionGiveCoins (Character* s_ch, Character* s_target, uint s_amount) :
		IAction(s_ch), target(s_target), amount(s_amount) {}

	virtual uint get_rounds (void) const { return 2; }
	virtual void describe (const StreamControl& stream) const { stream << "giving coins to " << StreamName(target, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void) {
		// checks
		if (!get_actor()->check_alive() || !get_actor()->check_move())
			return 1;

		// have enough coins?
		if (get_actor()->get_coins() == 0) {
			*get_actor() << "You don't have any coins.\n";
			return 1;
		} else if (get_actor()->get_coins() < amount) {
			*get_actor() << "You only have " << get_actor()->get_coins() << " coins.\n";
			return 1;
		}

		// FIXME: not safe - what if give_coins overflows?
		// do give
		target->give_coins(amount);
		get_actor()->take_coins(amount);

		// messages
		*get_actor() << "You give " << amount << " coins to " << StreamName(*target, DEFINITE) << ".\n";
		*target << StreamName(*get_actor(), INDEFINITE, true) << " gives you " << amount << " coins.\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamIgnore(target) << StreamName(*get_actor(), INDEFINITE, true) << " gives some coins to " << StreamName(*target, INDEFINITE) << ".\n";
		return 0;
	}

	private:
	Character* target;
	uint amount;
};

void
Character::do_give_coins (Character* target, uint amount)
{
	assert (target != NULL);
	assert (amount != 0);

	add_action(new ActionGiveCoins(this, target, amount));
}

class ActionWear : public IAction
{
	public:
	ActionWear (Character* s_ch, Object* s_obj) :
		IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds (void) const { return 5; }
	virtual void describe (const StreamControl& stream) const { stream << "putting on " << StreamName(obj, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void) {
		if (!get_actor()->check_move())
			return 1;

		if (!get_actor()->is_held(obj)) {
			*get_actor() << "You must be holding " << StreamName(*obj, DEFINITE) << " to put it on.\n";
			return 1;
		}

		if (get_actor()->wear (obj) < 0) {
			*get_actor() << "You can't wear " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		*get_actor() << "You wear " << StreamName(*obj, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " equips " << StreamName(obj) << ".\n";
		return 0;
	}

	private:
	Object* obj;
};

void
Character::do_wear (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionWear(this, obj));
}

class ActionRemove : public IAction
{
	public:
	ActionRemove (Character* s_ch, Object* s_obj) :
		IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds (void) const { return 5; }
	virtual void describe (const StreamControl& stream) const { stream << "removing " << StreamName(obj, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void) {
		if (!get_actor()->check_move())
			return 1;

		if (!get_actor()->is_worn(obj)) {
			*get_actor() << "You must be wearing " << StreamName(*obj, DEFINITE) << " to take it off.\n";
			return 1;
		}

		if (get_actor()->hold(obj) < 0) {
			*get_actor() << "Your hands are full.\n";
			return 1;
		}

		*get_actor() << "You remove " << StreamName(*obj, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " removes " << StreamName(obj) << ".\n";
		return 0;
	}

	private:
	Object* obj;
};

void
Character::do_remove (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionRemove(this, obj));
}

class ActionDrop : public IInstantAction
{
	public:
	ActionDrop (Character* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform (void)
	{
		if (!get_actor()->check_alive() || !get_actor()->check_move())
			return;

		if (!get_actor()->is_held (obj)) {
			*get_actor() << "You are not holding " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		if (!obj->is_dropable()) {
			*get_actor() << "You cannot drop " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		*get_actor() << "You drop " << StreamName(*obj, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " drops " << StreamName(obj) << ".\n";

		get_actor()->get_room()->add_object (obj);

		Events::send_drop(get_actor()->get_room(), get_actor(), obj);

		return;
	}

	private:
	Object* obj;
};

void
Character::do_drop (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionDrop(this, obj));
}

class ActionRead : public IInstantAction
{
	public:
	ActionRead (Character* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform (void)
	{
		// checks
		if (!get_actor()->check_see())
			return;

		switch (obj->do_action(S("read"), get_actor())) {
			// use the object normally
			case OBJECT_ACTION_OK_NORMAL:
			{
				// has plain text?
				String text = obj->get_property(S("read_text")).get_string();
				if (!text) {
					*get_actor() << StreamName(*obj, DEFINITE, true) << " cannot be read.\n";
					return;
				}

				// show text
				*get_actor() << StreamParse(text, S("object"), obj, S("get_actor()"), get_actor()) << "\n";
					
				// room text
				if (get_actor()->get_room()) {
					text = obj->get_property(S("read_room")).get_string();
					if (!text)
						*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " reads " << StreamName(obj) << ".\n";
					else
						*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamParse(text, S("object"), obj, S("get_actor()"), get_actor()) << "\n";
				}

				// event
				Events::send_read(get_actor()->get_room(), get_actor(), obj);
				return;
			}

			// just send the event
			case OBJECT_ACTION_OK_QUIET:
				// event
				Events::send_read(get_actor()->get_room(), get_actor(), obj);
				return;

			// fail with error
			case OBJECT_ACTION_CANCEL_NORMAL:
				*get_actor() << StreamName(*obj, DEFINITE, true) << " cannot be read.\n";
				return;

			// quiet fail
			case OBJECT_ACTION_CANCEL_QUIET:
				return;
		}
	}

	private:
	Object* obj;
};

void
Character::do_read (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionRead(this, obj));
}

class ActionEat : public IAction
{
	public:
	ActionEat (Character* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds (void) const { return 4; }
	virtual void describe (const StreamControl& stream) const { stream << "eating " << StreamName(obj, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;

		switch (obj->do_action(S("eat"), obj)) {
			// normal processing
			case OBJECT_ACTION_OK_NORMAL:
			{
				// you eat text
				String text = obj->get_property(S("eat_text")).get_string();
				if (!text) {
					*get_actor() << "You can't eat " << StreamName(*obj, DEFINITE) << ".\n";
					return 1;
				}

				// show text
				*get_actor() << StreamParse(text, S("object"), obj, S("get_actor()"), get_actor()) << "\n";
					
				// room text
				if (get_actor()->get_room()) {
					text = obj->get_property(S("eat_room")).get_string();
					if (!text)
						*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " eats " << StreamName(obj) << ".\n";
					else
						*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamParse(text, S("object"), obj, S("get_actor()"), get_actor()) << "\n";
				}

				// event
				Events::send_eat(get_actor()->get_room(), get_actor(), obj);
				return 0;
			}

			// just send event
			case OBJECT_ACTION_OK_QUIET:
				// event
				Events::send_eat(get_actor()->get_room(), get_actor(), obj);
				return 0;

			// failure message
			case OBJECT_ACTION_CANCEL_NORMAL:
				// error
				*get_actor() << "You can't eat " << StreamName(*obj, DEFINITE) << ".\n";
				return 1;

			// do nothing
			case OBJECT_ACTION_CANCEL_QUIET:
				// quiet
				return 1;
		}

		// should never reach this
		abort();
	}

	private:
	Object* obj;
};

void
Character::do_eat (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionEat(this, obj));
}

class ActionDrink : public IAction
{
	public:
	ActionDrink (Character* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds (void) const { return 4; }
	virtual void describe (const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;

		switch (obj->do_action(S("drink"), obj)) {
			// normal processing
			case OBJECT_ACTION_OK_NORMAL:
			{
				// you drink text
				String text = obj->get_property(S("drink_text")).get_string();
				if (!text) {
					*get_actor() << "You can't drink " << StreamName(*obj, DEFINITE) << ".\n";
					return 1;
				}

				// show text
				*get_actor() << StreamParse(text, S("object"), obj, S("get_actor()"), get_actor()) << "\n";
					
				// room text
				if (get_actor()->get_room()) {
					text = obj->get_property(S("drink_room")).get_string();
					if (!text)
						*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " drinks " << StreamName(obj) << ".\n";
					else
						*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamParse(text, S("object"), obj, S("get_actor()"), get_actor()) << "\n";
				}

				// event
				Events::send_drink(get_actor()->get_room(), get_actor(), obj);
				return 0;
			}

			// just send event
			case OBJECT_ACTION_OK_QUIET:
				// event
				Events::send_drink(get_actor()->get_room(), get_actor(), obj);
				return 0;

			// failure message
			case OBJECT_ACTION_CANCEL_NORMAL:
				// error
				*get_actor() << "You can't drink " << StreamName(*obj, DEFINITE) << ".\n";
				return 1;

			// do nothing
			case OBJECT_ACTION_CANCEL_QUIET:
				// quiet
				return 1;
		}

		// should never reach this
		abort();
	}

	private:
	Object* obj;
};

void
Character::do_drink (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionDrink(this, obj));
}

class ActionRaise : public IAction
{
	public:
	ActionRaise (Character* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds (void) const { return 2; }
	virtual void describe (const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;

		// held?
		if (!get_actor()->is_held(obj)) {
			*get_actor() << "You are not holding " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		switch (obj->do_action(S("raise"), get_actor())) {
			// normal processing
			case OBJECT_ACTION_OK_NORMAL:
				// output
				*get_actor() << "You raise " << StreamName(*obj, DEFINITE) << " into the air.\n";
				if (get_actor()->get_room())
					*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " raises " << StreamName(obj) << " into the air.\n";

				// event
				Events::send_raise(get_actor()->get_room(), get_actor(), obj);
				return 0;

			// just the event
			case OBJECT_ACTION_OK_QUIET:
				// event
				Events::send_raise(get_actor()->get_room(), get_actor(), obj);
				return 0;

			// error message
			case OBJECT_ACTION_CANCEL_NORMAL:
				*get_actor()<< "You cannot raise " << StreamName(*obj, DEFINITE) << ".\n";
				return 1;

			// silent fail
			case OBJECT_ACTION_CANCEL_QUIET:
				return 1;
		}

		// should never reach this
		abort();
	}

	private:
	Object* obj;
};

void
Character::do_raise (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionRaise(this, obj));
}

class ActionTouch : public IInstantAction
{
	public:
	ActionTouch (Character* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform (void)
	{
		// checks
		if (!get_actor()->check_move())
			return;

		// touchable?
		if (!obj->is_touchable()) {
			*get_actor() << "You cannot touch " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		switch (obj->do_action(S("touch"), get_actor())) {
			// normal processing
			case OBJECT_ACTION_OK_NORMAL:
				// output
				*get_actor() << "You touch " << StreamName(*obj, DEFINITE) << ".\n";
				if (get_actor()->get_room())
					*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " touches " << StreamName(obj) << ".\n";

				// event
				Events::send_touch(get_actor()->get_room(), get_actor(), obj);
				return;

			// just the event
			case OBJECT_ACTION_OK_QUIET:
				// event
				Events::send_touch(get_actor()->get_room(), get_actor(), obj);
				return;

			// error message
			case OBJECT_ACTION_CANCEL_NORMAL:
				*get_actor()<< "You cannot touch " << StreamName(*obj, DEFINITE) << ".\n";
				return;

			// silent fail
			case OBJECT_ACTION_CANCEL_QUIET:
				return;
		}
	}

	private:
	Object* obj;
};

void
Character::do_touch (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionTouch(this, obj));
}

class ActionKick : public IAction
{
	public:
	ActionKick (Character* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds (void) const { return 1; }
	virtual void describe (const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;

		if (!obj->is_touchable()) {
			*get_actor() << "You cannot kick " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		switch (obj->do_action(S("kick"), get_actor())) {
			// normal processing
			case OBJECT_ACTION_OK_NORMAL:
				// output
				*get_actor() << "You kick " << StreamName(*obj, DEFINITE) << ".\n";
				if (get_actor()->get_room())
					*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " kickes " << StreamName(obj) << ".\n";

				// event
				Events::send_kick(get_actor()->get_room(), get_actor(), obj);
				return 0;

			// just event event
			case OBJECT_ACTION_OK_QUIET:
				Events::send_kick(get_actor()->get_room(), get_actor(), obj);
				return 0;

			// failure message
			case OBJECT_ACTION_CANCEL_NORMAL:
				*get_actor()<< "You cannot kick " << StreamName(*obj, DEFINITE) << ".\n";
				return 1;

			// quiet failure
			case OBJECT_ACTION_CANCEL_QUIET:
				return 1;
		}

		// should never reach this
		abort();
	}

	private:
	Object* obj;
};

void
Character::do_kick (Object *obj)
{
	assert (obj != NULL);

	add_action(new ActionKick(this, obj));
}

class ActionOpenExit : public IAction
{
	public:
	ActionOpenExit (Character* s_ch, RoomExit* s_exit) : IAction(s_ch), exit(s_exit) {}

	virtual uint get_rounds (void) const { return 1; }
	virtual void describe (const StreamControl& stream) const { stream << "opening " << StreamName(exit, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;
		if (!exit->is_closed ()) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " is already open.\n";
			return 1;
		}

		// door?
		if (!exit->is_door()) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// locked?
		if (exit->is_locked ()) {
			*get_actor() << "You try to open " << StreamName(*exit, DEFINITE, true) << ", but it is locked.\n";
			if (get_actor()->get_room())
				*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " tries to open " << StreamName(exit, DEFINITE) << ", but it appears to be locked.\n";

			// event (you touch the door to see if its locked
			Events::send_touch(get_actor()->get_room(), get_actor(), exit);
			return 0;
		}

		// open it
		exit->open ();
		*get_actor() << "You open " << StreamName(*exit, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " opens " << StreamName(exit, DEFINITE) << ".\n";

		// events
		Events::send_touch(get_actor()->get_room(), get_actor(), exit);
		Events::send_open(get_actor()->get_room(), get_actor(), exit);

		return 0;
	}

	private:
	RoomExit* exit;
};

void
Character::do_open (RoomExit *exit)
{
	assert (exit != NULL);

	add_action(new ActionOpenExit(this, exit));
}

class ActionCloseExit : public IAction
{
	public:
	ActionCloseExit (Character* s_ch, RoomExit* s_exit) : IAction(s_ch), exit(s_exit) {}

	virtual uint get_rounds (void) const { return 1; }
	virtual void describe (const StreamControl& stream) const { stream << "closing " << StreamName(exit, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;
		if (!exit->is_closed()) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " is already closed.\n";
			return 1;
		}

		// door?
		if (!exit->is_door()) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// close it
		exit->close ();
		*get_actor() << "You close " << StreamName(*exit, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " closes " << StreamName(exit, DEFINITE) << ".\n";

		// events
		Events::send_touch(get_actor()->get_room(), get_actor(), exit);
		Events::send_close(get_actor()->get_room(), get_actor(), exit);

		return 0;
	}

	private:
	RoomExit* exit;
};

void
Character::do_close (RoomExit *exit)
{
	assert (exit != NULL);

	add_action(new ActionCloseExit(this, exit));
}

class ActionLockExit : public IAction
{
	public:
	ActionLockExit (Character* s_ch, RoomExit* s_exit) : IAction(s_ch), exit(s_exit) {}

	virtual uint get_rounds (void) const { return 1; }
	virtual void describe (const StreamControl& stream) const { stream << "locking " << StreamName(exit, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;

		// door?
		if (!exit->is_door()) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!exit->is_closed()) {
			*get_actor() << "You cannot lock " << StreamName(*exit, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// lock it
		exit->lock ();
		*get_actor() << "You lock " << StreamName(*exit, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " locks " << StreamName(exit, DEFINITE) << ".\n";

		// events
		Events::send_touch(get_actor()->get_room(), get_actor(), exit);
		Events::send_lock(get_actor()->get_room(), get_actor(), exit);

		return 0;
	}

	private:
	RoomExit* exit;
};

void
Character::do_lock (RoomExit *exit)
{
	assert (exit != NULL);

	add_action(new ActionLockExit(this, exit));
}

class ActionUnlockExit : public IAction
{
	public:
	ActionUnlockExit (Character* s_ch, RoomExit* s_exit) : IAction(s_ch), exit(s_exit) {}

	virtual uint get_rounds (void) const { return 1; }
	virtual void describe (const StreamControl& stream) const { stream << "unlocking " << StreamName(exit, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;

		// door?
		if (!exit->is_door()) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!exit->is_closed()) {
			*get_actor() << "You cannot unlock " << StreamName(*exit, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// unlock it
		exit->unlock ();
		*get_actor() << "You unlock " << StreamName(*exit, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " unlocks " << StreamName(exit, DEFINITE) << ".\n";

		// events
		Events::send_touch(get_actor()->get_room(), get_actor(), exit);
		Events::send_unlock(get_actor()->get_room(), get_actor(), exit);

		return 0;
	}

	private:
	RoomExit* exit;
};

void
Character::do_unlock (RoomExit *exit)
{
	assert (exit != NULL);

	add_action(new ActionUnlockExit(this, exit));
}

class ActionUseExit : public IAction
{
	public:
	ActionUseExit (Character* s_ch, RoomExit* s_exit) : IAction(s_ch), exit(s_exit), rounds(0) {}

	virtual uint get_rounds (void) const { return rounds; }
	virtual void describe (const StreamControl& stream) const { stream << "kicking " << StreamName(exit, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// exit must be in same room
		if (exit->get_room() != get_actor()->get_room())
			return 1;

		// set rounds 
		// one round plus one per every 20% below max health
		// add five if an NPC
		rounds = 1 + (100 - (get_actor()->get_hp() * 100 / get_actor()->get_max_hp())) / 20 + (NPC(get_actor()) ? 5 : 0);

		// checks
		if (!get_actor()->check_move())
			return 1;

		// closed?  drat
		if (exit->is_closed()) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " is closed.\n";
			return 1;
		}

		// try the use script - if it returns false, don't do normal stuff below
		if (!exit->get_use().empty()) {
			Scriptix::Value result = exit->get_use().run(exit, get_actor());
			if (result.is_false())
				return 1;
		}

		// get target room
		Room *new_room = exit->get_target_room ();
		if (new_room == NULL) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " does not lead anywhere.\n";
			return 1;
		}

		// do go
		get_actor()->enter (new_room, exit);

		return 0;
	}

	private:
	RoomExit* exit;
	uint rounds;
};

void
Character::do_go (RoomExit *exit)
{
	assert (exit != NULL);

	add_action(new ActionUseExit(this, exit));
}

class ActionKickExit : public IAction
{
	public:
	ActionKickExit (Character* s_ch, RoomExit* s_exit) : IAction(s_ch), exit(s_exit) {}

	virtual uint get_rounds (void) const { return 3; }
	virtual void describe (const StreamControl& stream) const { stream << "kicking " << StreamName(exit, INDEFINITE); }
	virtual void finish (void) {}

	virtual int start (void)
	{
		// checks
		if (!get_actor()->check_move())
			return 1;

		// door?
		if (!exit->is_door()) {
			*get_actor() << StreamName(*exit, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!exit->is_closed()) {
			*get_actor() << "You cannot kick " << StreamName(*exit, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// kick it
		exit->open();
		*get_actor() << "You kick " << StreamName(*exit, DEFINITE) << " open.\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " kicks " << StreamName(exit, DEFINITE) << " open.\n";

		// events
		Events::send_touch(get_actor()->get_room(), get_actor(), exit);
		Events::send_kick(get_actor()->get_room(), get_actor(), exit);

		return 0;
	}

	private:
	RoomExit* exit;
};

void
Character::do_kick (RoomExit *exit)
{
	assert (exit != NULL);

	add_action(new ActionKickExit(this, exit));
}
