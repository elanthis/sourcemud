/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef ENTITY_H
#define ENTITY_H

#include <list>
#include <vector>
#include <algorithm>
#include <map>

#include "common/types.h"
#include "common/error.h"
#include "common/string.h"
#include "common/imanager.h"
#include <set>
#include "mud/uniqid.h"
#include "mud/event.h"
#include "mud/color.h"
#include "mud/tag.h"
#include "mud/server.h"
#include "mud/clock.h"
#include "mud/macro.h"
#include "mud/name.h"

#define E_SUBTYPE(name,par) \
	public: \
	typedef par _parent_type; \
	inline static const void *get_setype () { return (const void *)name::get_setype; } \
	virtual bool check_etype (const void *type) const { return (type == name::get_setype ()) || par::check_etype(type); }
#define E_TYPE(name) E_SUBTYPE(name,Entity)

// for the global entity list
typedef std::list<Entity*> EntityList; // NOTE: no gc_alloc, don't want GC to scan this
typedef std::set<TagID> TagList;
typedef std::multimap<TagID, Entity*> TagTable; // NOTE: also non-GC scanning
typedef std::vector<EventHandler*> EventList;
typedef std::map<UniqueID, Entity*> UniqueIDMap; // NOTE: also non-GC scanning

// --- Entity Definiton ---

// entity control
class Entity : public IMacroObject
{
	public:
	Entity();

	// factory handling
	virtual String factory_type () const = 0;
	static Entity* create (String type); // efactory.cc

	// get unique ID
	inline const UniqueID& get_uid () const { return uid; }

	// name stuff
	virtual EntityName get_name () const = 0;

	// description
	virtual String get_desc () const = 0;

	// events
	inline const EventList& get_events () const { return events; }
	EventHandler* get_event (EventID event);

	// active
	inline bool is_active () const { return flags.active; }
	virtual void activate (); // subclasses should call Entity::activate() first, then do custom code
	virtual void deactivate (); // subclasses do custom code, then call Entity::deactivate last

	// destroy() will remove the entity from its parent using
	// get_owner()->owner_release(this).  over-ridable for any necessary
	// custom destroy behaviour; *must* call Entity::destroy()
	virtual void destroy ();

	// tags
	bool has_tag (TagID tag) const;
	int add_tag (TagID tag);
	int remove_tag (TagID tag);
	inline const TagList& get_tags () const { return tags; }

	// output
	virtual void display_desc (const class StreamControl& stream) const;
	virtual String ncolor () const { return S(CNORMAL); }

	// save/load
	static Entity* load (String factory, File::Reader& reader);
	int load (File::Reader& reader);
	virtual int load_node (File::Reader& reader, File::Node& node);
	virtual int load_finish () = 0;
	void save (File::Writer& writer, String ns, String name);
	virtual void save_data (File::Writer& writer);
	virtual void save_hook (File::Writer& writer);

	// check name
	virtual bool name_match (String name) const;

	// event triggers
	virtual void handle_event (const Event& event);
	virtual void broadcast_event (const Event& event) = 0;

	// for parsing, pull a property based on a char
	virtual int macro_property (const class StreamControl& stream, String method, const MacroList& argv) const;
	virtual void macro_default (const class StreamControl& stream) const;

	// heartbeat
	virtual void heartbeat () = 0;

	// sorting
	bool operator< (const Entity& ent) const;

	// our custom type checking system
	inline static const void *get_setype ()
		{ return (const void *)Entity::get_setype; }
	inline virtual bool check_etype (const void *type) const
		{ return (type == Entity::get_setype ()); }

	// owner management
	// set_owner() should *only* be called from an owning object's add*()
	// routines when adding the entity to the owner's child list.  get_owner()
	// returns the current owner.  owner_release() should *only* be called
	// from a child entity on its owner during a set_owner() operation.
	// set_owner() automatically sets the active state to the new owner's
	// active state.  set_owner() must be over-ridden to assert() on
	// invalid owner types and to set an owner pointer - it *MUST* call
	// Entity::set_owner() first.  get_owner() can be over-ridden to use a
	// sub-classed return type.  owner_release() must be over-ridden.
	virtual void set_owner (Entity* owner);
	virtual Entity* get_owner () const = 0;
	virtual void owner_release (Entity* child) = 0;

	// big list for updates
	private:
	UniqueID uid;
	EntityList::iterator eself; // for super-quick removal from list
	uint8 eheartbeat; // heart-beat index

	// various data
	protected:
	TagList tags;
	EventList events;

	// flags
	struct Flags {
		int active:1;
	} flags;

	// event handler
	int perform_event (EventHandler *ea, Entity* trigger, Entity* target);

	// protected destructor
	virtual ~Entity () {}
};

// manage all entities
class SEntityManager : public IManager
{
	public:
	SEntityManager ();
	~SEntityManager ();

	// initialize the system
	virtual int initialize ();

	// shutdown system
	virtual void shutdown ();

	// run a single heartbeat loop
	virtual void heartbeat ();

	// fetch by tag
	std::pair<TagTable::const_iterator, TagTable::const_iterator> tag_list (TagID tag) const;

	// fetch by ID
	Entity* get (const UniqueID& uid) const;

	// count by tag
	size_t tag_count (TagID tag) const;

	private:
	EntityList elist[TICKS_PER_ROUND]; // all entities in system
	EntityList::iterator ecur; // current entity for update
	uint8 eheartbeat; // current heartbeat

	// lookup by uniqid
	UniqueIDMap id_map;

	// tag map: no GC
	TagTable tag_map;

	// Entities have to be able to manage list - ick
	// also need to manage tag_map - double ick
	friend class Entity;
};
extern SEntityManager EntityManager;

// --- CASTING/TYPE-CHECKING ---

template <class TYPE>
struct _e_cast {
	static inline TYPE* cast (Entity* base) {
		return (base && base->check_etype(TYPE::get_setype())) ? (TYPE*)(base) : NULL;
	}
	static inline const TYPE* cast (const Entity* base) {
		return (base && base->check_etype(TYPE::get_setype())) ? (TYPE*)(base) : NULL;
	}
};

#define E_CAST(ENT,TYPE) (_e_cast<TYPE>::cast((ENT)))
#define ENTITY(ENT) E_CAST(ENT,Entity)

#endif
