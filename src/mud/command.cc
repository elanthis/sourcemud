/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/command.h"
#include "mud/creature.h"
#include "mud/player.h"
#include "mud/color.h"
#include "mud/account.h"
#include "mud/macro.h"
#include "mud/help.h"
#include "mud/room.h"
#include "mud/object.h"
#include "mud/creature.h"
#include "net/telnet.h"

_MCommand MCommand;

// helper functions
namespace
{
	char*
	repair(char* word, int cnt)
	{
		while (--cnt > 0)
			word[strlen(word)] = ' ';
		return word;
	}
	char*
	repair(char** words)
	{
		int cnt = 0;
		while (words[cnt] != NULL)
			++cnt;
		return repair(words[0], cnt);
	}
}

namespace commands
{
	// get a single argument
	char *
	getArg(char **c)
	{
		char *arg;

		assert(c != NULL);

		// trim whitespace
		while (isspace(**c))
			++ (*c);

		// portal if blank
		if (**c == '\0') {
			return NULL;
		}

		arg = *c; // Mark start
		// search to next whitespace
		while (**c != '\0' && !isspace(**c))
			++ (*c);

		/* if we are not at the end of line, add null and point to
		 * next location */
		if (**c != '\0') {
			**c = '\0';
			++ (*c);
		}

		return arg;
	}

	bool
	isArg(char *comm)
	{
		if (comm == NULL)
			return false;

		// look for a non-whitespace char
		while (*comm != '\0') {
			if (!isspace(*comm))
				return true;
			++ comm;
		}

		return false;
	}

	// add an arg back into line
	char *
	fixArg(char *arg, char **c)
	{
		int len = strlen(arg);
		// if arg is the last available argument, then arg+len will point to c
		if (arg + len == *c) {
			// just move head back
			*c = arg;
			return arg;
		} else {
			// clear the nul that separates the two
			*c = arg;
			arg[len] = ' ';
		}

		return arg;
	}

	// restore whole line
	char *
	restore(char *start, char **c)
	{
		// loop until we hit end
		char *ci = start;
		while (ci != *c) {
			// if we hit a null, make it a space
			if (*ci == '\0')
				*ci = ' ';
			++ ci;
		}
		// restore line pointer
		*c = start;

		return start;
	}
}

// custom dereferencing sorting operator...
namespace
{
	template <typename T>
	class DerefSort
	{
	public:
		bool operator()(const T* f1, const T* f2) {
			return *f1 < *f2;
		}
	};
}

// add a command to the list
void _MCommand::add(Command *command)
{
	assert(command != NULL);

	// add the command
	CommandList::iterator ci = std::lower_bound(commands.begin(), commands.end(), command, DerefSort<Command>());
	commands.insert(ci, command);
}

int _MCommand::call(Creature *ch, const std::string& comm)
{
	Player *ply = PLAYER(ch);

	// break up words
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "%s", comm.c_str());
	char* lptr = buffer;
	char* words[MAX_COMMAND_WORDS + 1];
	char* sep;
	int windex = 0;
	while (windex < MAX_COMMAND_WORDS && (sep = strpbrk(lptr, " \t\n\r")) != NULL) {
		*sep = '\0';
		if (sep != lptr) {
			words[windex++] = lptr;
		}
		lptr = sep + 1;
	}
	if (strlen(lptr))
		words[windex++] = lptr;
	words[windex] = NULL;
	if (windex == 0)
		return 1;

	// find the command if we can, using the phrase command list
	std::string argv[MAX_COMMAND_ARGS];
	int best_score = -1, result = 0;
	CommandList best_cmds;
	for (std::vector<CommandFormat>::const_iterator f = formats.begin(), e = formats.end(); f != e; ++f) {
		Command* command = f->getCommand();
		// must check privileges
		if (!command->access.valid() || (ply != NULL && ply->getAccount()->hasAccess(command->access))) {
			// do match
			result = f->match(words, argv);
			if (result == -1) {
				// all good - call function
				if (f->ch_func) {
					// call char functions
					f->ch_func(ch, argv);
					return 0;
				} else if (f->ply_func && ply) {
					// call player function
					f->ply_func(ply, argv);
					return 0;
				} else {
					// damnit
					return 1;
				}
			} else if (result != 0 && result > best_score) {
				// new "high score"
				best_score = result;
				best_cmds.clear();
				best_cmds.push_back(command);
			} else if (result == best_score) {
				// if not already in there...
				if (std::find(best_cmds.begin(), best_cmds.end(), command) == best_cmds.end())
					best_cmds.push_back(command);
			}
			// clear args
			for (int i = 0; i < MAX_COMMAND_ARGS; ++i)
				argv[i].clear();
		}
	}

	// had we a best command?
	if (!best_cmds.empty()) {
		Player* player = PLAYER(ply);
		if (player) {
			*player << CSPECIAL "Usage:" CNORMAL << "\n";
			player->setIndent(2);
			for (CommandList::iterator i = best_cmds.begin(); i != best_cmds.end(); ++i)
				*player << (*i)->usage;
			player->setIndent(0);
			return 1;
		}
	}

	// absolutely no command
	*ch << "I do not understand.\n";
	return 1;
}

