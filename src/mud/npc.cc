/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/file.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/body.h"
#include "mud/npc.h"
#include "mud/room.h"
#include "mud/macro.h"
#include "mud/settings.h"
#include "mud/object.h"
#include "mud/skill.h"
#include "mud/hooks.h"
#include "mud/shadow-object.h"
#include "mud/efactory.h"

Npc::Npc() : Creature()
{
	initialize();
}

Npc::Npc (NpcBP* s_blueprint) : Creature()
{
	initialize();
	blueprint = NULL;
	set_blueprint(s_blueprint);
}

void
Npc::initialize()
{
	blueprint = NULL;
	flags.zonelock = false;
	flags.revroomtag = false;
	room_tag = TagID();
}

Npc::~Npc()
{
}

int
Npc::load_finish()
{
	if (Creature::load_finish())
		return -1;
	
	if (blueprint == NULL) {
		Log::Error << "NPC has no blueprint";
		return -1;
	}
		
	return 0;
}

int
Npc::load_node(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("npc", "blueprint")
			NpcBP* blueprint;
			if ((blueprint = MNpcBP.lookup(node.get_string())) == NULL)
				Log::Error << "Could not find npc blueprint '" << node.get_string() << "'";
			else
				set_blueprint(blueprint);
		FO_ATTR("npc", "roomtag")
			room_tag = TagID::create(node.get_string());
		FO_ATTR("npc", "zonelock")
			flags.zonelock = node.get_bool();
		FO_ATTR("npc", "reverse_roomtag")
			flags.revroomtag = node.get_bool();
		FO_PARENT(Creature)
	FO_NODE_END
}

void
Npc::save_data(File::Writer& writer)
{
	if (get_blueprint())
		writer.attr("npc", "blueprint", get_blueprint()->get_id());

	Creature::save_data(writer);

	if (room_tag.valid())
		writer.attr("npc", "roomtag", TagID::nameof(room_tag));
	if (flags.zonelock)
		writer.attr("npc", "zonelock", true);
	if (flags.revroomtag)
		writer.attr("npc", "reverse_roomtag", true);
}

void
Npc::save_hook(File::Writer& writer)
{
	Creature::save_hook(writer);
	Hooks::save_npc(this, writer);
}
	
EntityName
Npc::get_name() const
{
	assert(blueprint != NULL);
	return blueprint->get_name();
}

std::string
Npc::get_desc() const
{
	assert(blueprint != NULL);
	return blueprint->get_desc();
}

GenderType
Npc::get_gender() const
{
	assert(blueprint != NULL);
	return blueprint->get_gender();
}

int
Npc::get_base_stat(CreatureStatID stat) const
{
	assert(blueprint != NULL);
	return blueprint->get_stat(stat);
}

uint
Npc::get_combat_dodge() const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_dodge();
}

uint
Npc::get_combat_attack() const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_attack();
}

uint
Npc::get_combat_damage() const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_damage();
}

void
Npc::kill(Creature *killer)
{
	// death message
	if (get_room())
		*get_room() << StreamName(this, DEFINITE, true) << " has been slain!\n";

	// lay down, drop crap
	position = CreaturePosition::LAY;

	// FIXME EVENT
	if (!Hooks::npc_death(this, killer)) {
		// only if there's no hook - hooks must do this for us!
		destroy();
	}
}

void
Npc::handle_event(const Event& event)
{
	// normal event handler
	Entity::handle_event(event);
}

void
Npc::heartbeat()
{
	// do creature update
	Creature::heartbeat();

	// update handler
	Hooks::npc_heartbeat(this);
}

void
Npc::set_blueprint(NpcBP* s_blueprint)
{
	blueprint = s_blueprint;
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		Creature::set_effective_stat(CreatureStatID(i), get_base_stat(CreatureStatID(i)));
	Creature::recalc();
}

// load npc from a blueprint
Npc*
Npc::load_blueprint(const std::string& name)
{
	// lookup the blueprint
	NpcBP* blueprint = MNpcBP.lookup(name);
	if (!blueprint)
		return NULL;
	
	// create it
	Npc* npc = new Npc(blueprint);
	if (npc == NULL)
		return NULL;

	// equip
	while (blueprint != NULL) {
		for (std::vector<std::string>::const_iterator i = blueprint->get_equip_list().begin(); i != blueprint->get_equip_list().end(); ++i) {
			Object* object = ShadowObject::load_blueprint(*i);
			if (object != NULL)
				npc->equip(object);
			else
				Log::Error << "Object blueprint '" << *i << "' from NPC blueprint '" << blueprint->get_id() << "' does not exist.";
		}
		blueprint = blueprint->get_parent();
	}

	// set HP
	npc->set_hp(npc->get_max_hp());

	return npc;
}

// display NPC description
void
Npc::display_desc(const StreamControl& stream)
{
	if (!get_desc().empty())
		stream << StreamMacro(get_desc(), "npc", this); // FIXME: re-enable 'actor'(looker)
	else
		stream << StreamName(this, DEFINITE, true) << " doesn't appear very interesting.";
}

bool
Npc::can_use_portal(Portal* portal) const
{
	assert(portal != NULL);

	// disabled portals can't be used
	if (portal->is_disabled())
		return false;

	// portal's room is our room, yes?
	if (!portal->has_room(get_room()))
		return false;

	// get target room; must be valid
	Room* troom = portal->get_relative_target(get_room());
	if (troom == NULL)
		return false;

	// check zone constraints
	if (is_zone_locked() && troom->get_zone() != get_room()->get_zone())
		return false;

	// check room tag constraints
	if (room_tag.valid()) {
		// does the room have the tag?
		if (troom->has_tag(room_tag)) {
			// reversed?
			if (is_room_tag_reversed())
				return false;
		// does not have tag
		} else {
			// required it?
			if (!is_room_tag_reversed())
				return false;
		}
	}

	// FIXME: check portal usage types; climb, crawl, etc

	// no failures - all good
	return true;
}

bool
Npc::is_blueprint(const std::string& name) const
{
	NpcBP* blueprint = get_blueprint();

	while (blueprint != NULL) {
		if (str_eq(blueprint->get_id(), name))
			return true;

		blueprint = blueprint->get_parent();
	}

	return false;
}

bool
Npc::name_match(const std::string& match) const
{
	if (get_name().matches(match))
		return true;

	// blueprint keywords
	NpcBP* blueprint = get_blueprint();
	while (blueprint != NULL) {
		for (std::vector<std::string>::const_iterator i = blueprint->get_keywords().begin(); i != blueprint->get_keywords().end(); i ++)
			if (phrase_match (*i, match))
				return true;

		blueprint = blueprint->get_parent();
	}

	// no match
	return false;
}

BEGIN_EFACTORY(NPC)
	return new Npc();
END_EFACTORY
