/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/streams.h"
#include "common/strbuf.h"
#include "common/string.h"
#include "mud/macro.h"
#include "mud/gametime.h"
#include "mud/player.h"
#include "net/manager.h"
#include "lua/core.h"
#include "lua/print.h"
#include "lib/lua51/lua.h"
#include "lib/lua51/lauxlib.h"

#define MACRO_MAX_DEPTH 8

#define CHUNK_LEN(start, end) ((end)-(start)+1)

class MacroParseException {
	public:
	MacroParseException(const char *_msg) : msg(_msg) {}

	const char* msg;
};

// macro value
class MacroCompiler {
public:
	MacroCompiler(const char *_source, size_t _source_len) : source(_source),
			sptr(source), source_len(_source_len), next_token(TOK_NONE) {}

	// macro text
	bool compile(std::ostream& stream) {
		try {
			parseBody(stream, true);
			return true;
		} catch (MacroParseException& e) {
			stream << "print(\"[[error: " << e.msg << "]]\")\n";
			return false;
		}
	}

private:
	// if statements
	enum token_type {
		TOK_NONE, TOK_ERROR, TOK_NAME, TOK_STRING, TOK_TRUE, TOK_FALSE,
		TOK_NUMBER, TOK_BEGIN, TOK_END, TOK_IF, TOK_ELIF, TOK_ELSE, TOK_ENDIF,
		TOK_EXPAND, TOK_RAISE, TOK_LPAREN, TOK_RPAREN, TOK_COMMA, TOK_AND,
		TOK_OR, TOK_NOT, TOK_VAR, TOK_EQ, TOK_NE, TOK_PROPERTY
	};

	const char *source;
	const char *sptr;
	size_t source_len;

	token_type next_token;
	std::string token_value;

	char getc() { if (sptr == source + source_len) return 0; else return *sptr++; }
	char peekc() const { if (sptr == source + source_len) return 0; else return *sptr; }
	bool eof() const { return sptr == source + source_len; }

	// format a Lua string safely
	void printString(std::ostream& stream, const char *string, size_t len) {
		stream << '\'';
		for (size_t i = 0; i != len; ++i) {
			if (string[i] == '\\')
				stream << "\\\\";
			else if (string[i] == '\'')
				stream << "\\'";
			else if (string[i] == '\n')
				stream << "\\n";
			else if (isprint(string[i]))
				stream << string[i];
			else {
				char buf[5];
				snprintf(buf, sizeof(buf), "\\%03d", (int)string[i]);
				stream << buf;
			}
		}
		stream << '\'';
	}

	// getc a token
	token_type getToken() {
		// skip whitespace
		while (!eof() && isspace(peekc()))
			getc();

		// no more tokens - error, expect }
		if (eof())
			return TOK_ERROR;

		char c = getc();

		// ! is an eval
		if (c == '!') {
			return TOK_EXPAND;
		// ^ is a raise
		} else if (c == '^') {
			return TOK_RAISE;
		// parenthesis
		} else if (c == '(') {
			return TOK_LPAREN;
		} else if (c == ')') {
			return TOK_RPAREN;
		// } is an end
		} else if (c == '}') {
			return TOK_END;
		// , separates function args
		} else if (c == ',') {
			return TOK_COMMA;
		// . is for property lookup
		} else if (c == '.') {
			return TOK_PROPERTY;

		// digits make a number
		} else if (isdigit(c)) {
			std::ostringstream numberbuf;
			numberbuf << c;
			while (!eof() && (isdigit(peekc()))) {
				c = getc();
				numberbuf << c;
			}
			token_value = numberbuf.str();
			return TOK_NUMBER;

		// alphanumeric is a name (or variable if starst with $)
		} else if (isalpha(c) || c == '_' || c == '$') {
			std::ostringstream namebuf;
			namebuf << c;
			while (!eof() && (isalnum(peekc()) || peekc() == '-' || peekc() == '_')) {
				c = getc();
				namebuf << c;
			}
			token_value = namebuf.str();
			if (token_value[0] == '$')
				return TOK_VAR;
			// keywords
			if (strcasecmp(token_value.c_str(), "eq") == 0)
				return TOK_EQ;
			else if (strcasecmp(token_value.c_str(), "ne") == 0)
				return TOK_NE;
			else if (strcasecmp(token_value.c_str(), "and") == 0)
				return TOK_AND;
			else if (strcasecmp(token_value.c_str(), "or") == 0)
				return TOK_OR;
			else if (strcasecmp(token_value.c_str(), "not") == 0)
				return TOK_NOT;
			else if (strcasecmp(token_value.c_str(), "if") == 0)
				return TOK_IF;
			else if (strcasecmp(token_value.c_str(), "elif") == 0)
				return TOK_ELIF;
			else if (strcasecmp(token_value.c_str(), "else") == 0)
				return TOK_ELSE;
			else if (strcasecmp(token_value.c_str(), "endif") == 0)
				return TOK_ENDIF;
			else if (strcasecmp(token_value.c_str(), "true") == 0)
				return TOK_TRUE;
			else if (strcasecmp(token_value.c_str(), "false") == 0)
				return TOK_FALSE;
			return TOK_NAME;

			// ' or " is a string
		} else if (c == '"' || c == '\'') {
			std::ostringstream stringbuf;
			// find end of string
			// FIXME: check for backslashes
			while (!eof() && peekc() != c)
				stringbuf << getc();
			// eat quote
			if (peekc() == c)
				getc();
			token_value = stringbuf.str();
			return TOK_STRING;
		}

		// unknown
		return TOK_ERROR;
	}