void _MCommand::showList(Player *player)
{
	int col, max_col;
	size_t colwidth;

	// max width
	colwidth = 1;
	for (CommandList::iterator i = commands.begin(); i != commands.end(); ++i)
		if ((*i)->name.size() >= colwidth)
			colwidth = (*i)->name.size() + 1;

	// calculate column size
	max_col = (player->getWidth() - 3) / colwidth; // figure max size
	if (max_col <= 0)
		max_col = 3;

	// initialize loop meta info
	col = 0;
	*player << CSPECIAL "Command List:\n" CNORMAL;
	player->setIndent(2);

	// loop over all commands
	for (CommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		// skip empty commands
		if ((*i)->name.empty())
			continue;

		// must check privileges
		if ((*i)->access.valid() && (player == NULL || !player->getAccount()->hasAccess((*i)->access)))
			continue;

		// print command
		*player << (*i)->name;

		// increment column
		col += (*i)->name.size() / colwidth + 1;
		if (col >= max_col) {
			*player << "\n";
			col = 0;
		}

		// set indent
		player->setIndent(col * colwidth + 2);
	}

	// finish off
	if (col != 0)
		*player << "\n";
	player->setIndent(0);
}

void Command::showMan(StreamControl& stream)
{
	HelpTopic* topic = MHelp.getTopic(name);

	stream << CSPECIAL "Help: " CNORMAL << name << "\n\n";

	if (topic != NULL) {
		stream << StreamIndent(2);
		stream << CSPECIAL "About:" CNORMAL "\n";
		stream << StreamIndent(4);
		stream << StreamMacro(topic->about) << "\n";
	}
	stream << StreamIndent(2);
	stream << CSPECIAL "Usage:" CNORMAL "\n";
	stream << StreamIndent(4);
	stream << usage << "\n";
	stream << StreamIndent(0);
}

// build command
int CommandFormat::build(const std::string& s_format)
{
	int arg;
	bool opt;
	CommandFormatNode::type_t type;
	std::vector<std::string> words;

	// set our format
	format = s_format;

	// modifiable string
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "%s", format.c_str());

	// loop thru format args, parsing
	char* word;
	char* fmt = buffer;
	while ((word = commands::getArg(&fmt)) != NULL) {
		// reset
		arg = -1;
		opt = false;
		type = CommandFormatNode::NONE;
		words.clear();

		// parse the argument
		char* c;
		for (c = word; *c != '\0'; ++c) {
			// optional?
			if (*c == '?')
				opt = true;
			// word?
			else if (*c == '%')
				type = CommandFormatNode::WORD;
			// any-string?
			else if (*c == '*')
				type = CommandFormatNode::PHRASE;
			// arg index?
			else if (*c == ':') {
				arg = 0;
				// loop thru digits
				++c;
				while (*c != '\0' && isdigit(*c)) {
					arg *= 10;
					arg += *c - '0';
					++c;
				}
				--c;
				// bounds modify arg
				if (arg < 0) arg = 0;
				if (arg >= MAX_COMMAND_ARGS) arg = MAX_COMMAND_ARGS - 1;
				// text?
			} else if (isalpha(*c)) {
				// reset
				std::ostringstream buf;
				do {
					buf << *c;
				} while (isalpha(*++c));
				--c;
				words.resize(1);
				words.back() = buf.str();
				type = CommandFormatNode::LITERAL;
			}

			// check we're not at eof - happens in word/index/list/etc.
			if (*c == '\0')
				break;
		}

		// handle the type
		switch (type) {
		case CommandFormatNode::NONE:
			// sanity check - have a type?
			Log::Error << "Command format '" << format << "' has no type for chunk " << nodes.size() + 1;
			return -1;
		case CommandFormatNode::LITERAL:
			nodes.push_back(CommandFormatNode(arg, opt, words.back()));
			break;
		default:
			nodes.push_back(CommandFormatNode(type, arg, opt));
			break;
		}
	}

	return 0;
}

