/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_ELIST_H
#define AWEMUD_MUD_ELIST_H

#include <algorithm>

#include "common/string.h"
#include "common/gcvector.h"
#include "common/error.h"

// --- EList Definition ---
template<class EntType>
class EList : public GCType::vector<EntType*>
{
	typedef GCType::vector<EntType*> vtype;

	public:
	EList<EntType> (void) : GCType::vector<EntType*>() {}
	EList<EntType> (size_t size) : GCType::vector<EntType*>(size) {}

	void add (EntType* ent);
	void remove (EntType* ent);
	bool has (EntType* ent);
	EntType* match (String str, uint index = 1, uint *matches = NULL);
};

// --- EList Implementation ---

template<class EntType>
void
EList<EntType>::add(EntType* ent)
{
	assert (ent != NULL);

	// sorted insert...
	for (typename vtype::iterator i = vtype::begin(); i != vtype::end(); ++i) {
		// duplicate?  grr... quit
		if (ent == *i)
			return;
		// if it's lower than the new one, insert us after
		if (!(**i < *ent)) {
			++i;
			insert(i, ent);
			return;
		}
	}
	// no sorted insert and no duplicate, push to back
	push_back(ent);
}

template<class EntType>
void
EList<EntType>::remove(EntType* ent)
{
	assert (ent != NULL);
	typename vtype::iterator i = std::find(vtype::begin(),vtype::end(),ent);
	if (i != vtype::end())
		erase(i);
}

template<class EntType>
bool
EList<EntType>::has(EntType* ent)
{
	assert (ent != NULL);
	typename vtype::iterator i = std::find(vtype::begin(),vtype::end(),ent);
	return i != vtype::end();
}

template<class EntType>
EntType*
EList<EntType>::match (String str, uint index, uint *matches)
{
	assert (!str.empty());
	assert (index != 0);

	// loop info
	uint count = 0;
	EntType* value = NULL;

	// the loop
	for (typename vtype::iterator i = vtype::begin(); i != vtype::end(); ++i)
		if ((*i)->name_match (str))
			if (++count == index) {
				value = *i;
				break;
			}

	// store count
	if (matches)
		*matches = count;

	// return find, if we had any
	return value;
}

#endif
