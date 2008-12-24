/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"

void
init_random (void)
{
	srandom (time (NULL));
}

int
get_random (int range)
{
	if (range)
		return random () % range;
	else
		return 0;
}

int
roll_dice (int dice, int sides)
{
	int val = 0;
	for (int i = 0; i < dice; i ++)
		val += get_random (sides) + 1;
	return val;
}
