/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "mud/action.h"
#include "mud/object.h"
#include "mud/npc.h"

void Creature::doEmote(const std::string& action)
{
	if (getRoom())
		*getRoom() << "(" << StreamName(this, DEFINITE, true) << ") " << action << "\n";
}

void Creature::doSay(const std::string& text)
{
	// don't say nothing
	if (text.empty())
		return;

	// last creature of text
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
		StreamControl stream(*getRoom());  // ends at second bracket, sends text
		stream << StreamIgnore(this);
		if (isDead())
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

	// FIXME EVENT
}

void Creature::doSing(const std::string& text)
{
	// split into lines
	std::vector<std::string> lines;
	explode(lines, text, ';');
	if (lines.size() > 4) {
		*this << "You may only sing up to four lines at a time.\n";
		return;
	}

	// trim lines
	std::vector<std::string>::iterator li = lines.begin();
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
	std::ostringstream output;
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
	StreamControl stream(*getRoom());
	stream << StreamIgnore(this);
	if (isDead())
		stream << "The ghostly voice of " << StreamName(this, INDEFINITE, false);
	else
		stream << StreamName(this, INDEFINITE, true);
	stream << " sings:\n" << output;
}

void Creature::doLook()
{
	// check
	if (!checkSee()) return;
	if (!PLAYER(this)) return;

	getRoom()->show(StreamControl(*this), this);
	Events::sendLook(getRoom(), this, getRoom());
}

void Creature::doLook(Creature *ch)
{
	assert(ch != NULL);

	// check
	if (!checkSee()) return;

	// send message to receiver
	if (this != ch && PLAYER(ch) != NULL)
		*PLAYER(ch) << StreamName(*this, NONE, true) << " glances at you.\n";

	// description
	*this << StreamCreatureDesc(ch);

	// inventory
	ch->displayEquip(StreamControl(this));

	// finish
	*this << "\n";

	Events::sendLook(getRoom(), this, ch);
}

void Creature::doLook(Object *obj, ObjectLocation type)
{
	assert(obj != NULL);

	// check
	if (!checkSee()) return;

	// specific container type
	if (type != 0) {
		if (obj->hasLocation(type))
			obj->showContents(PLAYER(this), type);
		else if (type == ObjectLocation::IN)
			*this << StreamName(*obj, DEFINITE, true) << " cannot be looked inside of.\n";
		else if (type == ObjectLocation::ON)
			*this << StreamName(*obj, DEFINITE, true) << " cannot be looked ontop of.\n";
		else
			*this << StreamName(*obj, DEFINITE, true) << " cannot be looked at that way.\n";
	} else {
		// generic - description and on or in contents
		if (!obj->getDesc().empty())
			*this << StreamMacro(obj->getDesc(), "self", obj, "actor", this) << "  ";
		// on contents?
		if (obj->hasLocation(ObjectLocation::ON))
			obj->showContents(PLAYER(this), ObjectLocation::ON);
		else
			*this << "\n";
	}

	Events::sendLook(getRoom(), this, obj);
}

void Creature::doLook(Portal *portal)
{
	assert(portal != NULL);

	// get target room
	Room* targetRoom = NULL;
	if (!portal->isClosed() && !portal->isNolook())
		targetRoom = portal->getRelativeTarget(getRoom());

	// basic description
	if (!portal->getDesc().empty())
		*this << StreamMacro(portal->getDesc(), "portal", portal, "actor", this) << "  ";
	else if (targetRoom == NULL)
		*this << "There is nothing remarkable about " << StreamName(*portal, DEFINITE) << ".  ";

	// show direction
	if (portal->getDir().valid() && portal->getName().getText() != portal->getDir().getName())
		*this << StreamName(*portal, DEFINITE, true) << " heads " << portal->getRelativeDir(getRoom()).getName() << ".  ";

	// closed portal?
	if (portal->isClosed())
		*this << StreamName(*portal, DEFINITE, true) << " is closed.";
	// open and is a door?
	else if (portal->isDoor())
		*this << StreamName(*portal, DEFINITE, true) << " is open.";

	// finish off line
	*this << "\n";

	Events::sendLook(getRoom(), this, portal);

	// display target room if possible
	if (targetRoom) {
		targetRoom->show(*this, this);
		Events::sendLook(getRoom(), this, targetRoom);
	}
}

