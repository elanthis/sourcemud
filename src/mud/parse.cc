/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <vector>

#include <ctype.h>

#include "mud/entity.h"
#include "mud/parse.h"
#include "mud/char.h"
#include "common/streams.h"
#include "mud/gametime.h"
#include "common/strbuf.h"

#define PARSE_OUT_SIZE 2048
#define PARSE_BUFFER_SIZE 4096
#define MAX_COMM_NAME 32
#define MAX_COMM_DATA 128
#define PARSE_MAX_DEPTH 8

#define CHUNK_LEN(start, end) ((end)-(start)+1)

// parse value
namespace {
	// if statements
	enum if_state {
		IF_DONE = 0,
		IF_TRUE,
		IF_FALSE,
		IF_FAKE
	};
	enum if_type {
		IF_NONE = 0,
		IF_EQUAL,
		IF_NOTEQUAL,
		IF_MATCH,
		IF_LESSTHAN,
		IF_LESSTHANOREQUAL,
		IF_GREATERTHAN,
		IF_GREATERTHANOREQUAL
	};
	enum token_type {
		TOK_ERROR,
		TOK_NAME,
		TOK_STRING,
		TOK_BEGIN,
		TOK_END,
		TOK_IF,
		TOK_ELIF,
		TOK_ELSE,
		TOK_ENDIF,
		TOK_BANG,
		TOK_VAR,
		TOK_METHOD
	};

	typedef std::vector<char> IfStack;

	struct ParseState {
		IfStack if_stack;
		const ParseArgs& argv;
		const ParseNames& names;
		int disable;

		inline ParseState (const ParseArgs& s_argv, const ParseNames& s_names) : if_stack(), argv(s_argv), names(s_names), disable(0) {}
	};

	token_type get_token (const char** in, const char* end, StringBuffer& namebuf);
	void skip (const char** in, const char* end);
	String get_arg (ParseArgs& argv, uint index);
	int invoke_method (const StreamControl& stream, ParseValue self, StringArg method, ParseArgs& argv);
	ParseValue get_value (const char* name, ParseState& state);
	int do_command (const char** in, const char* end, ParseState& state, const StreamControl& stream, int depth, bool if_allowed);
	int do_text(const StreamControl& stream, StringArg in, ParseState& state, int depth);
}

// externally defined in parse_commands.cc, generated from gen/parse-intr.xml
namespace parse {
	extern int exec_command (const StreamControl& stream, StringArg command, ParseArgs& argv);
}

// function definitions
namespace {
	// get a token
	// NOTE: leaves *in pointing at character following last character of token
	token_type get_token (const char** in, const char* end, StringBuffer& namebuf)
	{
		// clear buffer
		namebuf.reset();

		// skip whitespace
		while (*in != end && isspace(**in))
			++*in;

		// no more tokens - error, expect }
		if (*in == end) {
			return TOK_ERROR;
		}

		// ! is a bang
		else if (**in == '!') {
			++*in;
			return TOK_BANG;
		// { is a begin
		} else if (**in == '{') {
			++*in;
			return TOK_BEGIN;
		// } is an end
		} else if (**in == '}') {
			++*in;
			return TOK_END;
		// $ is a variable
		} else if (**in == '$') {
			++*in;
			return TOK_VAR;
		// . is method syntactic sugar
		} else if (**in == '.') {
			++*in;
			return TOK_METHOD;

		// alphanumeric is a name
		} else if (isalnum(**in)) {
			// find end of name
			const char* sin = *in;
			do {
				++*in;
			} while (*in != end && (isalnum(**in) || **in == '-'));
			// if, elif, else, and endif are special
			if (!strncasecmp(sin, S("if"), *in - sin))
				return TOK_IF;
			else if (!strncasecmp(sin, S("elif"), *in - sin))
				return TOK_ELIF;
			else if (!strncasecmp(sin, S("else"), *in - sin))
				return TOK_ELSE;
			else if (!strncasecmp(sin, S("endif"), *in - sin))
				return TOK_ENDIF;
			// save name
			namebuf << StreamChunk(sin, *in - sin);
			return TOK_NAME;

		// ' or " is a string
		} else if (**in == '"' || **in == '\'') {
			// find end of string
			const char* sin = *in;
			const char q = **in;
			do {
				++*in;
			} while (*in != end && **in != q);
			// save string
			namebuf << StreamChunk(sin + 1, *in - sin - 1);
			++*in; // skip past end quote character
			return TOK_STRING;
		}

		// unknown
		return TOK_ERROR;
	}

