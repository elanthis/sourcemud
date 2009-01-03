/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_BITSETS_H
#define SOURCEMUD_COMMON_BITSETS_H

#include "common/types.h"

// MAX: total number of bytes needed for N bits
// MAXPAD: same as MAX, padded up to 4 bytes (FIXME: 8 on 64-bit?)
// BYTE: byte index of a given bit N
// SHIFT: bit index of byte of given bit N

#define BITSET_MAX(n) (n/8+(n%8>0))
#define BITSET_MAXPAD(n) (BITSET_MAX(n)+BITSET_MAX(n)%4)
#define BITSET_BYTE(n) ((n-1)/8)
#define BITSET_SHIFT(n) (1<<((n-1)%8))

template<typename E> class BitSet
{
public:
	typedef E bit_t;

	BitSet<E>() { clear(); }

	void clear() { memset(bits, 0, sizeof(bits)); }

	bool check(E n) const { return bits[BITSET_BYTE(n)] & BITSET_SHIFT(n); }
	void set_on(E n) { bits[BITSET_BYTE(n)] |= BITSET_SHIFT(n); }
	void set_off(E n) { bits[BITSET_BYTE(n)] &= ~BITSET_SHIFT(n); }
	void set(E n, bool b) { if (b) set_on(n); else set_off(n); }

	uint size() const { return E::MAX - 1; }
	uint bytes() const { return sizeof(bits); }

	const unsigned char* get() const { return bits; }

private:
	unsigned char bits[BITSET_MAX(E::MAX-1)];
};

template<typename E> bool operator & (const BitSet<E>& set, typename BitSet<E>::bit_t n) { return set.check(n); }
template<typename E> BitSet<E>& operator &= (BitSet<E>& set, typename BitSet<E>::bit_t n) { set.set_on(n); return set; }
template<typename E> BitSet<E>& operator += (BitSet<E>& set, typename BitSet<E>::bit_t n) { set.set_on(n); return set; }
template<typename E> BitSet<E>& operator -= (BitSet<E>& set, typename BitSet<E>::bit_t n) { set.set_off(n); return set; }

#endif