// match command nodes
int CommandFormat::trymatch(int node, char** words, std::string argv[]) const
{
	int result;
	int cnt;

	// hit end of nodes?
	if (node >= (int)nodes.size()) {
		// words left?  fail */
		if (words[0])
			return 0;
		else
			return -1;
	}

	// we're out of input words -- continue if arg is optional, otherwise
	// we must now fail
	if (words[0] == NULL) {
		if (nodes[node].opt)
			return trymatch(node + 1, words, argv);
		else
			return 0;
	}

	// do matching
	switch (nodes[node].type) {
	case CommandFormatNode::WORD:
		// store the word
		if (nodes[node].arg >= 0)
			argv[nodes[node].arg] = std::string(words[0]);

		// try the next node, return on success
		result = trymatch(node + 1, &words[1], argv);
		if (result == -1)
			return -1;

		// clear out stored argument
		if (nodes[node].arg >= 0)
			argv[nodes[node].arg].clear();

		// if we're optional, then try the next node in the chain
		// otherwise, fail with match count
		if (nodes[node].opt)
			return trymatch(node + 1, words, argv);
		else
			return 1 + result;

	case CommandFormatNode::PHRASE:
		// if we're optional, we need 0 words, otherwise we need 1 word
		cnt = nodes[node].opt ? 0 : 1;

		// match the remainder of the nodes.  if the match fails,
		// then we move the input word index forward by one, and try
		// again, until we either get a match or run out of input
		// words to try
		while ((result = trymatch(node + 1, &words[cnt], argv)) >= 0) {
			// was this the last word?  we fail!
			if (words[cnt] == NULL)
				return cnt + result;
			++cnt;
		}

		// store phrase
		if (nodes[node].arg >= 0)
			argv[nodes[node].arg] = std::string(repair(words[0], cnt));

		// we have successfully matched all following nodes already,
		// so we can just return the success code
		return -1;

	case CommandFormatNode::LITERAL:
		// check for a match
		if (!phraseMatch(nodes[node].literal, words[0]))
			return 0;

		// store the full literal in the argument list
		if (nodes[node].arg >= 0)
			argv[nodes[node].arg] = nodes[node].literal;

		// try the next node, return on success
		result = trymatch(node + 1, &words[1], argv);
		if (result == -1)
			return -1;

		// clear the stored argument
		if (nodes[node].arg >= 0)
			argv[nodes[node].arg].clear();

		// if we're optional, then try the next node in the chain
		// otherwise, fail with match count
		if (nodes[node].opt)
			return trymatch(node + 1, words, argv);
		else
			return 1 + result;

	default:
		// auto-fail
		return 0;
	}
}