class ActionChangePosition : public IAction
{
public:
	ActionChangePosition(Creature* s_ch, CreaturePosition s_position) : IAction(s_ch), position(s_position) {}

	virtual uint getRounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << position.getState(); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		if (position == getActor()->getPosition()) {
			*getActor() << "You are already " << position.getState() << ".\n";
			return 1;
		}

		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " " << position.getActiveVerb() << ".\n";
		*getActor() << "You " << position.getPassiveVerb() << ".\n";
		getActor()->setPosition(position);

		return 0;
	}

private:
	CreaturePosition position;
};

void Creature::doPosition(CreaturePosition position)
{
	addAction(new ActionChangePosition(this, position));
}

class ActionGet : public IAction
{
public:
	ActionGet(Creature* s_ch, Object* s_obj, Object* s_container, ObjectLocation s_type) :
			IAction(s_ch), obj(s_obj), container(s_container), type(s_type) {
	}

	virtual uint getRounds() const { return 2; }
	virtual void describe(const StreamControl& stream) const { stream << "getting " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		if (!getActor()->checkAlive() || !getActor()->checkMove())
			return 1;

		// FIXME: check the container is still accessible

		if (!obj->isTouchable()) {
			*getActor() << "You cannot reach " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		} else if (!obj->isGettable()) {
			*getActor() << "You cannot pickup " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		} else if (getActor()->hold(obj) < 0) {
			*getActor() << "Your hands are full.\n";
			return 1;
		} else {
			// get the object
			if (container) {
				*getActor() << "You get " << StreamName(*obj, DEFINITE) << " from " << StreamName(container, DEFINITE) << ".\n";
				if (getActor()->getRoom()) *getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " gets " << StreamName(obj, INDEFINITE) << " from " << StreamName(container, INDEFINITE) << ".\n";
			} else {
				*getActor() << "You pick up " << StreamName(*obj, DEFINITE) << ".\n";
				if (getActor()->getRoom()) *getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " picks up " << StreamName(obj, INDEFINITE) << ".\n";
			}

			// notification
			Events::sendTouchItem(getActor()->getRoom(), getActor(), obj);
			Events::sendGraspItem(getActor()->getRoom(), getActor(), obj);
			Events::sendPickupItem(getActor()->getRoom(), getActor(), obj);
			return 0;
		}
	}

private:
	Object* obj;
	Object* container;
	ObjectLocation type;
};

void Creature::doGet(Object *obj, Object *contain, ObjectLocation type)
{
	assert(obj != NULL);

	addAction(new ActionGet(this, obj, contain, type));
}

class ActionPut : public IAction
{
public:
	ActionPut(Creature* s_ch, Object* s_obj, Object* s_container, ObjectLocation s_type) :
			IAction(s_ch), obj(s_obj), container(s_container), type(s_type) {
	}

	virtual uint getRounds() const { return 2; }
	virtual void describe(const StreamControl& stream) const { stream << "putting " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		if (!getActor()->checkAlive() || !getActor()->checkMove())
			return 1;

		// FIXME: boy is this incomplete!!

		if (!container->hasLocation(type)) {
			*getActor() << "You cannot do that with " << StreamName(*container) << ".\n";
			return 1;
		} else {
			// should force let go
			container->addObject(obj, type);
			*getActor() << "You put " << StreamName(*obj, DEFINITE) << " " << type.name() << " " << StreamName(container, DEFINITE) << ".\n";
			return 0;
		}
	}

private:
	Object* obj;
	Object* container;
	ObjectLocation type;
};

void Creature::doPut(Object *obj, Object *contain, ObjectLocation type)
{
	assert(obj != NULL);
	assert(contain != NULL);

	addAction(new ActionPut(this, obj, contain, type));
}

class ActionGiveCoins : public IAction
{
public:
	ActionGiveCoins(Creature* s_ch, Creature* s_target, uint s_amount) :
			IAction(s_ch), target(s_target), amount(s_amount) {}

