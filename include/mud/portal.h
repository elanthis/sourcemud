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

	std::string getName() const { return names[value]; }
	std::string getAbbr() const { return abbreviations[value]; }
	PortalDir getOpposite() const { return opposites[value]; }
	bool valid() const { return value != NONE; }

	dir_t getValue() const { return value; }

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

	std::string getName() const { return names[value]; }

	type_t getValue() const { return value; }

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

	std::string getName() const { return names[value]; }

	type_t getValue() const { return value; }

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

	virtual const char* factoryType() const { return "portal"; }

	// name information
	virtual EntityName getName() const;
	bool setName(const std::string& s_name) { return name.setFull(s_name); }
	void addKeyword(const std::string& keyword);

	// description information
	virtual std::string getDesc() const { return desc; }
	virtual void setDesc(const std::string& s_desc) { desc = s_desc; }

	// 'standard' portals have no custom name
	bool isStandard() const { return name.empty(); }

	// the taget room and portal (target portal is the portal you come out of)
	std::string getTarget() const { return target; }
	void setTarget(const std::string& t) { target = t; }

	// movement messages based on usage/detail
	std::string getGo() const;
	std::string getLeaves() const;
	std::string getEnters() const;

	// ownership - see entity.h
	virtual void setOwner(Entity*);
	virtual Entity* getOwner() const;
	virtual void ownerRelease(Entity*);
	class Room* getRoom() const { return parent_room; }

	// events
	virtual void handleEvent(const Event& event);
	virtual void broadcastEvent(const Event& event);

	// activate/deactivtee
	virtual void activate();
	virtual void deactivate();

	// portal usage (climb, etc.)
	PortalUsage getUsage() const { return usage; }
	void setUsage(PortalUsage t) { usage = t; }

	// portal detail (on, over, etc.)
	PortalDetail getDetail() const { return detail; }
	void setDetail(PortalDetail t) { detail = t; }

	// direction (east, west, etc.)
	PortalDir getDir() const { return dir; }
	void setDir(PortalDir d) { dir = d; }

	// get relative dir and target
	Room* getRelativeTarget(Room* base) const;
	PortalDir getRelativeDir(Room* base) const;
	Portal* getRelativePortal(Room* base) const;
	bool hasRoom(Room* base) const;

	// flags
	bool isValid() const;
	bool isHidden() const { return flags.hidden; }
	bool isClosed() const { return flags.closed; }
	bool isOneway() const { return flags.oneway; }
	bool isLocked() const { return flags.locked; }
	bool isDoor() const { return flags.door; }
	bool isNolook() const { return flags.nolook; }
	bool isDisabled() const { return flags.disabled; }

	// color of portal
	virtual const char* ncolor() const { return CEXIT; }

	// manage state
	void lock(Room* base, class Creature* actor);
	void unlock(Room* base, class Creature* actor);
	void close(Room* base, class Creature* actor);
	void open(Room* base, class Creature* actor);

	// heartbeat
	void heartbeat();

	virtual bool nameMatch(const std::string& name) const;

	// set flags
	void setDoor(bool v) { flags.door = v; }
	void setHidden(bool v) { flags.hidden = v; }
	void setClosed(bool v) { flags.closed = v; }
	void setOneway(bool v) { flags.oneway = v; }
	void setLocked(bool v) { flags.locked = v; }
	void setNolook(bool v) { flags.nolook = v; }
	void setDisabled(bool v) { flags.disabled = v; }

	// IO
	virtual void saveData(File::Writer& writer);
	virtual void saveHook(File::Writer& writer);
	virtual int loadNode(File::Reader& reader, File::Node& node);
	virtual int loadFinish();

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
};

#define PORTAL(ent) E_CAST(ent,Portal)

#endif