bool CommandFormat::operator< (const CommandFormat& format) const
{
	// priorities rule all...
	if (priority < format.priority)
		return true;
	else if (priority > format.priority)
		return false;

	// iterate over all our nodes, comparing with format
	for (uint i = 0; i != nodes.size() && i != format.nodes.size(); ++i) {
		// literal trumps variables
		if (nodes[i].type == CommandFormatNode::LITERAL) {
			if (format.nodes[i].type != CommandFormatNode::LITERAL)
				return true;
			else if (!strEq(nodes[i].literal, format.nodes[i].literal))
				// normal string comparison
				return nodes[i].literal < format.nodes[i].literal;
			// single word trumps many words
		} else if (nodes[i].type == CommandFormatNode::WORD) {
			if (format.nodes[i].type == CommandFormatNode::PHRASE)
				return true;
			// no-op
			else if (format.nodes[i].type != CommandFormatNode::WORD)
				return false;
			// if we're not both lists, format wins
		} else {
			if (format.nodes[i].type != CommandFormatNode::PHRASE)
				return false;
		}
	}

	// matched so far; either we're equal, or one of us has
	// extra nodes that need to be considered.  this is a bit
	// tricky.  compare these two command formats:
	//   verb *? at *   <-- want to try this one first
	//   verb *?
	// with these two command formats:
	//   verb %? at? *
	//   verb %         <-- want to try this one first
	// so we can't just look at the length of the command
	// formats.  this is because * will supercede all remaining
	// nodes, while the others will not.  so... if the last
	// node in both formats is a *, we want to give priority
	// to whichever has extra nodes; otherwise, we give priority
	// to whichever does NOT have extra nodes
	if (nodes.size() > format.nodes.size()) {
		return format.nodes.back().type == CommandFormatNode::PHRASE;
	} else if (format.nodes.size() > nodes.size()) {
		return nodes.back().type != CommandFormatNode::PHRASE;
	}

	// identical formats (which we should hope never happens)
	return false;
}

bool Command::operator< (const Command& command) const
{
	// compare command names
	return strcasecmp(name.c_str(), command.name.c_str()) < 0;
}

// show a man page; return false if cmd_name is not found
bool _MCommand::showMan(StreamControl& stream, const std::string& name, bool quiet)
{
	// find exact match?
	for (CommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		if ((*i)->name == name) {
			(*i)->showMan(stream);
			return true;
		}
	}

	// show possible matches
	bool multiple = false;
	Command* match = NULL;
	for (CommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		if (phraseMatch((*i)->name, name)) {
			if (!multiple && match == NULL) {
				match = *i;
			} else if (!multiple) {
				if (!quiet) {
					stream << CSPECIAL "Search:" CNORMAL;
					stream << StreamIndent(8);
					stream << match->name << "\n";
					stream << (*i)->name << "\n";
				}
				multiple = true;
				match = NULL;
			} else if (!quiet) {
				stream << (*i)->name << "\n";
			}
		}
	}
	if (!quiet)
		stream << StreamIndent(0);

	if (match) {
		match->showMan(stream);
	} else if (!multiple && !quiet) {
		stream << "No manual or matching commands found for '" << name << "'.\n";
	}

	return match;
}

void Creature::processCommand(const std::string& line)
{
	MCommand.call(this, line);
}

// Helper functions for the Creature::clFind_* functions
namespace
{
	// skip a word and following white space
	void skipWord(const char*& ptr)
	{
		// skip the word
		while (*ptr != 0 && !isspace(*ptr))
			++ptr;

		// skip the whitespace
		while (*ptr != 0 && isspace(*ptr))
			++ptr;
	}

	// return true if the string matches 'test' followed by a space or NUL
	bool wordMatch(const char* string, const char* test)
	{
		// if there's no match, it's false
		if (strncasecmp(string, test, strlen(test)))
			return false;

		// byte following the match must be a space or NULL
		return isspace(string[strlen(test)]) || string[strlen(test)] == 0;
	}

	// return true if a string has no more creatures
	inline bool strEmpty(const char* string)
	{
		return string[0] == 0;
	}

	// return a numeric value of the next word in the input
	uint strValue(const char* string)
	{
		// first, check for simple number
		char* end;
		int value = strtoul(string, &end, 10);
		if (end != NULL) {
			// just a number:
			if (*end == 0 || isspace(*end))
				return value;

			// find out if it's a number followed by st, nd, rd, or th
			if (value % 10 == 1 && value != 11 && wordMatch(end, "st"))
				return value;
			else if (value % 10 == 2 && value != 12 && wordMatch(end, "nd"))
				return value;
			else if (value % 10 == 3 && value != 13 && wordMatch(end, "rd"))
				return value;
			else if ((value % 10 > 3 || value % 10 == 0 || (value >= 10 && value <= 13)) && wordMatch(end, "th"))
				return value;
		}

		// try the first, second (other), third, fourth, etc.
		if (prefixMatch("other", string))
			return 2;
		else if (prefixMatch("first", string))
			return 1;
		else if (prefixMatch("second", string))
			return 2;
		else if (prefixMatch("third", string))
			return 3;
		else if (prefixMatch("fourth", string))
			return 4;
		else if (prefixMatch("fifth", string))
			return 5;

		// nobody's going to go higher than that with words, and if they do, fuck 'em
		return 0;
	}
}