	virtual uint getRounds() const { return 2; }
	virtual void describe(const StreamControl& stream) const { stream << "giving coins to " << StreamName(target, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkAlive() || !getActor()->checkMove())
			return 1;

		// have enough coins?
		if (getActor()->getCoins() == 0) {
			*getActor() << "You don't have any coins.\n";
			return 1;
		} else if (getActor()->getCoins() < amount) {
			*getActor() << "You only have " << getActor()->getCoins() << " coins.\n";
			return 1;
		}

		// FIXME: not safe - what if give_coins overflows?
		// do give
		target->giveCoins(amount);
		getActor()->takeCoins(amount);

		// messages
		*getActor() << "You give " << amount << " coins to " << StreamName(*target, DEFINITE) << ".\n";
		*target << StreamName(*getActor(), INDEFINITE, true) << " gives you " << amount << " coins.\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamIgnore(target) << StreamName(*getActor(), INDEFINITE, true) << " gives some coins to " << StreamName(*target, INDEFINITE) << ".\n";
		return 0;
	}

private:
	Creature* target;
	uint amount;
};

void Creature::doGiveCoins(Creature* target, uint amount)
{
	assert(target != NULL);
	assert(amount != 0);

	addAction(new ActionGiveCoins(this, target, amount));
}

class ActionWear : public IAction
{
public:
	ActionWear(Creature* s_ch, Object* s_obj) :
			IAction(s_ch), obj(s_obj) {}

	virtual uint getRounds() const { return 5; }
	virtual void describe(const StreamControl& stream) const { stream << "putting on " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		if (!getActor()->checkMove())
			return 1;

		if (!getActor()->isHeld(obj)) {
			*getActor() << "You must be holding " << StreamName(*obj, DEFINITE) << " to put it on.\n";
			return 1;
		}

		if (getActor()->wear(obj) < 0) {
			*getActor() << "You can't wear " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		*getActor() << "You wear " << StreamName(*obj, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " equips " << StreamName(obj) << ".\n";
		return 0;
	}

private:
	Object* obj;
};

void Creature::doWear(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionWear(this, obj));
}

class ActionRemove : public IAction
{
public:
	ActionRemove(Creature* s_ch, Object* s_obj) :
			IAction(s_ch), obj(s_obj) {}

