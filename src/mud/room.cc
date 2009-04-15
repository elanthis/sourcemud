/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */


#include "common.h"
#include "common/string.h"
#include "common/error.h"
#include "common/streams.h"
#include "common/rand.h"
#include "mud/macro.h"
#include "mud/zone.h"
#include "mud/object.h"
#include "mud/creature.h"
#include "mud/portal.h"
#include "mud/hooks.h"
#include "mud/efactory.h"
#include "mud/room.h"
#include "mud/color.h"
#include "mud/server.h"
#include "mud/player.h"
#include "mud/npc.h"
#include "mud/weather.h"

/* constructor */
Room::Room()
{
	/* clear de values */
	zone = NULL;
	flags.outdoors = false;
	flags.safe = false;
	flags.noweather = false;
	coins = 0;
}

Room::~Room()
{
}

int Room::loadNode(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
	FO_ATTR("room", "id")
	setId(node.getString());
	FO_ATTR("room", "name")
	setName(node.getString());
	FO_ATTR("room", "desc")
	setDesc(node.getString());
	FO_ATTR("room", "outdoors")
	flags.outdoors = node.getBool();
	FO_ATTR("room", "safe")
	flags.safe = node.getBool();
	FO_ATTR("room", "noweather")
	flags.noweather = node.getBool();
	FO_ATTR("room", "coins")
	coins = node.getInt();
	FO_ENTITY("room", "child")
	if (NPC(entity)) {
		addCreature(NPC(entity));
	} else if (OBJECT(entity)) {
		addObject(OBJECT(entity));
	} else if (PORTAL(entity)) {
		Portal* portal = PORTAL(entity);

		// direction checking
		if (!portal->getDir().valid())
			throw File::Error("Portal has no dir");
		if (getPortalByDir(portal->getDir()) != NULL)
			throw File::Error("Duplicate portal direction");

		// add
		portal->parent_room = this;
		portals[portal->getDir()] = portal;

		// activate if necessary
		if (isActive())
			portal->activate();
	} else {
		throw File::Error("Room child is not an Npc, Object, or Portal");
	}
	FO_PARENT(Entity)
	FO_NODE_END
}

int Room::loadFinish()
{
	return 0;
}

/* save the stupid thing */
void Room::saveData(File::Writer& writer)
{
	writer.attr("room", "id", id);

	if (!name.empty())
		writer.attr("room", "name", name.getFull());

	if (!desc.empty())
		writer.attr("room", "desc", desc);

	Entity::saveData(writer);

	if (flags.outdoors)
		writer.attr("room", "outdoors", true);
	if (flags.safe)
		writer.attr("room", "safe", true);
	if (flags.noweather)
		writer.attr("room", "noweather", true);

	if (coins)
		writer.attr("room", "coins", coins);

	for (std::map<PortalDir, Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		if (i->second->getOwner() == this)
			i->second->save(writer, "room", "child");
	}

	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->save(writer, "room", "child");

	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i) {
		if (NPC(*i))
			(*i)->save(writer, "room", "child");
	}
}

void Room::saveHook(File::Writer& writer)
{
	Entity::saveHook(writer);
	Hooks::saveRoom(this, writer);
}

Portal* Room::findPortal(const std::string& e_name, uint c, uint *matches)
{
	assert(c != 0);

	if (matches)
		*matches = 0;

	for (std::map<PortalDir, Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		if (i->second->nameMatch(e_name)) {
			if (matches)
				++ *matches;
			if ((-- c) == 0)
				return i->second;
		}
	}
	return NULL;
}

Portal* Room::getPortalByDir(PortalDir dir)
{
	std::map<PortalDir, Portal*>::iterator i = portals.find(dir);
	if (i != portals.end())
		return i->second;
	else
		return NULL;
}

Portal* Room::newPortal(PortalDir dir)
{
	Portal *portal = new Portal();
	if (portal == NULL)
		return NULL;
	portal->setDir(dir);
	portal->parent_room = this;
	portals[dir] = portal;
	if (isActive())
		portal->activate();
	return portal;
}

bool Room::registerPortal(Portal* portal)
{
	assert(portal != NULL);
	assert(portal->getTarget() == getId());

	std::map<PortalDir, Portal*>::iterator i = portals.find(portal->getDir().getOpposite());
	if (i == portals.end()) {
		portals[portal->getDir().getOpposite()] = portal;
		return true;
	} else if (i->second == portal)
		return true; // already registered
	else
		return false; // another portal is here
}

