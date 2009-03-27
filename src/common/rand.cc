/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/rand.h"

void Random::init()
{
	srandom(time(NULL));
}

int Random::get(int range)
{
	if (range)
		return random() % range;
	else
		return 0;
}

int Random::roll(int dice, int sides)
{
	int val = 0;
	for (int i = 0; i < dice; i ++)
		val += Random::get(sides) + 1;
	return val;
}
