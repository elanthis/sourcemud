/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include "common/string.h"
#include "mud/account.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "common/gcvector.h"
#include "scriptix/function.h"

#define MAX_COMMAND_ARGS 10
#define MAX_COMMAND_WORDS 25

class Character;
class Player;

typedef void (*CharCommandFunc) (class Character*, String argv[]); // can manipulate argv
typedef void (*PlayerCommandFunc) (class Player*, String argv[]); // can manipulate argv

// a parsable command format string
class CommandFormat : public GC
{
	public:
	CommandFormat (class Command* s_command, int s_priority = 100) : command(s_command), nodes(), ch_func(NULL), ply_func(NULL), priority(s_priority) {}

	// set command callback
	inline void clear_callback (void) { ch_func = NULL; ply_func = NULL; script = Scriptix::ScriptFunction(); }
	inline void set_callback (CharCommandFunc s_func) { clear_callback(); ch_func = s_func; }
	inline void set_callback (PlayerCommandFunc s_func) { clear_callback(); ply_func = s_func; }
	inline void set_callback (Scriptix::ScriptFunction s_func) { clear_callback(); script = s_func; }

	// get the basics
	inline StringArg get_format (void) const { return format; }
	inline int get_priority (void) const { return priority; }

	// construct format; return non-zero on failure
	int build (StringArg format);

	// get the command desc
	inline Command* get_command (void) const { return command; }

	// sort
	bool operator< (const CommandFormat& format) const;

	// attempt a match; will mangle line during work, unmangles on
	// non-match, leaves mangled on match.  return -1 on perfect match,
	// 0 on no match, and positive on partial match; closer the match,
	// higher the positive return value.  The argv MUST be at least
	// COMMAND_MAX_ARGS elements in size.
	inline int match (char** words, String argv[]) const { return trymatch(0, words, argv); }

	private:
	class Command* command;
	String format;
	struct FormatNode : public GC
	{
		enum type_t { NONE, ONE, MANY, TEXT, LIST } type; // type
		int arg; // argument index
		bool opt; // optional, can skip?
		StringList list; // text to match

		// new TEXT node
		FormatNode(int s_arg, bool s_opt, StringArg s_text) :
			type(TEXT), arg(s_arg), opt(s_opt), list() { list.push_back(s_text); }
		// new LIST node
		FormatNode(int s_arg, bool s_opt, const StringList& s_list) :
			type(LIST), arg(s_arg), opt(s_opt), list(s_list) {}
		// new WORD/STRING node
		FormatNode(type_t s_type, int s_arg, bool s_opt) :
			type(s_type), arg(s_arg), opt(s_opt), list() {}
		// copy contrusctor
		FormatNode(const FormatNode& node) :
			type(node.type), arg(node.arg), opt(node.opt), list(node.list) {}
	};
	typedef GCType::vector<FormatNode> NodeList;
	NodeList nodes;
	Scriptix::ScriptFunction script;
	CharCommandFunc ch_func;
	PlayerCommandFunc ply_func;
	int priority;

	// do the matching
	int trymatch (int node, char** words, String argv[]) const;

	// let command manager see us
	friend class SCommandManager;
};

class Command : public GC
{
	private:
	// data
	String name;
	String usage;
	typedef GCType::vector<CommandFormat*> FormatList;
	FormatList formats;
	AccessID access; // required permission

	public:
	// constructor/destructor - virtual
	inline Command (StringArg s_name, StringArg s_usage, AccessID s_access) : name(s_name), usage(s_usage), formats (), access (s_access)  {}

	// basics
	const String& get_name (void) const { return name; }
	const String& get_usage (void) const { return usage; }
	AccessID get_access (void) const { return access; }

	// add a new format
	inline void add_format (CommandFormat* format) { formats.push_back(format); }

	// display help
	void show_man (class Player* player);

	// sort
	bool operator< (const Command& command) const;

	// let command manager see us
	friend class SCommandManager;
};

class SCommandManager : public IManager
{
	public:
	// initialize
	virtual int initialize (void);

	// shutdown
	virtual void shutdown (void);

	// add a new command
	int add (Command* command);

	// invoke a command
	int call (class Character* character, StringArg cmd_line);

	// show a man page; return false if cmd_name is not found
	bool show_man (class Player* player, StringArg cmd_name, bool quiet = false);

	// show a command list
	void show_list (class Player* player);

	private:
	typedef GCType::vector<Command*> CommandList;
	typedef GCType::vector<CommandFormat*> FormatList;
	
	CommandList commands;
	FormatList formats;
};
extern SCommandManager CommandManager;

namespace commands {
	char *get_arg (char **); /* updates pointer, returns arg */
	bool is_arg (char *);
	char *fix_arg (char *, char **); /* give an arg and current pointer, it moves pointer back and unsets the null bit, making it reparsable/complete */
	char *restore (char *, char **); /* like fix_arg, but fixes from a start location to the current location, restoring possibly multiple arguments */
}

#endif