	// skip to end of processing
	void skip (const char** in, const char* end)
	{
		int nest = 0;
		char quote = 0;

		// iterate
		while (*in != end) {
			// in quote?
			if (quote) {
				// de-quote?
				if (**in == quote)
					quote = 0;
			// not in quote
			} else {
				// nest on {
				if (**in == '{') {
					++nest;
				// ignore \\ sequence
				} else if (**in == '\\') {
					++in;
					if (*in == end)
						break;
				// handle quote
				} else if (**in == '"' || **in == '\'') {
					quote = **in;
				// un-nest
				} else if (**in == '}') {
					if (nest == 0) {
						++*in;
						break;
					}
					--nest;
				}
			}
			++*in;
		}
	}

	// get an argument as a string
	String get_arg (ParseArgs& argv, uint index)
	{
		// bounds check
		if (index >= argv.size())
			return String();

		// return string (empty if type is not a string)
		return argv[index].get_string();
	}

	// invoke a method
	int invoke_method (const StreamControl& stream, ParseValue self, StringArg method, ParseArgs& argv)
	{
		// if it's an entity, invoke Entity::parse_property();
		if (self.is_entity()) {
			return self.get_entity()->parse_property(stream, method, argv);
		}
		
		// it's a string, so process it ourself
		if (self.is_string()) {
			String string = self.get_string();

			// LENGTH
			if (!strcasecmp(method, "length")) {
				stream << string.size();
				return 0;
			}
		}

		return -1;
	}

	// return a value from the argv
	ParseValue get_value (const char* name, ParseState& state)
	{
		char* end;
		unsigned index;

		// if it's an int, do an index lookup
		index = strtol(name, &end, 10);
		if (*end == 0) {
			--index;
			if (index < 0 || index >= state.argv.size())
				return ParseValue();
			return state.argv[index];
		}

		// search by name
		for (uint i = 0; i < state.names.size(); ++i) {
			if (!strcasecmp(state.names[i], name))
				return state.argv[i];
		}

		// nothing found
		return ParseValue();
	}