// parse a command line to get an object
Object*
Creature::clFindObject(const std::string& line, int type, bool silent)
{
	uint matches;
	Object* object;
	const char* text = line.c_str();

	// check for arg
	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// do we have a my keyword?
	if (wordMatch(text, "my")) {
		type &= ~GOC_FLOOR; // clear floor flag
		skipWord(text);
	} else if (wordMatch(text, "the")) {
		skipWord(text);
	}

	// get index
	uint index = strValue(text);
	if (index)
		skipWord(text);
	else
		index = 1;

	// check for arg
	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// store name
	std::string name(text);

	// search room
	if (type & GOC_FLOOR && getRoom() != NULL) {
		object = getRoom()->findObject(name, index, &matches);
		if (object)
			return object;
		if (matches >= index) {
			*this << "You do not see '" << name << "'.\n";
			return NULL;
		}
		index -= matches; // update count; subtract matches
	}

	// do a sub search for on containers in the room, like tables
	if (type & GOC_SUB && getRoom() != NULL) {
		for (EList<Object>::iterator iter = getRoom()->objects.begin(); iter != getRoom()->objects.end(); ++iter) {
			// have an ON container - search inside
			if (*iter != NULL && (*iter)->hasLocation(ObjectLocation::ON)) {
				if ((object = (*iter)->findObject(name, index, ObjectLocation::ON, &matches))) {
					return object;
				}
				if (matches >= index) {
					*this << "You do not see '" << name << "'.\n";
					return NULL;
				}
				index -= matches; // update count; subtract matches
			}
		}
	}

	// FIXME: these aren't handling the matches/index stuff:

	// try held
	if (type & GOC_HELD)
		if ((object = findHeld(name)) != NULL)
			return object;

	// try worn
	if (type & GOC_WORN)
		if ((object = findWorn(name)) != NULL)
			return object;

	// print error
	if (!silent) {
		if (type & GOC_ROOM)
			*this << "You do not see '" << name << "'.\n";
		else if ((type & GOC_HELD) && !(type & GOC_WORN))
			*this << "You are not holding '" << name << "'.\n";
		else if ((type & GOC_WORN) && !(type & GOC_HELD))
			*this << "You are not wearing '" << name << "'.\n";
		else if ((type & GOC_HELD) && (type & GOC_WORN))
			*this << "You don't have '" << name << "'.\n";
	}
	return NULL;
}

// find an object inside a particular container
Object*
Creature::clFindObject(const std::string& line, Object* container, ObjectLocation type, bool silent)
{
	const char* text = line.c_str();

	// check for arg
	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// ignore both 'my' and 'the'
	if (wordMatch(text, "my") || wordMatch(text, "the"))
		skipWord(text);

	// get index
	uint index = strValue(text);
	if (index)
		skipWord(text);
	else
		index = 1;

	// check for arg
	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// store name
	std::string name(text);

	// search room
	Object* object = container->findObject(name, index, type);
	if (object)
		return object;

	// type name
	*this << "You do not see '" << name << "' " << type.name() << " " << StreamName(container, DEFINITE) << ".\n";
	return NULL;
}


// parse a command line to get a creature
Creature*
Creature::clFindCreature(const std::string& line, bool silent)
{
	const char* text = line.c_str();

	// check for arg
	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	if (getRoom() == NULL) {
		if (!silent)
			*this << "You are not in a room.\n";
		return NULL;
	}

	if (wordMatch(text, "the"))
		skipWord(text);

	uint index = strValue(text);
	if (index)
		skipWord(text);
	else
		index = 1;

	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	Creature* ch = getRoom()->findCreature(text, index);
	if (ch == NULL && !silent)
		*this << "You do not see '" << text << "'.\n";

	return ch;
}

