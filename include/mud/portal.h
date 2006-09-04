/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_PORTAL_H
#define AWEMUD_MUD_PORTAL_H

#include "mud/entity.h"
#include "common/string.h"
#include "scriptix/function.h"

// Direction object
class PortalDir {
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
		COUNT
	} dir_t;

	private:
	dir_t value;

	static const String names[];
	static dir_t opposites[];
	
	public:
	inline PortalDir (int s_value) : value((dir_t)s_value) {}
	inline PortalDir () : value(NONE) {}

	inline String get_name() const { return names[value]; }
	inline PortalDir get_opposite() const { return opposites[value]; }
	inline bool valid () const { return value != NONE; }

	inline dir_t get_value () const { return value; }

	static PortalDir lookup (String name);

	inline bool operator == (const PortalDir& dir) const { return dir.value == value; }
	inline bool operator != (const PortalDir& dir) const { return dir.value != value; }
	inline operator bool () const { return valid(); }
};

// Portal usage object
class PortalUsage {
	public:
	typedef enum {
		WALK = 0,
		CLIMB,
		CRAWL,
		COUNT
	} type_t;

	private:
	type_t value;

	static const String names[];
	
	public:
	inline PortalUsage (int s_value) : value((type_t)s_value) {}
	inline PortalUsage () : value(WALK) {}

	inline String get_name() const { return names[value]; }

	inline type_t get_value () const { return value; }

	static PortalUsage lookup (String name);

	inline bool operator == (const PortalUsage& dir) const { return dir.value == value; }
	inline bool operator != (const PortalUsage& dir) const { return dir.value != value; }
};

// Portal flavour detail
class PortalDetail  {
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

	static const String names[];
	
	public:
	inline PortalDetail (int s_value) : value((type_t)s_value) {}
	inline PortalDetail () : value(NONE) {}

	inline String get_name() const { return names[value]; }

	inline type_t get_value () const { return value; }

	static PortalDetail lookup (String name);

	inline bool operator == (const PortalDetail& dir) const { return dir.value == value; }
	inline bool operator != (const PortalDetail& dir) const { return dir.value != value; }
};

// Room portals.  These define things like doors, general directions (west,
// east), and so on
class Portal : public Entity
{
	public:
	Portal ();

	// name information
	virtual EntityName get_name () const;
	bool set_name (String s_name) { return name.set_name(s_name); }
	void add_keyword (String keyword);

	// description information
	virtual String get_desc () const { return desc; }
	virtual void set_desc (String s_desc) { desc = s_desc; }

	// 'standard' portals have no custom name
	inline bool is_standard () const { return name.get_name().empty(); }

	// the taget room and portal (target portal is the portal you come out of)
	inline String get_target () const { return target; }
	inline void set_target (String t) { target = t; }

	// ownership - see entity.h
	virtual void set_owner (Entity*);
	virtual Entity* get_owner () const;
	virtual void owner_release (Entity*);
	virtual inline class Room *get_room () const { return parent_room; }
	
	// get the TARGET room and portal
	class Room *get_target_room () const;
	Portal *get_target_portal () const;

	// get a function to call when used
	inline const Scriptix::ScriptFunction& get_use () const { return on_use; }
	void set_use (String script);

	// enter message
	String get_enters () const;
	inline void set_enters (String t) { text.enters = t; }

	// leave message
	String get_leaves () const;
	inline void set_leaves (String t) { text.leaves = t; }

	// go message
	String get_go () const;
	inline void set_go (String t) { text.go = t; }

	// portal usage (climb, etc.)
	inline PortalUsage get_usage () const { return usage; }
	inline void set_usage (PortalUsage t) { usage = t; }

	// portal detail (on, over, etc.)
	inline PortalDetail get_detail () const { return detail; }
	inline void set_detail (PortalDetail t) { detail = t; }

	// direction (east, west, etc.)
	inline PortalDir get_dir () const { return dir; }
	inline void set_dir (PortalDir d) { dir = d; }

	// flags
	bool is_valid () const;
	inline bool is_hidden () const { return flags.hidden; }
	inline bool is_closed () const { return flags.closed; }
	inline bool is_synced () const { return flags.synced; }
	inline bool is_locked () const { return flags.locked; }
	inline bool is_door () const { return flags.door; }
	inline bool is_nolook () const { return flags.nolook; }
	inline bool is_disabled () const { return flags.disabled; }

	// color of portal
	virtual String ncolor () const { return S(CEXIT); }

	// manage state
	void lock ();
	void unlock ();
	void close ();
	void open ();

	// heartbeat
	void heartbeat ();

	virtual bool name_match (String name) const;

	// set flags
	inline void set_door (bool v) { flags.door = v; }
	inline void set_hidden (bool v) { flags.hidden = v; }
	inline void set_closed (bool v) { flags.closed = v; }
	inline void set_synced (bool v) { flags.synced = v; }
	inline void set_locked (bool v) { flags.locked = v; }
	inline void set_nolook (bool v) { flags.nolook = v; }
	inline void set_disabled (bool v) { flags.disabled = v; }

	// IO
	virtual void save (File::Writer& writer);
	virtual void save_hook (class ScriptRestrictedWriter* writer);
	virtual int load_node(File::Reader& reader, File::Node& node);
	virtual int load_finish ();

	// sort
	bool operator< (const Portal& portal) const;

	protected:
	// data members
	EntityName name;
	String desc;
	String target;
	struct {
		String go;
		String leaves;
		String enters;
	} text;
	PortalDir dir;
	PortalUsage usage;
	PortalDetail detail;
	class Room *parent_room;
	StringList keywords;

	// flags
	struct Flags {
		char hidden:1, closed:1, synced:1, locked:1, door:1, nolook:1,
			disabled:1;

		Flags() : hidden(false), closed(false), synced(true), locked(false),
			door(false), nolook(false), disabled(false) {}
	} flags;

	// scripts
	Scriptix::ScriptFunctionSource on_use;

	protected:
	~Portal () {}

	// extra
	friend class Room;
	E_TYPE(Portal)
};

#define PORTAL(ent) E_CAST(ent,Portal)

#endif
