/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_BITSETS_H
#define AWEMUD_COMMON_BITSETS_H

#include "common/types.h"

// MAX: total number of bytes needed for N bits
// MAXPAD: same as MAX, padded up to 4 bytes (FIXME: 8 on 64-bit?)
// BYTE: byte index of a given bit N
// SHIFT: bit index of byte of given bit N

#define BITSET_MAX(n) (n/8+(n%8>0))
#define BITSET_MAXPAD(n) (BITSET_MAX(n)+BITSET_MAX(n)%4)
#define BITSET_BYTE(n) ((n-1)/8)
#define BITSET_SHIFT(n) (1<<((n-1)%8))

typedef uint bit_t;

template<uint N> class BitSet
{
	public:
	void clear() { memset(bits, 0, sizeof(bits)); }

	bool check(bit_t n) const { return bits[BITSET_BYTE(n)] & BITSET_SHIFT(n); }
	void set_on(bit_t n) { bits[BITSET_BYTE(n)] |= BITSET_SHIFT(n); }
	void set_off(bit_t n) { bits[BITSET_BYTE(n)] &= ~BITSET_SHIFT(n); }
	void set(bit_t n, bool b) { if(b) set_on(n); else set_off(n); }

	uint size() const { return N; }
	uint bytes() const { return sizeof(bits); }

	const unsigned char* get() const { return bits; }

	private:
	unsigned char bits[BITSET_MAX(N)];
};

typedef BitSet<32> bits_t;

template<uint N> bool operator & (const BitSet<N>& set, bit_t n) { return set.check(n); }
template<uint N> BitSet<N>& operator &= (BitSet<N>& set, bit_t n) { set.set_on(n); return set; }
template<uint N> BitSet<N>& operator += (BitSet<N>& set, bit_t n) { set.set_on(n); return set; }
template<uint N> BitSet<N>& operator -= (BitSet<N>& set, bit_t n) { set.set_off(n); return set; }

#endif
