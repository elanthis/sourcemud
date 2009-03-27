/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef __FILEAPI_H__
#define __FILEAPI_H__

#include "common/log.h"

namespace File
{
class Error
{
protected:
	std::string what;

public:
	Error() : what() {}
	Error(const std::string& s_what) : what(s_what) {}

	inline const std::string& getWhat() const { return what; }
};

class Value
{
public:
	enum Type {
		TYPE_NONE,
		TYPE_STRING,
		TYPE_INT,
		TYPE_BOOL,
		TYPE_LIST,
	};

	Value() : type(TYPE_NONE), value(), list() {}
	Value(Type s_type, const std::string& s_value) : type(s_type), value(s_value), list() {}
	Value(const std::vector<Value>& s_list) : type(TYPE_LIST), value(), list(s_list) {}

	Type getType() const { return type; }
	std::string getValue() const { return value; }
	const std::vector<Value>& getList() const { return list; }

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

	Node(class Reader& s_reader) : reader(s_reader) {}

	inline std::string getNs() const { return ns; }
	inline std::string getName() const { return name; }
	inline Value::Type getValueType() const { return value.getType(); }

	// type checking getters; throws File::Error on type-mismatch
	std::string getString() const;
	bool getBool() const;
	int getInt() const;

	const std::vector<Value>& getList() const;
	const std::vector<Value>& getList(size_t size) const;

	std::string getString(size_t index) const;
	int getInt(size_t index) const;

	// file info
	class Reader& getReader() const { return reader; }
	inline size_t getLine() const { return line; }

	// type of node
	inline bool isAttr() const { return type == ATTR; }
	inline bool isEnd() const { return type == END; }
	inline bool isBegin() const { return type == BEGIN_UNTYPED || type == BEGIN_TYPED; }
	inline bool isBeginUntyped() const { return type == BEGIN_UNTYPED; }
	inline bool isBeginTyped() const { return type == BEGIN_TYPED; }
	inline Type getNodeType() const { return type; }

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
	Reader() : in(), filename(), line(0) {}
	~Reader() { close(); }

	const std::string getFilename() const { return filename; }
	int open(const std::string& file);
	bool isOpen() const { return in; }
	void close() { if (in) in.close(); }

	// fetch another node from the input
	bool get(Node& node);
	// consume the rest of the current begin
	void consume();

	// get the current path
	std::string getPath() const { return filename; }

	// get current line
	inline size_t getLine() const { return line; }

private:
	std::ifstream in;
	std::string filename;
	size_t line;

	enum Token { TOKEN_ERROR, TOKEN_EOF, TOKEN_STRING, TOKEN_NUMBER, TOKEN_TRUE, TOKEN_FALSE, TOKEN_BEGIN, TOKEN_END, TOKEN_SET, TOKEN_KEY, TOKEN_START_LIST, TOKEN_END_LIST, TOKEN_COMMA, TOKEN_NAME };

	Token readToken(std::string& data);
	bool setValue(Token type, std::string& data, Value& value);
};

class Writer
{
public:
	Writer() : out(), indent(0) {}
	Writer(const std::string& file) : out(), indent(0) { open(file); }
	~Writer() { close(); }

	int open(const std::string& file);
	bool isOpen() const { return out; }
	void close();

	// attributes
	void attr(const std::string& ns, const std::string& name, const std::string& data);
	void attr(const std::string& ns, const std::string& name, long data);
	void attr(const std::string& ns, const std::string& name, bool data);
	void attr(const std::string& ns, const std::string& name, const std::vector<Value>& list);

	inline void attr(const std::string& ns, const std::string& name, unsigned long data) { attr(ns, name, (long)data); }
	inline void attr(const std::string& ns, const std::string& name, int data) { attr(ns, name, (long)data); }
	inline void attr(const std::string& ns, const std::string& name, unsigned int data) { attr(ns, name, (long)data); }
	inline void attr(const std::string& ns, const std::string& name, short data) { attr(ns, name, (long)data); }
	inline void attr(const std::string& ns, const std::string& name, unsigned short data) { attr(ns, name, (long)data); }

	// output a data block
	void block(const std::string& ns, const std::string& name, const std::string& data);

	// begin a new section
	void begin(const std::string& ns, const std::string& name);

	// open begin... MUST be followed by a regular begin
	void beginAttr(const std::string& ns, const std::string& name, const std::string& type);

	// end a section
	void end();

	// output a comment
	void comment(const std::string& text);

	// add a blank line to output
	inline void bl() { if (out) out << "\n"; }

private:
	std::string path;
	std::ofstream out;
	size_t indent;

	void doIndent();
};

// return true if a valid attribute/object name
bool validName(const std::string& name);
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
				if (node.isEnd()) { \
					break;
#define FO_NODE_BEGIN \
	do { \
	const bool _x_is_read = false; \
	if (false && _x_is_read) {
#define FO_ATTR(ns,name) \
		} else if (node.isAttr() && node.getNs() == ns && node.getName() == name) {
#define FO_WILD(ns) \
		} else if (node.isAttr() && node.getNs() == ns) {
#define FO_OBJECT(ns,name) \
		} else if (node.isBeginUntyped() && node.getNs() == ns && node.getName() == name) {
#define FO_ENTITY(ns,name) \
		} else if (node.isBeginTyped() && node.getNs() == ns && node.getName() == name) { \
			Entity* entity = Entity::load(node.getString(), reader); \
			if (entity == NULL) throw File::Error();
#define FO_PARENT(ns) \
		} else if (ns::loadNode(reader, node) == FO_SUCCESS_CODE) { \
			/* no-op */
#define FO_READ_ERROR \
		} else { \
			if (node.isBegin()) \
				reader.consume(); \
			Log::Error << node << ": Unrecognized attribute"; \
			throw File::Error(); \
		} \
	} } catch (File::Error& error) { \
		if (!error.getWhat().empty()) Log::Error << reader.getFilename() << ',' << reader.getLine() << ": " << error.getWhat();
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
