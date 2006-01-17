/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef CLOCK_H
#define CLOCK_H

#define TICKS_PER_ROUND 4		/* how many ticks in one round? */
#define GAME_TIME_SCALE 4		/* speed of game time compared to real time */

#define TICKS_TO_ROUNDS(t) ((t) / TICKS_PER_ROUND)
#define ROUNDS_TO_TICKS(s) ((s) * TICKS_PER_ROUND)

// current number of game ticks
namespace AweMUD {
	unsigned long get_ticks (void);
	unsigned long get_rounds (void);
}

// NOTE: code in server.cc

#endif
