/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include "common/imanager.h"
#include "mud/account.h"
#include "mud/server.h"

#define MAX_COMMAND_ARGS 10
#define MAX_COMMAND_WORDS 25

class Creature;
class Player;

typedef void (*CreatureCommandFunc)(class Creature*, std::string argv[]);  // can manipulate argv
typedef void (*PlayerCommandFunc)(class Player*, std::string argv[]);  // can manipulate argv

struct CommandFormatNode {
	enum type_t { NONE, WORD, PHRASE, LITERAL } type; // type
	int arg; // argument index
	bool opt; // optional, can skip?
	std::string literal; // text to match

	// new LITERAL node
	CommandFormatNode(int s_arg, bool s_opt, const std::string& s_literal) :
			type(LITERAL), arg(s_arg), opt(s_opt), literal(s_literal) {}
	// new WORD or PHRASE node
	CommandFormatNode(type_t s_type, int s_arg, bool s_opt) :
			type(s_type), arg(s_arg), opt(s_opt), literal() {}
	// copy contrusctor
	CommandFormatNode(const CommandFormatNode& node) :
			type(node.type), arg(node.arg), opt(node.opt), literal(node.literal) {}
};

class CommandFormat
{
public:
	CommandFormat(class Command* s_command, const std::string& format, CreatureCommandFunc s_func, int s_priority = 100) : command(s_command), ch_func(s_func), ply_func(NULL), priority(s_priority) { build(format); }
	CommandFormat(class Command* s_command, const std::string& format, PlayerCommandFunc s_func, int s_priority = 100) : command(s_command), ch_func(NULL), ply_func(s_func), priority(s_priority) { build(format); }

	// get the basics
	inline std::string getFormat() const { return format; }
	inline int getPriority() const { return priority; }

	// construct format; return non-zero on failure
	int build(const std::string& format);

	// get the command desc
	inline Command* getCommand() const { return command; }

	// sort
	bool operator< (const CommandFormat& format) const;

	// attempt a match; will mangle line during work, unmangles on
	// non-match, leaves mangled on match.  return -1 on perfect match,
	// 0 on no match, and positive on partial match; closer the match,
	// higher the positive return value.  The argv MUST be at least
	// COMMAND_MAX_ARGS elements in size.
	inline int match(char** words, std::string argv[]) const { return trymatch(0, words, argv); }

private:
	class Command* command;
	std::string format;
	CreatureCommandFunc ch_func;
	PlayerCommandFunc ply_func;
	int priority;
	std::vector<CommandFormatNode> nodes;

	// do the matching
	int trymatch(int node, char** words, std::string argv[]) const;

	// let command manager see us
	friend class _MCommand;
};

class Command
{
private:
	// data
	std::string name;
	std::string usage;
	AccessID access; // required permission

public:
	// constructor/destructor - virtual
	inline Command(const std::string& s_name, const std::string& s_usage, AccessID s_access) : name(s_name), usage(s_usage), access(s_access)  {}

	// basics
	const std::string& getName() const { return name; }
	const std::string& getUsage() const { return usage; }
	AccessID getAccess() const { return access; }

	// display help
	void showMan(class StreamControl& player);

	// sort
	bool operator< (const Command& command) const;

	// let command manager see us
	friend class _MCommand;
};

class _MCommand : public IManager
{
public:
	// initialize
	virtual int initialize();

	// shutdown
	virtual void shutdown();

	// add a new command
	void add(Command* command);
	void add(const CommandFormat& format) { formats.push_back(format); }

	// invoke a command
	int call(class Creature* character, const std::string& cmd_line);

	// show a man page; return false if cmd_name is not found
	bool showMan(class StreamControl& stream, const std::string& cmd_name, bool quiet = false);

	// show a command list
	void showList(class Player* player);

private:
	typedef std::vector<Command*> CommandList;
	CommandList commands;
	std::vector<CommandFormat> formats;
};
extern _MCommand MCommand;

namespace commands
{
	char *getArg(char **);  /* updates pointer, returns arg */
	bool isArg(char *);
	char *fixArg(char *, char **);  /* give an arg and current pointer, it moves pointer back and unsets the null bit, making it reparsable/complete */
	char *restore(char *, char **);  /* like fix_arg, but fixes from a start location to the current location, restoring possibly multiple arguments */
}

#define COMMAND(name,usage,func,access,klass) \
	{ \
		extern void func (klass*, std::string[]); \
		void (*fptr)(klass*, std::string[]) = func; \
		Command* command = new Command(name,usage,access);
#define FORMAT(priority, format) add(CommandFormat(command, format, (fptr), (priority)));
#define END_COMM add(command); }

#endif