	// accept a token
	bool accept(token_type is_token) {
		if (next_token == TOK_NONE)
			next_token = getToken();

		if (next_token == is_token) {
			next_token = TOK_NONE;
			return true;
		} else
			return false;
	}

	// peek at token without removing it
	bool peek(token_type is_token) {
		if (next_token == TOK_NONE)
			next_token = getToken();

		return next_token == is_token;
	}

	// expect a token
	void expect(token_type is_token) {
		if (next_token == TOK_NONE)
			next_token = getToken();

		if (next_token == is_token)
			next_token = TOK_NONE;
		else
			throw MacroParseException("did not get expected token type");
	}

	bool acceptBinaryOp(std::ostream& stream) {
		// == operator
		if (accept(TOK_EQ)) {
			stream << " == ";
			return true;

		// ~= operator
		} else if (accept(TOK_NE)) {
			stream << " ~= ";
			return true;
	
		// and operator
		} else if (accept(TOK_AND)) {
			stream << " and ";
			return true;

		// or operator
		} else if (accept(TOK_OR)) {
			stream << " or ";
			return true;

		// not a binary operator
		} else
			return false;
	}

	bool acceptTerminal(std::ostream& stream) {
		// variable
		if (accept(TOK_VAR)) {
			stream << "macro.lookup('" << token_value << "')";
			return true;

		// boolean literals
		} else if (accept(TOK_TRUE)) {
			stream << "true";
			return true;
		} else if (accept(TOK_FALSE)) {
			stream << "false";
			return true;

		// number literal
		} else if (accept(TOK_NUMBER)) {
			stream << token_value;
			return true;

		// string literal
		} else if (accept(TOK_STRING)) {
			printString(stream, token_value.c_str(), token_value.size());
			return true;

		// unknown token
		} else
			return false;
	}

	void parseArguments(std::ostream& stream) {
		if (accept(TOK_LPAREN)) {
			if (!accept(TOK_RPAREN)) {
				for (;;) {
					stream << ',';
					if (!acceptUnary(stream))
						throw MacroParseException("expected expression in argument list");
					if (!accept(TOK_COMMA)) {
						expect(TOK_RPAREN);
						break;
					}
				}
			}
		}
	}