/* parse a command line to get an portal*/
Portal* Creature::clFindPortal(const std::string& line, bool silent)
{
	const char* text = line.c_str();

	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	if (getRoom() == NULL) {
		if (!silent)
			*this << "You are not in a room.\n";
		return NULL;
	}

	if (wordMatch(text, "the"))
		skipWord(text);

	uint index = strValue(text);
	if (index)
		skipWord(text);
	else
		index = 1;

	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	std::string name(text);
	Portal* portal;
	do {
		portal = getRoom()->findPortal(name, index++);
	} while (portal != NULL && portal->isDisabled());

	if (portal == NULL && !silent) {
		*this << "You do not see '" << text << "'.\n";
	}

	return portal;
}

// parse a command line to get a creature, object, or portal
Entity* Creature::clFindAny(const std::string& line, bool silent)
{
	uint matches;

	const char* text = line.c_str();

	// check for arg
	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// do we have a my keyword?
	if (wordMatch(text, "my")) {
		// do 'my' object search
		skipWord(text);
		return clFindObject(text, GOC_EQUIP, silent);
	} else if (wordMatch(text, "the")) {
		skipWord(text);
	}

	// determine our index
	uint index = strValue(text);
	if (index)
		skipWord(text);
	else
		index = 1;

	// check for arg
	if (strEmpty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	std::string name(text);

	// look for a creature
	if (getRoom()) {
		Creature* ch = getRoom()->findCreature(name, index, &matches);
		if (ch != NULL)
			return ch;
		if (matches >= index) {
			if (!silent)
				*this << "You do not see '" << name << "'.\n";
			return NULL;
		}
		index -= matches;
	}

	// search room for object
	if (getRoom() != NULL) {
		Object* obj = getRoom()->findObject(name, index, &matches);
		if (obj)
			return obj;
		if (matches >= index) {
			if (!silent)
				*this << "You do not see '" << name << "'.\n";
			return NULL;
		}
		index -= matches; // update count; subtract matches

		// do a sub search for on containers, like tables
		for (EList<Object>::iterator iter = getRoom()->objects.begin(); index > 0 && iter != getRoom()->objects.end(); ++iter) {
			if (*iter != NULL) {
				// have an ON container - search inside
				if ((*iter)->hasLocation(ObjectLocation::ON)) {
					if ((obj = (*iter)->findObject(name, index, ObjectLocation::ON, &matches))) {
						return obj;
					}
					if (matches >= index) {
						*this << "You do not see '" << name << "'.\n";
						return NULL;
					}
					index -= matches; // update count; subtract matches
				}
			}
		}
	}

	// try held
	for (uint i = 0;; ++i) {
		Object* obj = getHeldAt(i);
		// end of list?
		if (!obj)
			break;
		// matched and index was 1?
		if (obj->nameMatch(name)) {
			if (--index == 0)
				return obj;
		}
	}

	// try worn
	for (uint i = 0;; ++i) {
		Object* obj = getWornAt(i);
		// end of list?
		if (!obj)
			break;
		// matched and index was 1?
		if (obj->nameMatch(name)) {
			if (--index == 0)
				return obj;
		}
	}

	// try an portal
	if (getRoom()) {
		Portal* portal = NULL;
		do {
			portal = getRoom()->findPortal(name, index++);
		} while (portal != NULL && portal->isDisabled());

		if (portal != NULL)
			return portal;
	}

	// print error
	if (!silent)
		*this << "You do not see '" << name << "'.\n";
	return NULL;
}

void Player::processCommand(const std::string& in_line)
{
	if (strEq(in_line, "quit")) {
		endSession();
		return;
	}

	if (in_line.empty())
		return;

	std::string line;
	if (in_line == "#")
		line = last_command;
	else
		line = last_command = in_line;

	/* check for talking with \", a special case */
	if (line[0] == '\"' || line[0] == '\'') {
		if (line[1] == '\0')
			*this << "Say what?\n";
		else
			doSay(std::string(line.c_str() + 1)); // FIXME: make this more efficient
		return;
	}

	/* check for emote'ing with :, a special case */
	if (line[0] == ':' || line[0] == ';') {
		if (line[1] == '\0')
			*this << "Do what?\n";
		else
			doEmote(std::string(line.c_str() + 1)); // FIXME: make this more efficient
		return;
	}

	MCommand.call(this, line);
}

void _MCommand::shutdown()
{
	for (std::vector<Command*>::iterator i = commands.begin(),
	        e = commands.end(); i != e; ++i)
		delete *i;
	commands.clear();
}
