/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <ctype.h>

#include <vector>

#include "common/error.h"
#include "mud/command.h"
#include "mud/char.h"
#include "mud/player.h"
#include "mud/social.h"
#include "common/streams.h"
#include "mud/color.h"
#include "mud/account.h"
#include "mud/parse.h"
#include "mud/help.h"
#include "scriptix/array.h"

SCommandManager CommandManager;

// helper functions
namespace {
	char*
	repair (char* word, int cnt)
	{
		while (--cnt > 0)
			word[strlen(word)] = ' ';
		return word;
	}
	char*
	repair (char** words)
	{
		int cnt = 0;
		while (words[cnt] != NULL)
			++cnt;
		return repair(words[0], cnt);
	}
}

namespace commands {
	// get a single argument
	char *
	get_arg (char **c) {
		char *arg;

		assert (c != NULL);

		// trim whitespace
		while (isspace (**c))
			++ (*c);

		// exit if blank
		if (**c == '\0') {
			return NULL;
		}

		arg = *c; // Mark start
		// search to next whitespace
		while (**c != '\0' && !isspace (**c))
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
	is_arg (char *comm) {
		if (comm == NULL)
			return false;

		// look for a non-whitespace char
		while (*comm != '\0') {
			if (!isspace (*comm))
				return true;
			++ comm;
		}
			
		return false;
	}

	// add an arg back into line
	char *
	fix_arg (char *arg, char **c) {
		int len = strlen (arg);
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
	restore (char *start, char **c) {
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
namespace {
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
int
SCommandManager::add (Command *command)
{
	assert (command != NULL);

	// add the command
	CommandList::iterator ci = std::lower_bound(commands.begin(), commands.end(), command, DerefSort<Command>());
	commands.insert(ci, command);

	// add its formats
	for (Command::FormatList::iterator i = command->formats.begin(); i != command->formats.end(); ++i) {
		FormatList::iterator fi = std::lower_bound(formats.begin(), formats.end(), *i, DerefSort<CommandFormat>());
		formats.insert(fi, *i);
	}

	return 0;
}

int
SCommandManager::call (Character *ch, StringArg comm) {
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
	String argv[MAX_COMMAND_ARGS];
	int best_score = -1, result = 0;
	CommandList best_cmds;
	for (FormatList::iterator fi = formats.begin(); fi != formats.end(); ++fi) {
		CommandFormat* format = *fi;
		Command* command = format->get_command();
		// must check privileges
		if (!command->access.valid() || (ply != NULL && ply->get_account()->has_access(command->access))) {
			// do match
			result = format->match(words, argv);
			if (result == -1) {
				// all good - call function
				if (format->ch_func) {
					// call char functions
					format->ch_func (ch, argv);
					return 0;
				} else if (format->ply_func && ply) {
					// call player function
					format->ply_func(ply, argv);
					return 0;
				} else if (!format->script.empty()) {
					// call Scriptix function
					Scriptix::Array* arg_list = new Scriptix::Array(10, NULL);
					for (int i = 0; i < MAX_COMMAND_ARGS; ++i)
						Scriptix::Array::append(arg_list, argv[i] ? Scriptix::Value(argv[i]) : Scriptix::Value(Scriptix::Nil));
					return format->script.run(ch, arg_list).to_int();
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
			player->set_indent(2);
			for (CommandList::iterator i = best_cmds.begin(); i != best_cmds.end(); ++i)
				*player << (*i)->usage;
			player->set_indent(0);
			return 1;
		}
	}

	// not found - try socials database
	const Social* social = SocialManager.find_social(String(words[0])); // FIXME: efficiency
	if (social != NULL) {
		// parse
		const SocialAdverb* sa = NULL;
		Entity* target = NULL;
		if (words[1]) {
			if ((sa = social->get_adverb(String(words[1]))) != NULL) { // FIXME: efficiency
				// find target
				if (words[2]) {
					target = ch->cl_find_any (String(repair(&words[2])), false); // FIXME: efficiency
					if (!target)
						return 1;
				}
			}
		}
		
		// find de default
		if (sa == NULL) {
			sa = social->get_default();
			// find target
			if (words[1]) {
				target = ch->cl_find_any (String(repair(&words[1])), false); // FIXME: efficiency
				if (!target)
					return 1;
			}
		}

		// all ready?
		if (sa) {
			ch->do_social(sa, target);
			return 0;
		}
	}

	// absolutely no command
	*ch << "I do not understand.\n";
	return 1;
}

void
SCommandManager::show_list (Player *player)
{
	int col, max_col;
	size_t colwidth;

	// max width
	colwidth = 1;
	for(CommandList::iterator i = commands.begin(); i != commands.end(); ++i)
		if ((*i)->name.size() >= colwidth)
			colwidth = (*i)->name.size() + 1;

	// calculate column size
	max_col = (player->get_width() - 3) / colwidth; // figure max size
	if (max_col <= 0)
		max_col = 3;

	// initialize loop meta info
	col = 0;
	*player << CSPECIAL "Command List:\n" CNORMAL;
	player->set_indent(2);

	// loop over all commands
	for(CommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		// skip empty commands
		if ((*i)->name.empty())
			continue;

		// must check privileges
		if ((*i)->access.valid() && (player == NULL || !player->get_account()->has_access((*i)->access)))
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
		player->set_indent(col * colwidth + 2);
	}

	// finish off
	if (col != 0)
		*player << "\n";
	player->set_indent(0);
}

void	
Command::show_man (Player* player)
{
	HelpTopic* topic = HelpManager.get_topic(name);

	*player << CSPECIAL "Help: " CNORMAL << name << "\n\n";

	if (topic != NULL) {
		player->set_indent(2);
		*player << CSPECIAL "About:" CNORMAL "\n";
		player->set_indent(4);
		*player << StreamParse(topic->about, S("player"), player) << S("\n");
	}
	player->set_indent(2);
	*player << CSPECIAL "Usage:" CNORMAL "\n";
	player->set_indent(4);
	*player << usage << "\n";
	player->set_indent(0);
}

// build command
int
CommandFormat::build (StringArg s_format)
{
	int arg;
	bool opt;
	FormatNode::type_t type;
	StringList words;

	// set our format
	format = s_format;

	// modifiable string
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "%s", format.c_str());

	// loop thru format args, parsing
	char* word;
	char* fmt = buffer;
	while ((word = commands::get_arg(&fmt)) != NULL) {
		// reset
		arg = -1;
		opt = false;
		type = FormatNode::NONE;
		words.clear();

		// parse the argument
		char* c;
		for (c = word; *c != '\0'; ++c) {
			// optional?
			if (*c == '?')
				opt = true;
			// word?
			else if (*c == '%')
				type = FormatNode::ONE;
			// any-string?
			else if (*c == '*')
				type = FormatNode::MANY;
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
			}
			// list?
			else if (*c == '(') {
				// reset
				words.clear();
				// do lookup
				char* sep;
				++c;
				while ((sep = strpbrk(c, ",)")) != NULL) {
					if (sep > c)
						words.push_back(String(c, sep - c));
					if (*sep == ')') {
						c = sep;
						break;
					} else {
						c = sep + 1;
					}
				}
				// push any remaining word
				if (strlen(c))
					words.push_back(String(c));
				// type is words
				type = FormatNode::LIST;
			}
			// text?
			else if (isalpha(*c)) {
				// reset
				StringBuffer buf;
				do {
					buf << *c;
				} while (isalpha(*++c));
				--c;
				words.resize(1);
				words.back() = buf.str();
				type = FormatNode::TEXT;
			}

			// check we're not at eof - happens in word/index/list/etc.
			if (*c == '\0')
				break;
		}

		// handle the type
		switch (type) {
			case FormatNode::NONE:
				// sanity check - have a type?
				Log::Error << "Command format '" << format << "' has no type for chunk " << nodes.size()+1;
				return -1;
			case FormatNode::TEXT:
				nodes.push_back(FormatNode(arg, opt, words.back()));
				break;
			case FormatNode::LIST:
				nodes.push_back(FormatNode(arg, opt, words));
				break;
			default:
				nodes.push_back(FormatNode(type, arg, opt));
				break;
		}
	}

	return 0;
}

// match command nodes
int
CommandFormat::trymatch (int node, char** words, String argv[]) const
{
	int result;
	int cnt;

	// hit eol?
	if (node >= (int)nodes.size()) {
		// words left?  fail */
		if (words[0])
			return 0;
		else
			return -1;
	}

	// do matching
	if (words[0]) {
		switch (nodes[node].type) {
			case FormatNode::NONE:
				// auto-fail
				return 0;
			case FormatNode::ONE:
				// store the word
				if (nodes[node].arg >= 0)
					argv[nodes[node].arg] = String(words[0]);
				// try next match
				result = trymatch(node + 1, &words[1], argv);
				// match failed?
				if (result >= 0) {
					// clear out store
					if (nodes[node].arg >= 0)
						argv[nodes[node].arg].clear();
					// were we optional?
					if (nodes[node].opt) {
						// try without us then
						return trymatch(node + 1, words, argv);
					} else {
						return 1 + result;
					}
				} else {
					return -1;
				}
				break;
			case FormatNode::MANY:
				// if we're optional, we need 0 words, otherwise we need 1 word
				cnt = nodes[node].opt ? 0 : 1;
				// try to find following match
				while ((result = trymatch(node + 1, &words[cnt], argv)) >= 0) {
					// was this the last word?  we fail!
					if (words[cnt] == NULL)
						return 1 + result;
					++cnt;
				}
				// store word
				if (nodes[node].arg >= 0)
					argv[nodes[node].arg] = String(repair(words[0], cnt));
				// not only are we now successful, but so too is everyone after us!
				return -1;
			case FormatNode::TEXT:
				// have we a word that matches
				if (phrase_match(nodes[node].list.front(), String(words[0]))) { // FIXME: efficiency
					// store the word
					if (nodes[node].arg >= 0)
						argv[nodes[node].arg] = nodes[node].list.front();
					// try next match
					result = trymatch(node + 1, &words[1], argv);
					// match failed?
					if (result >= 0) {
						// clear out store
						if (nodes[node].arg >= 0)
							argv[nodes[node].arg].clear();
						// were we optional?
						if (nodes[node].opt) {
							// try without us then
							return trymatch(node + 1, words, argv);
						} else {
							return 1 + result;
						}
					} else {
						return -1;
					}
				}
				break;
			case FormatNode::LIST:
				// search list
				cnt = 0;
				for (StringList::const_iterator i = nodes[node].list.begin(); i != nodes[node].list.end(); ++i) {
					if (phrase_match(*i, String(words[0]))) // FIXME: efficiency
						break;
					++cnt;
				}
				if (cnt < (int)nodes[node].list.size()) {
					// store the word
					if (nodes[node].arg >= 0)
						argv[nodes[node].arg] = nodes[node].list[cnt];
					// try next match
					result = trymatch(node + 1, &words[1], argv);
					// match failed?
					if (result >= 0) {
						// clear out store
						if (nodes[node].arg >= 0)
							argv[nodes[node].arg].clear();
						// were we optional?
						if (nodes[node].opt) {
							// try without us then
							return trymatch(node + 1, words, argv);
						} else {
							return 1 + result;
						}
					} else {
						return -1;
					}
				}
				break;
		}
	}

	// no match, was it optional?
	if (nodes[node].opt) {
		// try next node
		return trymatch(node + 1, words, argv);
	} else {
		return 0;
	}
}

bool
CommandFormat::operator< (const CommandFormat& format) const
{
	// priorities rule all...
	if (priority < format.priority)
		return true;
	else if (priority > format.priority)
		return false;

	// iterate over all our nodes, comparing with format
	for (uint i = 0; i < nodes.size() && i < format.nodes.size(); ++i) {
		// word list trumps all
		if (nodes[i].type == FormatNode::LIST) {
			if (format.nodes[i].type != FormatNode::LIST)
				return true;
		// text trumps variables
		} else if (nodes[i].type == FormatNode::TEXT) {
			if (format.nodes[i].type == FormatNode::LIST)
				return false;
			else if (format.nodes[i].type != FormatNode::TEXT)
				return true;
			else if (!str_eq(nodes[i].list.front(), format.nodes[i].list.front()))
				// normal string comparison
				return nodes[i].list.front() < format.nodes[i].list.front();
		// single word trumps many words
		} else if (nodes[i].type == FormatNode::ONE) {
			if (format.nodes[i].type == FormatNode::MANY)
				return true;
				// no-op
			else if (format.nodes[i].type != FormatNode::ONE)
				return false;
		// if we're not both lists, format wins
		} else {
			if (format.nodes[i].type != FormatNode::MANY)
				return false;
		}
	}

	// matched so far; we're either equal or have extra terms.
	// extra terms comes first
	if (nodes.size() > format.nodes.size())
		return true;

	// equal or he wins, so false
	return false;
}

bool
Command::operator< (const Command& command) const
{
	// compare command names
	return strcasecmp(name.c_str(), command.name.c_str()) < 0;
}

// show a man page; return false if cmd_name is not found
bool
SCommandManager::show_man (Player* player, StringArg name, bool quiet)
{
	// find exact match?
	for (CommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		if ((*i)->name == name) {
			(*i)->show_man (player);
			return true;
		}
	}

	// show possible matches
	bool multiple = false;
	Command* match = NULL;
	for (CommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		if (phrase_match((*i)->name, String(name))) { // FIXME: efficiency
			if (!multiple && match == NULL) {
				match = *i;
			} else if (!multiple) {
				if (!quiet) {
					*player << CSPECIAL "Search:" CNORMAL;
					player->set_indent(8);
					*player << match->name << "\n";
					*player << (*i)->name << "\n";
				}
				multiple = true;
				match = NULL;
			} else if (!quiet) {
				*player << (*i)->name << "\n";
			}
		}
	}
	if (!quiet)
		player->set_indent(0);

	if (match) {
		match->show_man(player);
	} else if (!multiple && !quiet) {
		*player << "No manual or matching commands found for '" << name << "'.\n";
	}

	return match;
}