	bool acceptPostfix(std::ostream& stream) {
		// function (with optional arguments)
		if (accept(TOK_NAME)) {
			stream << "macro.invoke('" << token_value << '\'';
			parseArguments(stream);
			stream << ')';
			return true;

		// variable (with optional property lookup... with optional arguments)
		} else if (accept(TOK_VAR)) {
			std::string var = token_value;
			if (accept(TOK_PROPERTY)) {
				expect(TOK_NAME);
				stream << "macro.property(";
				printString(stream, var.c_str(), var.size());
				stream << ',';
				printString(stream, token_value.c_str(), token_value.size());
				parseArguments(stream);
				stream << ')';
			} else {
				stream << "macro.lookup(";
				printString(stream, var.c_str(), var.size());
				stream << ')';
			}
			return true;

		// get terminal
		} else
			return acceptTerminal(stream);

		return true;
	}

	bool acceptUnary(std::ostream& stream) {
		// NOT operator
		if (accept(TOK_NOT)) {
			stream << "not ";
			if (!acceptUnary(stream))
				throw MacroParseException("expected expression after unary operator");
			return true;

		// RAISE operator
		} else if (accept(TOK_RAISE)) {
			stream << "macro.raise(";
			if (!acceptUnary(stream))
				throw MacroParseException("expected expression after unary operator");
			stream << ')';
			return true;

		// sub-expression
		} else if (accept(TOK_LPAREN)) {
			stream << '(';
			if (!acceptUnary(stream))
				throw MacroParseException("expected expression after unary operator");
			stream << ')';
			return true;

		// just a terminal with no unary operator
		} else
			return acceptPostfix(stream);
	}

	bool acceptExpr(std::ostream& stream) {
		// start with a unary
		if (!acceptUnary(stream))
			return false;

		// so long as we have a binary operator
		while (acceptBinaryOp(stream)) {
			if (!acceptUnary(stream))
				throw MacroParseException("Expected expression after binary operator");
		}

		return true;
	}

	void parseBody(std::ostream& stream, bool top) {
		const char *lptr = sptr;
		// look for {
		while (!eof()) {
			// possible macro code
			if (*sptr == '{') {
				// spit out string so far
				if (sptr != lptr) {
					stream << "print(";
					printString(stream, lptr, sptr - lptr);
					stream << ")\n";
				}

				// look to next location; use lptr for source info
				lptr = ++sptr;

				// hit end of string inside macro
				if (eof())
					throw MacroParseException("unexpected end of string after {");

				// if it's another {, it's an escape; set it as the next printable
				// character and continue
				if (*sptr == '{') {
					lptr = sptr;
					continue;
				}

				// compile macro
				try {
					next_token = TOK_NONE;

					// if statement
					if (accept(TOK_IF)) {
						// parse expression
						stream << "if ";
						if (!acceptExpr(stream))
							throw MacroParseException("expected expression after if");
						expect(TOK_END);
						stream << " then\n";

						// true-case body
						parseBody(stream, false);

						// else-ifs?
						while (accept(TOK_ELIF)) {
							// parse expression
							stream << "elseif ";
							if (!acceptExpr(stream))
								throw MacroParseException("expected expression after elif");
							expect(TOK_END);
							stream << " then\n";

							// true-case body
							parseBody(stream, false);
						}

						// else?
						if (accept(TOK_ELSE)) {
							expect(TOK_END);
							stream << "else\n";
							parseBody(stream, false);
						}

						// require the endif
						expect(TOK_ENDIF);
						stream << "end\n";
						expect(TOK_END);

					// return to parent on else/elseif/endif
					} else if (peek(TOK_ELSE) || peek(TOK_ELIF) || peek(TOK_ENDIF)) {
						if (top)
							throw MacroParseException("else/elif/endif outside of if");
						return;

					// expression
					} else {
						stream << "print(";
						if (!acceptExpr(stream))
							throw MacroParseException("expected expression");
						expect(TOK_END);
						stream << ")\n";
					}

				} catch (MacroParseException& e) {
					// print if in top of body
					if (top)
						stream << "print(\"[[error: " << StreamChunk(lptr, sptr - lptr) << ": " << e.msg << "]]\")\n";
					else
						throw e;
				}

				// set lptr to the current character
				lptr = sptr;
			} else
				++sptr;
		}

		// spit out remaining bit of string
		if (sptr != lptr) {
			stream << "print(";
			printString(stream, lptr, sptr - lptr);
			stream << ")\n";
		}

		// if we hit the EOF and we're not the top-level block, that's an error
		if (!top)
			throw MacroParseException("unexpected end of string inside if block");
	}
};

