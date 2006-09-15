/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef __FILEAPI_H__
#define __FILEAPI_H__

#include <sys/stat.h>

#include <fstream>

#include "common/string.h"
#include "common/gcbase.h"
#include "common/log.h"
#include "scriptix/native.h"
#include "common/gcmap.h"
#include "common/gcvector.h"
#include "mud/uniqid.h"

namespace File
{
	enum DataType
	{
		TYPE_INT = 0,
		TYPE_BOOL,
		TYPE_STRING,
		TYPE_ID
	};

	class Error
	{
		protected:
		String what;

		public:
		Error (String s_what) : what(s_what) {}

		inline String get_what () const { return what; }
	};

	class KeyError : public Error
	{
		public:
		KeyError (DataType type, String name, uint index);
	};

	class Node : public GC
	{
		public:
		inline String get_name () const { return name; }
		inline String get_key () const { return key; }
		inline DataType get_datatype () const { return datatype; }

		// data getter
		inline const String& get_data () { return data; }

		// type checking getters; throws File::Error on type-mismatch
		String get_string () const;
		bool get_bool () const;
		int get_int () const;
		UniqueID get_id () const;

		// line number of node
		inline size_t get_line () const { return line; }

		// type of node
		inline bool is_attr () const { return type == ATTR; }
		inline bool is_end () const { return type == END; }
		inline bool is_begin () const { return type == BEGIN; }

		private:
		enum {
			ATTR,  // normal name = data
			KEYED, // attribute in name:key = data
			BEGIN, // object form name { data }
			END,   // } after a BEGIN
		} type;
		String name; // basic name
		String key; // attribute key
		String data; // our data
		DataType datatype; // type of data
		size_t line; // line node came from

		friend class Reader;
	};

	class Reader : public Cleanup
	{
		public:
		Reader () : in(), filename(), line(0) {}
		~Reader () { close(); }

		const String get_filename () const { return filename; }
		int open (String file);
		bool is_open () const { return in; }
		void close () { if (in) in.close(); }

		// fetch another node from the input
		bool get (Node& node);
		// consume the rest of the current begin
		void consume ();

		// get the current path
		String get_path () const { return filename; }

		// get current line
		inline size_t get_line () const { return line; }

		private:
		std::ifstream in;
		String filename;
		size_t line;

		enum Token { TOKEN_ERROR, TOKEN_EOF, TOKEN_STRING, TOKEN_NUMBER, TOKEN_TRUE, TOKEN_FALSE, TOKEN_BEGIN, TOKEN_END, TOKEN_SET, TOKEN_ID, TOKEN_KEY };

		Token read_token(String& data);
	};

	class Object : public GC
	{
		public:
		int load (Reader& reader);

		String get_string (String name, uint index) const;
		bool get_bool (String name, uint index) const;
		int get_int (String name, uint index) const;
		UniqueID get_id (String name, uint index) const;

		size_t get_string_count (String name) const;
		size_t get_bool_count (String name) const;
		size_t get_int_count (String name) const;
		size_t get_id_count (String name) const;
		
		private:
		typedef GCType::map<String, GCType::vector<String> > StringList;
		typedef GCType::map<String, GCType::vector<bool> > BoolList;
		typedef GCType::map<String, GCType::vector<int> > IntList;
		typedef GCType::map<String, GCType::vector<UniqueID> > IDList;

		StringList strings;
		BoolList bools;
		IntList ints;
		IDList ids;
	};

	class Writer : public Cleanup
	{
		public:
		Writer () : out(), indent(0) {}
		Writer (String file) : out(), indent(0) { open(file); }
		~Writer () { close(); }

		int open (String file);
		bool is_open () const { return out; }
		void close ();

		// attributes
		void attr (String name, String key, String data);
		void attr (String name, String key, long data);
		void attr (String name, String key, bool data);
		void attr (String name, String key, const UniqueID& data);

		inline void attr (String name, String key, unsigned long data) { attr(name, key, (long)data); }
		inline void attr (String name, String key, int data) { attr(name, key, (long)data); }
		inline void attr (String name, String key, unsigned int data) { attr(name, key, (long)data); }
		inline void attr (String name, String key, short data) { attr(name, key, (long)data); }
		inline void attr (String name, String key, unsigned short data) { attr(name, key, (long)data); }

		// output a data block
		void block (String name, String key, String data);

		// begin a new section
		void begin (String name);

		// end a section
		void end ();

		// output a comment
		void comment (String text);

		// add a blank line to output
		inline void bl () { if(out) out << "\n"; }

		private:
		std::ofstream out;
		size_t indent;

		void do_indent();
	};

	// return true if a valid attribute/object name
	bool valid_name (String name);
}

// can only write simply attributes, all with type "attr"
class ScriptRestrictedWriter : public Scriptix::Native
{
	public:
	ScriptRestrictedWriter (File::Writer* s_writer);

	inline File::Writer* get_writer (void) const { return writer; }

	inline void release (void) { writer = NULL; }

	private:
	File::Writer* writer;
};

// -- Special Easy Helpers - Yay --
#define FO_ERROR_CODE -1
#define FO_SUCCESS_CODE 0
#define FO_NOTFOUND_CODE 1
#define FO_READ_BEGIN \
	try { \
		const bool _x_is_read = true; \
		File::Node node; \
		while (reader.get(node) && _x_is_read) { \
			if (node.is_end()) { \
				break;
#define FO_NODE_BEGIN \
	do { \
	const bool _x_is_read = false; \
	if (false && _x_is_read) {
#define FO_ATTR(name,key) \
		} else if (node.is_attr() && node.get_name() == name && node.get_key() == key) {
#define FO_WILD(name) \
		} else if (node.is_attr() && node.get_name() == name) {
#define FO_OBJECT(name) \
		} else if (node.is_begin() && node.get_name() == name) {
#define FO_PARENT(klass) \
		} else if (klass::load_node(reader, node) == FO_SUCCESS_CODE) { \
			/* no-op */
#define FO_READ_ERROR \
		} else { \
			if (node.is_begin()) \
				reader.consume(); \
			else if (node.is_attr()) Log::Error << "Unrecognized attribute '" << node.get_name() << '.' << node.get_key() << "' at " << reader.get_filename() << ':' << node.get_line(); \
			else if (node.is_begin()) Log::Error << "Unrecognized object '" << node.get_name() << "' at " << reader.get_filename() << ':' << node.get_line(); \
			throw(File::Error(S("unexpected value"))); \
		} \
	} } catch (File::Error& error) { \
		Log::Error << error.get_what() << " at " << reader.get_filename() << ':' << reader.get_line();
#define FO_READ_END \
	}
#define FO_NODE_END \
	} else { \
		/* nothing found */ \
		return FO_NOTFOUND_CODE; \
	} \
	} while(false); \
	/* found match */ \
	return FO_SUCCESS_CODE;
#define FO_TYPE_ASSERT(type) \
	if(node.get_datatype() != (File::TYPE_ ## type)) { \
		Log::Error << "Incorrect data type for '" << node.get_name() << "' at " << reader.get_filename() << ':' << node.get_line(); \
		throw(File::Error(S("data type mismatch"))); \
	}

#endif
