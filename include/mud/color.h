/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef COLOR_H
#define COLOR_H

#define ANSI_BLANK	""

#define ANSI_NORMAL	"\033[0m"

#define ANSI_UNDERLINE	"\033[0;4m"

#define ANSI_BLACK			"\033[0;30m"
#define ANSI_RED			"\033[0;31m"
#define ANSI_GREEN			"\033[0;32m"
#define ANSI_BROWN			"\033[0;33m"
#define ANSI_BLUE			"\033[0;34m"
#define ANSI_MAGENTA		"\033[0;35m"
#define ANSI_CYAN			"\033[0;36m"
#define ANSI_GREY			"\033[0;37m"
#define ANSI_LIGHTBLACK		"\033[1;30m"
#define ANSI_LIGHTRED		"\033[1;31m"
#define ANSI_LIGHTGREEN		"\033[1;32m"
#define ANSI_YELLOW			"\033[1;33m"
#define ANSI_LIGHTBLUE		"\033[1;34m"
#define ANSI_LIGHTMAGENTA	"\033[1;35m"
#define ANSI_LIGHTCYAN		"\033[1;36m"
#define ANSI_WHITE			"\033[1;37m"
#define ANSI_DARKRED		"\033[2;31m"
#define ANSI_DARKGREEN		"\033[2;32m"
#define ANSI_DARKYELLOW		"\033[2;33m"
#define ANSI_DARKBLUE		"\033[2;34m"
#define ANSI_DARKMAGENTA	"\033[2;35m"
#define ANSI_DARKCYAN		"\033[2;36m"
#define ANSI_DARKGREY		"\033[2;37m"

enum {
	COLOR_NORMAL = 0,
	COLOR_BLACK,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_BROWN,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_GREY,
	COLOR_LIGHTBLACK,
	COLOR_LIGHTRED,
	COLOR_LIGHTGREEN,
	COLOR_YELLOW,
	COLOR_LIGHTBLUE,
	COLOR_LIGHTMAGENTA,
	COLOR_LIGHTCYAN,
	COLOR_WHITE,
	COLOR_DARKRED,
	COLOR_DARKGREEN,
	COLOR_DARKYELLOW,
	COLOR_DARKBLUE,
	COLOR_DARKMAGENTA,
	COLOR_DARKCYAN,
	COLOR_DARKGREY,
};

#define CNORMAL		"\033!C0!"
#define CTITLE		"\033!C1!"
#define CDESC		"\033!C2!"
#define CPLAYER		"\033!C3!"
#define CNPC		"\033!C4!"
#define CITEM		"\033!C5!"
#define CSPECIAL	"\033!C6!"
#define CADMIN		"\033!C7!"
#define CWARNING	"\033!C7!"
#define CEXIT		"\033!C8!"
#define CSTAT		"\033!C9!"
#define CSTAT_BAD2	"\033!C10!"
#define CSTAT_BAD1	"\033!C11!"
#define CSTAT_GOOD1	"\033!C12!"
#define CSTAT_GOOD2	"\033!C13!"
#define CBOLD		"\033!C14!"
#define CTALK		"\033!C15!"

enum {
	COLOR_CLASS_NORMAL = 0,
	COLOR_CLASS_TITLE,
	COLOR_CLASS_DESC,
	COLOR_CLASS_PLAYER,
	COLOR_CLASS_NPC,
	COLOR_CLASS_ITEM,
	COLOR_CLASS_SPECIAL,
	COLOR_CLASS_ADMIN,
	COLOR_CLASS_EXIT,
	COLOR_CLASS_STAT,
	COLOR_CLASS_STATVBAD,
	COLOR_CLASS_STATBAD,
	COLOR_CLASS_STATGOOD,
	COLOR_CLASS_STATVGOOD,
	COLOR_CLASS_BOLD,
	COLOR_CLASS_TALK,
	NUM_CTYPES
};

/* in connection.cc */
extern std::string color_values[];
extern std::string color_value_names[];
extern std::string color_type_names[];
extern const int color_type_defaults[];
extern std::string color_type_rgb[];

#endif
