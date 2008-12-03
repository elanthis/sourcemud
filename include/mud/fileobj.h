/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef __FILEAPI_H__
#define __FILEAPI_H__

#include "common.h"
#include "common/log.h"
#include "mud/uniqid.h"

namespace File
{
	class Error
	{
		protected:
		std::string what;

		public:
		Error () : what() {}
		Error (const std::string& s_what) : what(s_what) {}

		inline const std::string& get_what () const { return what; }
	};

	class Value
	{
		public:
		enum Type {
			TYPE_NONE,
			TYPE_STRING,
			TYPE_INT,
			TYPE_BOOL,
			TYPE_ID,
			TYPE_LIST,
		};

		Value () : type(TYPE_NONE), value(), list() {}
		Value (Type s_type, const std::string& s_value) : type(s_type), value(s_value), list() {}
		Value (const std::vector<Value>& s_list) : type(TYPE_LIST), value(), list(s_list) {}

		Type get_type () const { return type; }
		std::string get_value () const { return value; }
		const std::vector<Value>& get_list () const { return list; }

		private:
		Type type;
		std::string value;
		std::vector<Value> list;
	};

	class Node
	{
		public:
		enum Type {
			ATTR,  // normal ns.name = data
			BEGIN_UNTYPED, // object form ns.name { data }
			BEGIN_TYPED, // object form ns.name = type { data }
			END,   // } after a BEGIN_UNTYPED or BEGIN_TYPED
			ERROR // error
		};

		Node (class Reader& s_reader) : reader(s_reader) {}

		inline std::string get_ns () const { return ns; }
		inline std::string get_name () const { return name; }
		inline Value::Type get_value_type () const { return value.get_type(); }

		// type checking getters; throws File::Error on type-mismatch
		std::string get_string () const;
		bool get_bool () const;
		int get_int () const;
		UniqueID get_id () const;

		const std::vector<Value>& get_list () const;
		const std::vector<Value>& get_list (size_t size) const;

		std::string get_string (size_t index) const;
		int get_int (size_t index) const;

		// file info
		class Reader& get_reader () const { return reader; }
		inline size_t get_line () const { return line; }

		// type of node
		inline bool is_attr () const { return type == ATTR; }
		inline bool is_end () const { return type == END; }
		inline bool is_begin () const { return type == BEGIN_UNTYPED || type == BEGIN_TYPED; }
		inline bool is_begin_untyped () const { return type == BEGIN_UNTYPED; }
		inline bool is_begin_typed () const { return type == BEGIN_TYPED; }
		inline Type get_node_type () const { return type; }

		private:
		Type type;
		class Reader& reader;
		std::string ns;
		std::string name;
		Value value;
		size_t line; // line node came from

		friend class Reader;
	};

	class Reader
	{
		public:
		Reader () : in(), filename(), line(0) {}
		~Reader () { close(); }

		const std::string get_filename () const { return filename; }
		int open (const std::string& file);
		bool is_open () const { return in; }
		void close () { if (in) in.close(); }

		// fetch another node from the input
		bool get (Node& node);
		// consume the rest of the current begin
		void consume ();

		// get the current path
		std::string get_path () const { return filename; }

		// get current line
		inline size_t get_line () const { return line; }

		private:
		std::ifstream in;
		std::string filename;
		size_t line;

		enum Token { TOKEN_ERROR, TOKEN_EOF, TOKEN_STRING, TOKEN_NUMBER, TOKEN_TRUE, TOKEN_FALSE, TOKEN_BEGIN, TOKEN_END, TOKEN_SET, TOKEN_ID, TOKEN_KEY, TOKEN_START_LIST, TOKEN_END_LIST, TOKEN_COMMA, TOKEN_NAME };

		Token read_token(std::string& data);
		bool set_value (Token type, std::string& data, Value& value);
	};

	class Writer
	{
		public:
		Writer () : out(), indent(0) {}
		Writer (const std::string& file) : out(), indent(0) { open(file); }
		~Writer () { close(); }

		int open (const std::string& file);
		bool is_open () const { return out; }
		void close ();

		// attributes
		void attr (const std::string& ns, const std::string& name, const std::string& data);
		void attr (const std::string& ns, const std::string& name, long data);
		void attr (const std::string& ns, const std::string& name, bool data);
		void attr (const std::string& ns, const std::string& name, const UniqueID& data);
		void attr (const std::string& ns, const std::string& name, const std::vector<Value>& list);

		inline void attr (const std::string& ns, const std::string& name, unsigned long data) { attr(ns, name, (long)data); }
		inline void attr (const std::string& ns, const std::string& name, int data) { attr(ns, name, (long)data); }
		inline void attr (const std::string& ns, const std::string& name, unsigned int data) { attr(ns, name, (long)data); }
		inline void attr (const std::string& ns, const std::string& name, short data) { attr(ns, name, (long)data); }
		inline void attr (const std::string& ns, const std::string& name, unsigned short data) { attr(ns, name, (long)data); }

		// output a data block
		void block (const std::string& ns, const std::string& name, const std::string& data);

		// begin a new section
		void begin (const std::string& ns, const std::string& name);

		// open begin... MUST be followed by a regular begin
		void begin_attr (const std::string& ns, const std::string& name, const std::string& type);

		// end a section
		void end ();

		// output a comment
		void comment (const std::string& text);

		// add a blank line to output
		inline void bl () { if(out) out << "\n"; }

		private:
		std::string path;
		std::ofstream out;
		size_t indent;

		void do_indent();
	};

	// return true if a valid attribute/object name
	bool valid_name (const std::string& name);
}

// stream a node
const StreamControl& operator<< (const StreamControl& stream, const File::Node& node);

// -- Special Easy Helpers - Yay --
#define FO_ERROR_CODE -1
#define FO_SUCCESS_CODE 0
#define FO_NOTFOUND_CODE 1
#define FO_READ_BEGIN \
	do { \
		try { \
			const bool _x_is_read = true; \
			File::Node node(reader); \
			while (reader.get(node) && _x_is_read) { \
				if (node.is_end()) { \
					break;
#define FO_NODE_BEGIN \
	do { \
	const bool _x_is_read = false; \
	if (false && _x_is_read) {
#define FO_ATTR(ns,name) \
		} else if (node.is_attr() && node.get_ns() == ns && node.get_name() == name) {
#define FO_WILD(ns) \
		} else if (node.is_attr() && node.get_ns() == ns) {
#define FO_OBJECT(ns,name) \
		} else if (node.is_begin_untyped() && node.get_ns() == ns && node.get_name() == name) {
#define FO_ENTITY(ns,name) \
		} else if (node.is_begin_typed() && node.get_ns() == ns && node.get_name() == name) { \
			Entity* entity = Entity::load(node.get_string(), reader); \
			if (entity == NULL) throw File::Error();
#define FO_PARENT(ns) \
		} else if (ns::load_node(reader, node) == FO_SUCCESS_CODE) { \
			/* no-op */
#define FO_READ_ERROR \
		} else { \
			if (node.is_begin()) \
				reader.consume(); \
			Log::Error << node << ": Unrecognized attribute"; \
			throw File::Error(); \
		} \
	} } catch (File::Error& error) { \
		if (!error.get_what().empty()) Log::Error << reader.get_filename() << ',' << reader.get_line() << ": " << error.get_what();
#define FO_READ_END \
	} } while(false);
#define FO_NODE_END \
	} else { \
		/* nothing found */ \
		return FO_NOTFOUND_CODE; \
	} \
	} while(false); \
	/* found match */ \
	return FO_SUCCESS_CODE;

#endif