void Room::unregisterPortal(Portal* portal)
{
	assert(portal != NULL);
	assert(portal->getTarget() == getId());

	std::map<PortalDir, Portal*>::iterator i = portals.find(portal->getDir().getOpposite());
	if (i != portals.end() && i->second == portal)
		portals.erase(i);
}

// coins
uint Room::giveCoins(uint amount)
{
	return coins += amount < (UINT_MAX - coins) ? amount : (UINT_MAX - coins);
}
uint Room::takeCoins(uint amount)
{
	return coins -= amount < coins ? amount : coins;
}

/* update: one game tick */
void Room::heartbeat()
{
	// call update hook
	Hooks::roomHeartbeat(this);
}

void Room::activate()
{
	Entity::activate();

	for (std::map<PortalDir, Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		if (i->second->getRoom() == this)
			i->second->activate();

	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		(*i)->activate();

	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->activate();
}

void Room::deactivate()
{
	for (std::map<PortalDir, Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		if (i->second->getRoom() == this)
			i->second->deactivate();
	}

	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		(*i)->deactivate();

	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->deactivate();

	Entity::deactivate();
}

void Room::setOwner(Entity* s_owner)
{
}

Entity* Room::getOwner() const
{
	return NULL;
}

void Room::ownerRelease(Entity* child)
{
	// Creature?
	Creature* ch = CHARACTER(child);
	if (ch != NULL) {
		creatures.remove(ch);
		return;
	}

	// Object?
	Object* obj = OBJECT(child);
	if (obj != NULL) {
		objects.remove(obj);
		return;
	}

	// Portal?
	Portal* portal = PORTAL(child);
	if (portal != NULL) {
		std::map<PortalDir, Portal*>::iterator i = portals.find(portal->getDir());
		if (i != portals.end() && i->second == portal)
			portals.erase(i);
	}

	// something we don't support
	assert(false);
}

/* print out Room */
void Room::show(const StreamControl& stream, Creature* viewer)
{
	// if there's a hook for this, don't do our version
	if (Hooks::showRoom(this, viewer))
		return;

	// basic info
	stream << "[ " << StreamName(*this, NONE, true) << " ]\n";
	stream << CDESC "  " << StreamMacro(getDesc()).set("self", this).set("viewer", viewer) << CNORMAL;

	// we're outdoors - do that stuff
	if (isOutdoors()) {
		// show weather
		if (!flags.noweather)
			stream << "  " << MWeather.getCurrentDesc();
		// show time
		if (MTime.time.isDay()) {
			if (!MTime.calendar.day_text.empty())
				stream << "  " << MTime.calendar.day_text[Random::get(MTime.calendar.day_text.size())];
		} else {
			if (!MTime.calendar.night_text.empty())
				stream << "  " << MTime.calendar.night_text[Random::get(MTime.calendar.night_text.size())];
		}
	}
	stream << "\n";

	// lists of stuffs
	std::vector<Entity*> ents;
	ents.reserve(10);

	// portals
	for (std::map<PortalDir, Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		// portal not hidden?
		if (!i->second->isHidden() && !i->second->isDisabled())
			ents.push_back(i->second);
	}
	if (!ents.empty()) {
		stream << "Obvious exits are ";
		for (size_t i = 0; i < ents.size(); ++i) {
			if (i > 0) {
				if (ents.size() == 2)
					stream << " and ";
				else if (i == ents.size() - 1)
					stream << ", and ";
				else
					stream << ", ";
			}
			if (PORTAL(ents[i])->isStandard())
				stream << CEXIT << PORTAL(ents[i])->getRelativeDir(this).getName() << CNORMAL;
			else
				stream << StreamName(ents[i], INDEFINITE) << '[' << PORTAL(ents[i])->getRelativeDir(this).getAbbr() << ']';
		}
		stream << ".\n";
	}
	ents.clear();

	// displaying creatures and objects
	int displayed = 0;
	Entity* last = NULL;

	// show players and NPCs
	if (!creatures.empty()) {
		// iterator
		for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i) {
			// not ourselves
			if ((Creature*)(*i) != viewer) {
				// have we a last entry?
				if (last) {
					// pre-text
					if (!displayed)
						stream << "You see ";
					else
						stream << ", ";

					// name
					stream << StreamName(*last, INDEFINITE);

					++displayed;
				}

				// last is this one now
				last = (*i);
			}
		}
	}

	// object list
	if (!objects.empty()) {
		// iterator
		for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i) {
			// no hidden?
			if (!(*i)->isHidden()) {
				// have we a last item?
				if (last) {
					// pre-text
					if (!displayed)
						stream << "You see ";
					else
						stream << ", ";

					stream << StreamName(*last, INDEFINITE);

					++displayed;
				}

				// last is us
				last = (*i);
			}
		}
	}

	// left over last?
	if (last) {
		// pre-text
		if (!displayed)
			stream << "You see ";
		else {
			if (coins)
				stream << ", ";
			else if (displayed >  1)
				stream << ", and ";
			else
				stream << " and ";
		}

		// show name
		stream << StreamName(*last, INDEFINITE);

		++displayed;
	}

	// coins?
	if (coins) {
		if (!displayed)
			stream << "You see ";
		else if (displayed > 1)
			stream << ", and ";
		else
			stream << " and ";

		if (coins == 1)
			stream << "one coin";
		else
			stream << coins << " coins";

		++displayed;
	}

	// finish up
	if (displayed)
		stream << ".\n";
}

