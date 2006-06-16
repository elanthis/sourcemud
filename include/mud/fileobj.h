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

class UniqueID;

namespace File
{
	class Error
	{
		protected:
		String what;

		public:
		Error (StringArg s_what) : what(s_what) {}

		inline String get_what () const { return what; }
	};

	enum DataType
	{
		TYPE_INT = 0,
		TYPE_BOOL,
		TYPE_STRING,
		TYPE_ID
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
		inline bool is_keyed () const { return type == KEYED; }
		inline bool is_custom () const { return type == CUSTOM; }
		inline bool is_end () const { return type == END; }
		inline bool is_begin () const { return type == BEGIN; }

		private:
		enum {
			ATTR,  // normal name = data
			KEYED, // attribute in name:key = data
			CUSTOM,  // for scripts, @name = data
			BEGIN, // object form name { data }
			END,   // } after a BEGIN
		} type;
		String name; // only in data or begin
		String key; // for keyed attributes
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
		int open (StringArg file);
		bool is_open () const { return in; }
		void close () { if (in) in.close(); }

		// fetch another node from the input
		bool get (Node& node);
		// consume the rest of the current begin
		void consume ();

		// get current line
		inline size_t get_line () const { return line; }

		private:
		std::ifstream in;
		String filename;
		size_t line;

		enum Token { TOKEN_ERROR, TOKEN_EOF, TOKEN_STRING, TOKEN_NUMBER, TOKEN_TRUE, TOKEN_FALSE, TOKEN_BEGIN, TOKEN_END, TOKEN_SET, TOKEN_CUSTOM, TOKEN_ID, TOKEN_KEY };

		Token read_token(String& data);
	};

	class Writer : public Cleanup
	{
		public:
		Writer () : out(), indent(0) {}
		Writer (StringArg file) : out(), indent(0) { open(file); }
		~Writer () { close(); }

		int open (StringArg file);
		bool is_open () const { return out; }
		void close ();

		// core attributes
		void attr (StringArg name, StringArg data);
		void attr (StringArg name, long data);
		void attr (StringArg name, long long data);
		void attr (StringArg name, bool data);
		void attr (StringArg name, const UniqueID& data);

		void keyed (StringArg name, StringArg key, StringArg data);
		void keyed (StringArg name, StringArg key, long data);
		void keyed (StringArg name, StringArg key, bool data);
		void keyed (StringArg name, StringArg key, const UniqueID& data);

		// map int types to long
		//   -- we only support reading longs
		inline void attr (StringArg name, unsigned long data) { attr(name, (long)data); }
		inline void attr (StringArg name, int data) { attr(name, (long)data); }
		inline void attr (StringArg name, unsigned int data) { attr(name, (long)data); }
		inline void attr (StringArg name, short data) { attr(name, (long)data); }
		inline void attr (StringArg name, unsigned short data) { attr(name, (long)data); }

		inline void keyed (StringArg name, StringArg key, unsigned long data) { keyed(name, key, (long)data); }
		inline void keyed (StringArg name, StringArg key, int data) { keyed(name, key, (long)data); }
		inline void keyed (StringArg name, StringArg key, unsigned int data) { keyed(name, key, (long)data); }
		inline void keyed (StringArg name, StringArg key, short data) { keyed(name, key, (long)data); }
		inline void keyed (StringArg name, StringArg key, unsigned short data) { keyed(name, key, (long)data); }

		// custom attributes (for scripts)
		void custom (StringArg name, StringArg data);
		void custom (StringArg name, long data);
		void custom (StringArg name, bool data);
		void custom (StringArg name, const UniqueID& data);

		// output a data block class:name<<< ... >>> 
		void block (StringArg name, StringArg data);

		// begin a new section
		void begin (StringArg name);

		// end a section
		void end ();

		// output a comment
		void comment (StringArg text);

		// add a blank line to output
		inline void bl () { if(out) out << "\n"; }

		private:
		std::ofstream out;
		size_t indent;

		void do_indent();
	};

	// return true if a valid attribute/object name
	bool valid_name (StringArg name);
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
#define FO_ATTR(name) \
		} else if (node.is_attr() && node.get_name() == name) {
#define FO_KEYED(name) \
		} else if (node.is_keyed() && node.get_name() == name) {
#define FO_CUSTOM \
		} else if (node.is_custom()) {
#define FO_ATTR_BEGIN \
		} else if (node.is_attr()) { \
			do { \
				/* custom parsing code */
#define FO_ATTR_END \
				if (_x_is_read) { \
					Log::Error << "Unrecognized " << (node.is_attr() ? "attribute" : "object") << " '" << node.get_name() << "' at " << reader.get_filename() << ':' << node.get_line() << " in " << __FILE__ <<  ':' << __LINE__; \
					throw(File::Error("unexpected value")); \
				} else { \
					return FO_NOTFOUND_CODE; \
				} \
			} while(false); 
#define FO_OBJECT(name) \
		} else if (node.is_begin() && node.get_name() == name) {
#define FO_OBJECT_ANY \
		} else if (node.is_begin()) {
#define FO_PARENT(klass) \
		} else if (klass::load_node(reader, node) == FO_SUCCESS_CODE) { \
			/* no-op */
#define FO_READ_ERROR \
		} else { \
			if (node.is_begin()) \
				reader.consume(); \
			if (node.is_attr()) Log::Error << "Unrecognized attribute '" << node.get_name() << "' at " << reader.get_filename() << ':' << node.get_line(); \
			else if (node.is_keyed()) Log::Error << "Unrecognized keyed attribute '" << node.get_name() << "' at " << reader.get_filename() << ':' << node.get_line(); \
			else if (node.is_custom()) Log::Error << "Unexpected custom attribute '@" << node.get_name() << "' at " << reader.get_filename() << ':' << node.get_line(); \
			else if (node.is_begin()) Log::Error << "Unrecognized object '" << node.get_name() << "' at " << reader.get_filename() << ':' << node.get_line(); \
			throw(File::Error("unexpected value")); \
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
		throw(File::Error("data type mismatch")); \
	}

#endif
