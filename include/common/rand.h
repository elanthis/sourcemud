/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef RAND_H
#define RAND_H

namespace Random {
	void init();		// initializes random numbers
	int get(int range);		// returns 0 - range
	int roll(int dice, int sides);	// returns <dice>d<sides> roll
}

#endif