// parsing
namespace macro {
	// macro text
	bool text(const StreamControl& stream, const std::string& in, const MacroArgs& argv) {
		// look up the macro cache
		lua_getfield(Lua::state, LUA_REGISTRYINDEX, "macro-cache");
		if (!lua_istable(Lua::state, -1)) {
			lua_pop(Lua::state, 1);
			lua_newtable(Lua::state);
			lua_pushvalue(Lua::state, -1);
			lua_setfield(Lua::state, LUA_REGISTRYINDEX, "macro-cache");
		}

		// look up the macro in the macro cache
		lua_getfield(Lua::state, -1, in.c_str());
		if (!lua_isfunction(Lua::state, -1)) {
			lua_pop(Lua::state, 1);

			// create compiler
			MacroCompiler mc(in.c_str(), in.size());

			// compile macro
			std::ostringstream source;
			mc.compile(source);
			Log::Info << "SCRIPT:\n" << source.str();

			// compile Lua script
			int rs = luaL_loadstring(Lua::state, source.str().c_str());
			if (rs != 0) {
				Log::Error << "Couldn't compile script: " << lua_tostring(Lua::state, -1);
				lua_pop(Lua::state, 2);
				return false;
			}

			// store in cache
			lua_pushvalue(Lua::state, -1);
			lua_setfield(Lua::state, -3, in.c_str());
			lua_remove(Lua::state, -2);
		}

		// execute the script
		Lua::setPrint(new StreamWrap(stream));
		int rs = lua_pcall(Lua::state, 0, 0, 0);
		Lua::setPrint(NULL);
		if (rs != 0) {
			Log::Error << "Couldn't execute script: " << lua_tostring(Lua::state, -1);
			lua_pop(Lua::state, 1);
			return false;
		}

		return true;
	}

	int execMacro(const StreamControl& stream, const std::string& command, MacroList& argv) {
		// FIXME: implement these in scripts/macro.lua
		if (strEq(command, "uptime")) {
			if (argv.size() != 0)
				return -1;
			stream << MUD::getUptime();
		} else if (strEq(command, "player-count")) {
			if (argv.size() != 0)
				return -1;
			stream << MPlayer.count();
		} else if (strEq(command, "day-or-night")) {
			if (argv.size() != 0)
				return -1;
			stream << (MTime.time.isNight() ? "night" : "day");
		} else if (strEq(command, "hostname")) {
			if (argv.size() != 0)
				return -1;
			stream << MNetwork.getHost();
		} else if (strEq(command, "date")) {
			if (argv.size() != 0)
				return -1;
			stream << MTime.time.dateStr();
		} else if (strEq(command, "time")) {
			if (argv.size() != 0)
				return -1;
			stream << MTime.time.timeStr();
		} else if (strEq(command, "date-year")) {
			if (argv.size() != 0)
				return -1;
			stream << MTime.time.getYear();
		} else if (strEq(command, "date-month")) {
			if (argv.size() != 0)
				return -1;
			stream << MTime.time.getMonth();
		} else if (strEq(command, "date-day")) {
			if (argv.size() != 0)
				return -1;
			stream << MTime.time.getDay();
		} else if (strEq(command, "time-hours24")) {
			if (argv.size() != 0)
				return -1;
			stream << MTime.time.getHour();
		} else if (strEq(command, "time-hours12")) {
			if (argv.size() != 0)
				return -1;
			uint hours = MTime.time.getHour();
			stream << (hours == 0 ? 12 : (hours <= 12 ? hours : hours - 12));
		} else if (strEq(command, "time-ampm")) {
			if (argv.size() != 0)
				return -1;
			stream << (MTime.time.getHour() < 12 ? "am" : "pm");
		} else if (strEq(command, "time-minutes")) {
			if (argv.size() != 0)
				return -1;
			stream << (MTime.time.getMinutes());
		} else {
			return -1;
		}
		return 0;
	}
}
