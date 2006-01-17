/*
 * Scriptix - Lite-weight scripting interface
 * Copyright (c) 2002, 2003, 2004, 2005  AwesomePlay Productions, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "common/gcvector.h"
#include "scriptix/value.h"
#include "scriptix/iterator.h"
#include "scriptix/type.h"

namespace Scriptix {

class Array : public IValue {
	public:
	Array ();
	Array (size_t n_size, Value n_list[]);

	virtual const TypeInfo* get_type () const;

	// IValue Operations
	protected:
	virtual bool is_true () const;

	// Array Operations
	public:
	virtual Value get_index (long index) const;
	virtual Value set_index (long index, Value set);
	virtual Value append (Value value);
	virtual bool has (Value value) const;
	virtual Iterator* get_iter ();

	// Methods
	public:
	static Value method_length (size_t argc, Value argv[]);
	static Value method_append (size_t argc, Value argv[]);
	static Value method_remove (size_t argc, Value argv[]);
	static Value method_iter (size_t argc, Value argv[]);

	// Custom
	public:
	inline size_t get_count (void) const { return list.size(); }
	inline Value get_index (size_t i) const { return list[i]; }
	// NOTE: the following should only be used on ranges 0 thru (count - 1)
	inline Value set_index (size_t i, Value value) { return list[i] = value; }

	// Iterators
	public:
	class ArrayIterator : public Scriptix::Iterator
	{
		// data
		private:
		Array* array;
		size_t index;

		// Next iterator
		public:
		virtual bool next (Value& value);

		// Constructor
		public:
		inline ArrayIterator (Array* s_arr) :
			Scriptix::Iterator(), array(s_arr), index(0) {}
	};

	public:
	inline static Value get_index (const Array* array, long index)
	{
		return array->get_index(index);
	}
	inline static Value set_index (Array* array, long index, Value set)
	{
		return array->set_index(index, set);
	}
	inline static Value append (Array* array, Value value)
	{
		return array->append(value);
	}
	inline static bool Has (const Array* array, Value value)
	{
		return array->has(value);
	}
	inline static Iterator* get_iter (Array* array)
	{
		return array->get_iter();
	}

	protected:
	GCType::vector<Value> list;
};

} // namespace Scriptix