	virtual uint getRounds() const { return 5; }
	virtual void describe(const StreamControl& stream) const { stream << "removing " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		if (!getActor()->checkMove())
			return 1;

		if (!getActor()->isWorn(obj)) {
			*getActor() << "You must be wearing " << StreamName(*obj, DEFINITE) << " to take it off.\n";
			return 1;
		}

		if (getActor()->hold(obj) < 0) {
			*getActor() << "Your hands are full.\n";
			return 1;
		}

		*getActor() << "You remove " << StreamName(*obj, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " removes " << StreamName(obj) << ".\n";
		return 0;
	}

private:
	Object* obj;
};

void Creature::doRemove(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionRemove(this, obj));
}

class ActionDrop : public IInstantAction
{
public:
	ActionDrop(Creature* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform() {
		if (!getActor()->checkAlive() || !getActor()->checkMove())
			return;

		if (!getActor()->isHeld(obj)) {
			*getActor() << "You are not holding " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		if (!obj->isDropable()) {
			*getActor() << "You cannot drop " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		// do drop
		*getActor() << "You drop " << StreamName(*obj, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " drops " << StreamName(obj) << ".\n";

		getActor()->getRoom()->addObject(obj);

		// send notification
		Events::sendReleaseItem(getActor()->getRoom(), getActor(), obj);
		Events::sendDropItem(getActor()->getRoom(), getActor(), obj);
		return;
	}

private:
	Object* obj;
};

void Creature::doDrop(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionDrop(this, obj));
}

class ActionRead : public IInstantAction
{
public:
	ActionRead(Creature* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform() {
		// checks
		if (!getActor()->checkSee())
			return;

		// FIXME: add back text property, and an event
		*getActor() << StreamName(*obj, DEFINITE, true) << " cannot be read.\n";
	}

private:
	Object* obj;
};

void Creature::doRead(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionRead(this, obj));
}

class ActionEat : public IAction
{
public:
	ActionEat(Creature* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint getRounds() const { return 4; }
	virtual void describe(const StreamControl& stream) const { stream << "eating " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// FIXME: text and event
		*getActor() << "You can't eat " << StreamName(*obj, DEFINITE) << ".\n";
		return 1;
	}

private:
	Object* obj;
};

void Creature::doEat(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionEat(this, obj));
}

class ActionDrink : public IAction
{
public:
	ActionDrink(Creature* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint getRounds() const { return 4; }
	virtual void describe(const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// FIXME: text and event
		*getActor() << "You can't drink " << StreamName(*obj, DEFINITE) << ".\n";
		return 1;
	}

private:
	Object* obj;
};

void Creature::doDrink(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionDrink(this, obj));
}

class ActionRaise : public IAction
{
public:
	ActionRaise(Creature* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint getRounds() const { return 2; }
	virtual void describe(const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// held?
		if (!getActor()->isHeld(obj)) {
			*getActor() << "You are not holding " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		// output
		*getActor() << "You raise " << StreamName(*obj, DEFINITE) << " into the air.\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " raises " << StreamName(obj) << " into the air.\n";

		// FIXME EVENT
		return 0;
	}

private:
	Object* obj;
};

void Creature::doRaise(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionRaise(this, obj));
}

class ActionTouch : public IInstantAction
{
public:
	ActionTouch(Creature* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform() {
		// checks
		if (!getActor()->checkMove())
			return;

		// touchable?
		if (!obj->isTouchable()) {
			*getActor() << "You cannot touch " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		// output
		*getActor() << "You touch " << StreamName(*obj, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " touches " << StreamName(obj) << ".\n";

		// FIXME EVENT
		return;
	}

private:
	Object* obj;
};

void Creature::doTouch(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionTouch(this, obj));
}

class ActionKick : public IAction
{
public:
	ActionKick(Creature* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint getRounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		if (!obj->isTouchable()) {
			*getActor() << "You cannot kick " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		// output
		*getActor() << "You kick " << StreamName(*obj, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " kickes " << StreamName(obj) << ".\n";

		// FIXME EVENT
		return 0;
	}

private:
	Object* obj;
};

void Creature::doKick(Object *obj)
{
	assert(obj != NULL);

	addAction(new ActionKick(this, obj));
}

class ActionOpenPortal : public IAction
{
public:
	ActionOpenPortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint getRounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "opening " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// door?
		if (!portal->isDoor()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// already open?
		if (!portal->isClosed()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is already open.\n";
			return 1;
		}

		// locked?
		if (portal->isLocked()) {
			*getActor() << "You try to open " << StreamName(*portal, DEFINITE, true) << ", but it is locked.\n";
			if (getActor()->getRoom())
				*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " tries to open " << StreamName(portal, DEFINITE) << ", but it appears to be locked.\n";

			// FIXME EVENT (Touch event)
			return 0;
		}

		// open it
		portal->open(getActor()->getRoom(), getActor());
		*getActor() << "You open " << StreamName(*portal, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " opens " << StreamName(portal, DEFINITE) << ".\n";

		// FIXME EVENT (Touch and Open)

		return 0;
	}

private:
	Portal* portal;
};

void Creature::doOpen(Portal *portal)
{
	assert(portal != NULL);

	addAction(new ActionOpenPortal(this, portal));
}

class ActionClosePortal : public IAction
{
public:
	ActionClosePortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint getRounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "closing " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// door?
		if (!portal->isDoor()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// already closed?
		if (portal->isClosed()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is already closed.\n";
			return 1;
		}

		// close it
		portal->close(getActor()->getRoom(), getActor());
		*getActor() << "You close " << StreamName(*portal, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " closes " << StreamName(portal, DEFINITE) << ".\n";

		// FIXME EVENT (Touch and Close)

		return 0;
	}

private:
	Portal* portal;
};

void Creature::doClose(Portal *portal)
{
	assert(portal != NULL);

	addAction(new ActionClosePortal(this, portal));
}

class ActionLockPortal : public IAction
{
public:
	ActionLockPortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint getRounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "locking " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// door?
		if (!portal->isDoor()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!portal->isClosed()) {
			*getActor() << "You cannot lock " << StreamName(*portal, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// lock it
		portal->lock(getActor()->getRoom(), getActor());
		*getActor() << "You lock " << StreamName(*portal, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " locks " << StreamName(portal, DEFINITE) << ".\n";

		// FIXME EVENT (Touch and Lock)

		return 0;
	}

private:
	Portal* portal;
};

void Creature::doLock(Portal *portal)
{
	assert(portal != NULL);

	addAction(new ActionLockPortal(this, portal));
}

class ActionUnlockPortal : public IAction
{
public:
	ActionUnlockPortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint getRounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "unlocking " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// door?
		if (!portal->isDoor()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!portal->isClosed()) {
			*getActor() << "You cannot unlock " << StreamName(*portal, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// unlock it
		portal->unlock(getActor()->getRoom(), getActor());
		*getActor() << "You unlock " << StreamName(*portal, DEFINITE) << ".\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " unlocks " << StreamName(portal, DEFINITE) << ".\n";

		// FIXME EVENT (Touch and Unlock)
		return 0;
	}

private:
	Portal* portal;
};

void Creature::doUnlock(Portal *portal)
{
	assert(portal != NULL);

	addAction(new ActionUnlockPortal(this, portal));
}

class ActionUsePortal : public IAction
{
public:
	ActionUsePortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal), rounds(0) {}

	virtual uint getRounds() const { return rounds; }
	virtual void describe(const StreamControl& stream) const { stream << "kicking " << StreamName(portal, INDEFINITE); }

	virtual void finish() {
		// checks
		if (!getActor()->checkMove())
			return;

		// closed?  drat
		if (portal->isClosed()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is closed.\n";
			return;
		}

		// get target room
		Room *new_room = portal->getRelativeTarget(getActor()->getRoom());
		if (new_room == NULL) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " does not lead anywhere.\n";
			return;
		}

		// set rounds
		// one round plus one per every 20% below max health
		// add five if an NPC
		rounds = 1 + (100 - (getActor()->getHP() * 100 / getActor()->getMaxHP())) / 20 + (NPC(getActor()) ? 5 : 0);

		// do go
		getActor()->enter(new_room, portal);
	}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// closed?  drat
		if (portal->isClosed()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is closed.\n";
			return 1;
		}

		// get target room
		Room *new_room = portal->getRelativeTarget(getActor()->getRoom());
		if (new_room == NULL) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " does not lead anywhere.\n";
			return 1;
		}

		// set rounds
		// one round plus one per every 20% below max health
		// add five if an NPC
		rounds = 1 + (100 - (getActor()->getHP() * 100 / getActor()->getMaxHP())) / 20 + (NPC(getActor()) ? 5 : 0);

		return 0;
	}

private:
	Portal* portal;
	uint rounds;
};

void Creature::doGo(Portal *portal)
{
	assert(portal != NULL);

	addAction(new ActionUsePortal(this, portal));
}

class ActionKickPortal : public IAction
{
public:
	ActionKickPortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint getRounds() const { return 3; }
	virtual void describe(const StreamControl& stream) const { stream << "kicking " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!getActor()->checkMove())
			return 1;

		// door?
		if (!portal->isDoor()) {
			*getActor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!portal->isClosed()) {
			*getActor() << "You cannot kick " << StreamName(*portal, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// kick it
		portal->open(getActor()->getRoom(), getActor());
		*getActor() << "You kick " << StreamName(*portal, DEFINITE) << " open.\n";
		if (getActor()->getRoom())
			*getActor()->getRoom() << StreamIgnore(getActor()) << StreamName(getActor(), INDEFINITE, true) << " kicks " << StreamName(portal, DEFINITE) << " open.\n";

		// FIXME EVENT (Touch and Kick)

		return 0;
	}

private:
	Portal* portal;
};

void Creature::doKick(Portal *portal)
{
	assert(portal != NULL);

	addAction(new ActionKickPortal(this, portal));
}
