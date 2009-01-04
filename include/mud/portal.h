/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_PORTAL_H
#define SOURCEMUD_MUD_PORTAL_H

#include "mud/entity.h"

// Direction object
class PortalDir
{
public:
	// the directions
	typedef enum {
		NONE = 0,
		NORTH,
		EAST,
		SOUTH,
		WEST,
		NORTHWEST,
		NORTHEAST,
		SOUTHEAST,
		SOUTHWEST,
		UP,
		DOWN,
		COUNT
	} dir_t;

private:
	dir_t value;

	static const std::string names[];
	static const std::string abbreviations[];
	static dir_t opposites[];

public:
	PortalDir(int s_value) : value((dir_t)s_value) {}
	PortalDir() : value(NONE) {}

	std::string get_name() const { return names[value]; }
	std::string get_abbr() const { return abbreviations[value]; }
	PortalDir get_opposite() const { return opposites[value]; }
	bool valid() const { return value != NONE; }

	dir_t get_value() const { return value; }

	static PortalDir lookup(const std::string& name);

	bool operator < (const PortalDir& dir) const { return value < dir.value; }
	bool operator == (const PortalDir& dir) const { return dir.value == value; }
	bool operator != (const PortalDir& dir) const { return dir.value != value; }
	operator bool() const { return valid(); }
};

// Portal usage object
class PortalUsage
{
public:
	typedef enum {
		WALK = 0,
		CLIMB,
		CRAWL,
		COUNT
	} type_t;

private:
	type_t value;

	static const std::string names[];

public:
	PortalUsage(int s_value) : value((type_t)s_value) {}
	PortalUsage() : value(WALK) {}

	std::string get_name() const { return names[value]; }

	type_t get_value() const { return value; }

	static PortalUsage lookup(const std::string& name);

	bool operator == (const PortalUsage& dir) const { return dir.value == value; }
	bool operator != (const PortalUsage& dir) const { return dir.value != value; }
};

// Portal flavour detail
class PortalDetail
{
public:
	typedef enum {
		NONE = 0,
		IN,
		ON,
		OVER,
		UNDER,
		ACROSS,
		OUT,
		UP,
		DOWN,
		THROUGH,
		COUNT
	} type_t;

private:
	type_t value;

	static const std::string names[];

public:
	PortalDetail(int s_value) : value((type_t)s_value) {}
	PortalDetail() : value(NONE) {}

	std::string get_name() const { return names[value]; }

	type_t get_value() const { return value; }

	static PortalDetail lookup(const std::string& name);

	bool operator == (const PortalDetail& dir) const { return dir.value == value; }
	bool operator != (const PortalDetail& dir) const { return dir.value != value; }
};

// Room portals.  These define things like doors, general directions (west,
// east), and so on
class Portal : public Entity
{
public:
	Portal();

	virtual const char* factory_type() const { return "portal"; }

	// name information
	virtual EntityName get_name() const;
	bool set_name(const std::string& s_name) { return name.set_name(s_name); }
	void add_keyword(const std::string& keyword);

	// description information
	virtual std::string get_desc() const { return desc; }
	virtual void set_desc(const std::string& s_desc) { desc = s_desc; }

	// 'standard' portals have no custom name
	bool is_standard() const { return name.empty(); }

	// the taget room and portal (target portal is the portal you come out of)
	std::string get_target() const { return target; }
	void set_target(const std::string& t) { target = t; }

	// movement messages based on usage/detail
	std::string get_go() const;
	std::string get_leaves() const;
	std::string get_enters() const;

	// ownership - see entity.h
	virtual void set_owner(Entity*);
	virtual Entity* get_owner() const;
	virtual void owner_release(Entity*);
	class Room* get_room() const { return parent_room; }

	// events
	virtual void handle_event(const Event& event);
	virtual void broadcast_event(const Event& event);

	// activate/deactivtee
	virtual void activate();
	virtual void deactivate();

	// portal usage (climb, etc.)
	PortalUsage get_usage() const { return usage; }
	void set_usage(PortalUsage t) { usage = t; }

	// portal detail (on, over, etc.)
	PortalDetail get_detail() const { return detail; }
	void set_detail(PortalDetail t) { detail = t; }

	// direction (east, west, etc.)
	PortalDir get_dir() const { return dir; }
	void set_dir(PortalDir d) { dir = d; }

	// get relative dir and target
	Room* get_relative_target(Room* base) const;
	PortalDir get_relative_dir(Room* base) const;
	Portal* get_relative_portal(Room* base) const;
	bool has_room(Room* base) const;

	// flags
	bool is_valid() const;
	bool is_hidden() const { return flags.hidden; }
	bool is_closed() const { return flags.closed; }
	bool is_oneway() const { return flags.oneway; }
	bool is_locked() const { return flags.locked; }
	bool is_door() const { return flags.door; }
	bool is_nolook() const { return flags.nolook; }
	bool is_disabled() const { return flags.disabled; }

	// color of portal
	virtual const char* ncolor() const { return CEXIT; }

	// manage state
	void lock(Room* base, class Creature* actor);
	void unlock(Room* base, class Creature* actor);
	void close(Room* base, class Creature* actor);
	void open(Room* base, class Creature* actor);

	// heartbeat
	void heartbeat();

	virtual bool name_match(const std::string& name) const;

	// set flags
	void set_door(bool v) { flags.door = v; }
	void set_hidden(bool v) { flags.hidden = v; }
	void set_closed(bool v) { flags.closed = v; }
	void set_oneway(bool v) { flags.oneway = v; }
	void set_locked(bool v) { flags.locked = v; }
	void set_nolook(bool v) { flags.nolook = v; }
	void set_disabled(bool v) { flags.disabled = v; }

	// IO
	virtual void save_data(File::Writer& writer);
	virtual void save_hook(File::Writer& writer);
	virtual int load_node(File::Reader& reader, File::Node& node);
	virtual int load_finish();

protected:
	// data members
	EntityName name;
	std::string desc;
	std::string target;
	PortalDir dir;
	PortalUsage usage;
	PortalDetail detail;
	class Room* parent_room;
	std::vector<std::string> keywords;

	// flags
	struct Flags {
char hidden:
1, closed:
1, locked:
1, door:
1, nolook:
		1,
disabled:
1, oneway:
		1;

		Flags() : hidden(false), closed(false), locked(false),
				door(false), nolook(false), disabled(false), oneway(false) {}
	} flags;

protected:
	~Portal() {}

	// extra
	friend class Room;
	E_TYPE(Portal)
};

#define PORTAL(ent) E_CAST(ent,Portal)

#endif