	// handle a command
	int do_command (const char** in, const char* end, ParseState& state, const StreamControl& stream, int depth, bool if_allowed)
	{
		const char* sin;
		token_type token;
		bool is_if = false;
		bool is_bang = false;
		ParseValue value;
		String method;
		StringBuffer buffer;
		ParseArgs argv;

		// save start, advance
		sin = *in - 1;

		// max depth
		if (depth > PARSE_MAX_DEPTH) {
			skip(in, end);
			stream << StreamChunk(sin, *in - sin);
			return -1;
		}

		// grab a token
		if ((token = get_token(in, end, buffer)) == TOK_ERROR) {
			skip(in, end);
			stream << StreamChunk(sin, *in - sin);
			return -1;
		}

		// if processing, depth one only
		if (if_allowed) {
			// begin new true block
			if (token == TOK_IF) {
				state.if_stack.push_back(IF_FAKE);
				is_if = true;

				// if disabled, go no further
				if (state.disable)
					return 0;
			// execute another if
			} else if (token == TOK_ELIF) {
				// check we're not empty
				if (state.if_stack.empty()) {
					skip(in, end);
					stream << StreamChunk(sin, *in - sin);
					return -1;

				// dump if we're in a fake if
				} else if (state.if_stack.back() == IF_FAKE) {
					skip(in, end);
					stream << StreamChunk(sin, *in - sin);
					return 0;

				// already had true state - all done
				} else if (state.if_stack.back() == IF_TRUE || state.if_stack.back() == IF_DONE) {
					state.if_stack.back() = IF_DONE;
					skip(in, end);
					++state.disable;
					return 0;

				// we were false, so now we _may_ be true...
				} else {
					is_if = true;
				}
			// alternate to true state
			} else if (token == TOK_ELSE) {
				// chek we're not empty
				if (state.if_stack.empty()) {
					skip(in, end);
					stream << StreamChunk(sin, *in - sin);
					return -1;

				// dump if we're in a fake if
				} else if (state.if_stack.back() == IF_FAKE) {
					skip(in, end);
					stream << StreamChunk(sin, *in - sin);
					return 0;

				// already had true state - all done
				} else if (state.if_stack.back() == IF_TRUE) {
					state.if_stack.back() = IF_DONE;
					++state.disable;

				// we were false, so now we are true
				} else if (state.if_stack.back() == IF_FALSE) {
					state.if_stack.back() = IF_TRUE;
					--state.disable;
				}

				skip(in, end);
				return 0;
			// end if block
			} else if (token == TOK_ENDIF) {
				// chek we're not empty
				if (state.if_stack.empty()) {
					skip(in, end);
					stream << StreamChunk(sin, *in - sin);
					return -1;

				// dump if we're in a fake if
				} else if (!state.disable && state.if_stack.back() == IF_FAKE) {
					skip(in, end);
					stream << StreamChunk(sin, *in - sin);

				// just end the if block
				} else {
					skip(in, end);
				}

				if (state.if_stack.back() == IF_DONE || state.if_stack.back() == IF_FALSE)
					--state.disable;
				state.if_stack.pop_back();
				return 0;
			}

			// if it was an if, grab another token
			if (is_if) {
				if ((token = get_token(in, end, buffer)) == TOK_ERROR) {
					skip(in, end);
					stream << StreamChunk(sin, *in - sin);
					return -1;
				}
			}
		}

		// if we're disable and not in a check, exit now
		if (state.disable) {
			skip(in, end);
			return 0;
		}

		// if a bang, mark and grab another token
		if (token == TOK_BANG) {
			is_bang = true;

			if ((token = get_token(in, end, buffer)) == TOK_ERROR) {
				skip(in, end);
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}
		}

		// if a variable, get name, then another token
		if (token == TOK_VAR) {
			// expect a name token
			if ((token = get_token(in, end, buffer)) != TOK_NAME) {
				skip(in, end);
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}

			// get value of variable, error if no such variable
			value = get_value(buffer.str(), state);
			if (value.is_null()) {
				skip(in, end);
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}

			// next token
			if ((token = get_token(in, end, buffer)) == TOK_ERROR) {
				skip(in, end);
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}

			// eat method token, syntactic sugar
			if (token == TOK_METHOD) {
				// next token
				if ((token = get_token(in, end, buffer)) == TOK_ERROR) {
					skip(in, end);
					stream << StreamChunk(sin, *in - sin);
					return -1;
				}
			}
		}

		// if we have a name, then it's a method
		if (token == TOK_NAME) {
			method = buffer.str();

			// next token
			if ((token = get_token(in, end, buffer)) == TOK_ERROR) {
				skip(in, end);
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}

		// if we have no method, then we can have no arguments
		} else if (token != TOK_END) {
			skip(in, end);
			stream << StreamChunk(sin, *in - sin);
			return -1;

		// no method, so we better have a value
		} else if (value.is_null()) {
			skip(in, end);
			stream << StreamChunk(sin, *in - sin);
			return -1;
		}

		// keep pushing arguments
		while (token != TOK_END) {
			// string
			if (token == TOK_STRING) {
				argv.push_back(buffer.str());
			// sub-command
			} else if (token == TOK_BEGIN) {
				if (do_command(in, end, state, buffer, depth + 1, false)) {
					stream << StreamChunk(sin, *in - sin);
					return -1;
				}
				argv.push_back(buffer.str());
			// something else
			} else {
				skip(in, end);
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}
			// another token
			if ((token = get_token(in, end, buffer)) == TOK_ERROR) {
				skip(in, end);
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}
		}

		// have a variable
		if (!value.is_null()) {
			// a method?
			if (method) {
				if (invoke_method(buffer, value, method, argv)) {
					stream << StreamChunk(sin, *in - sin);
					return -1;
				}
			// just a value
			} else {
				if (value.is_string())
					buffer << value.get_string();
				else if (value.is_entity())
					buffer << StreamName(value.get_entity());
			}
		// just a function?
		} else {
			if (parse::exec_command(buffer, method, argv)) {
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}
		}

		// upper-case output if method name was upper-cased
		if (isupper(method[0])) {
			buffer[0] = toupper(buffer[0]);
		}

		// execute if statement
		if (is_if) {
			state.if_stack.back() = buffer[0] != 0 ? IF_TRUE : IF_FALSE;
			if (state.if_stack.back() == IF_FALSE)
				++state.disable;
		// or expand bang
		} else if (is_bang) {
			ParseState bstate(state.argv, state.names);
			StringBuffer bbuffer;
			if (do_text(bbuffer, buffer.str(), bstate, depth + 1)) {
				stream << StreamChunk(sin, *in - sin);
				return -1;
			}
			stream << bbuffer;
		// or output text
		} else {
			stream << buffer;
		}

		// done
		return 0;
	}

	// parse text
	int
	do_text(const StreamControl& stream, StringArg in, ParseState& state, int depth)
	{
		// declarations
		const char* c;

		// iterate over input
		for (c = in.c_str(); *c != 0; ++c) {
			// backslash escape
			if (!state.disable && *c == '\\') {
				++c;
				if (*c == 0) // trailing backslash
					return 0;

				// another backslash
				if (*c == '\\')
					stream << '\\';
				// newline
				else if (*c == 'n')
					stream << '\n';
				// begin-parse
				else if (*c == '{')
					stream << '{';
				// anything else is unsupported, ignore

			// begin parse command
			} else if (*c == '{') {
				++c; // skip {
				do_command(&c, in.c_str() + in.size(), state, stream, depth, true);
				--c; // go back to }
			// just text
			} else if (!state.disable) {
				stream << *c;
			}
		}

		return 0;
	}
}

// parsing
namespace parse {
	// parse text
	const StreamControl&
	text(const StreamControl& stream, StringArg in, const ParseArgs& argv, const ParseNames& names)
	{
		ParseState state(argv, names);

		if (do_text(stream, in, state, 1))
			stream << in;

		return stream;
	}
}