/* print all portals */
void Room::showPortals(const StreamControl& stream)
{
	for (std::map<PortalDir, Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		stream << StreamName(*i->second) << " <" << i->second->getTarget() << ">\n";
}

/* broadcast a message to the Room */
void Room::put(const std::string& msg, size_t len, std::vector<Creature*>* ignore_list)
{
	// iterator
	for (EList<Creature>::iterator i = creatures.begin(); i != creatures.end(); ++i) {
		// skip ignored creatures
		if (ignore_list != NULL) {
			if (std::find(ignore_list->begin(), ignore_list->end(), (*i)) != ignore_list->end())
				continue;
		}
		// output
		(*i)->streamPut(msg.c_str(), len);
	}
}

/* find a Creature by name */
Creature* Room::findCreature(const std::string& cname, uint c, uint *matches)
{
	assert(c != 0);

	return CHARACTER(creatures.match(cname, c, matches));
}

/* find an object by name */
Object* Room::findObject(const std::string& oname, uint c, uint *matches)
{
	assert(c != 0);

	return OBJECT(objects.match(oname, c, matches));
}

void Room::addCreature(Creature* creature)
{
	assert(creature != NULL);

	creature->setOwner(this);
	creatures.add(creature);
}

void Room::addObject(Object* object)
{
	assert(object != NULL);

	object->setOwner(this);
	objects.add(object);
}

unsigned long Room::countPlayers() const
{
	unsigned long count = 0;
	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		if (PLAYER(*i))
			++count;
	return count;
}

void Room::handleEvent(const Event& event)
{
	Entity::handleEvent(event);
}

void Room::broadcastEvent(const Event& event)
{
	// propogate to objects
	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		MEvent.resend(event, *i);

	// propogate to creatures
	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		MEvent.resend(event, *i);

	// propogate to portals
	for (std::map<PortalDir, Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		MEvent.resend(event, i->second);
}

// StreamSink for room buffering
class
			RoomStreamSink : public IStreamSink
{
public:
	RoomStreamSink(class Room& s_room) : room(s_room), buffer(), ignores() {}

	virtual void streamPut(const char* text, size_t len) { buffer.write(text, len); }
	virtual void streamIgnore(class Creature* ch) { ignores.push_back(ch); }
	virtual void streamEnd();

private:
	class Room& room;
	std::ostringstream buffer;
	typedef std::vector<class Creature*> IgnoreList;
	IgnoreList ignores;
};

// flush room output
void RoomStreamSink::streamEnd()
{
	// send output
	std::string text = buffer.str();
	if (!text.empty()) {
		if (ignores.empty())
			room.put(text, text.size());
		else
			room.put(text, text.size(), &ignores);
		buffer.clear();
	}
}

IStreamSink* Room::getStream()
{
	return new RoomStreamSink(*this);
}

StreamControl::StreamControl(Room& rptr) : sink(new RoomStreamSink(rptr)) {}

BEGIN_EFACTORY(Room)
return new Room();
END_EFACTORY
