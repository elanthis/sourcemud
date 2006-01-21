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

#ifndef __SX_COMPILER_H__
#define __SX_COMPILER_H__

#include "common/gcbase.h"
#include "common/gcvector.h"

#include "scriptix/array.h"

namespace Scriptix {
namespace Compiler {

class CompilerNode;
class CompilerFunction;
class CompilerExtend;
class CompilerBlock;

typedef GCType::vector<Atom> NameList;

class Compiler : public GCType::GC
{
	public:
	Compiler ();

	// building trees/input
	inline void set_file(const BaseString& path) { file = new String(path); line = 1; }
	inline void set_line(size_t lineno) { line = lineno; }
	inline void LineIncr() { ++line; }
	inline String* get_file() const { return file; }
	inline size_t get_line() const { return line; }
	inline void set_handler(CompilerHandler* s_handler) { handler = s_handler; }

	// compile
	int Compile();

	// error function
	void Error(const BaseString& msg);

	// get the index for a variable name (ret -1 on error)
	long add_var(CompilerFunction* func, Atom id);
	long get_var(CompilerFunction* func, Atom id);

	// define global
	void set_global(Atom id, Value value);
	long get_global(Atom id);

	// add functions
	CompilerFunction* add_func(Atom name, const NameList& args, CompilerNode* body, bool pub);

	// new types
	TypeInfo* add_type(Atom name, const TypeInfo* parent);
	CompilerFunction* add_typeFunc(Atom name, const NameList& args, CompilerNode* body);
	CompilerFunction* add_method(Atom name, const NameList& args, CompilerNode* body);
	TypeInfo* get_type(Atom name);

	public: // FIXME
	typedef GCType::vector<CompilerFunction*> FunctionList;
	typedef GCType::vector<CompilerExtend*> ExtendList;
	typedef GCType::vector<TypeInfo*> TypeList;
	typedef GCType::vector<unsigned long> ReturnList;
	typedef GCType::vector<CompilerBlock*> BlockList;
	typedef GCType::vector<DebugMetaData> DebugList;

	CompilerHandler* handler;
	CompilerNode* nodes;
	FunctionList funcs;
	ExtendList extends;
	TypeList types;
	ReturnList returns;
	size_t last_line;
	size_t last_op;
	String* file;
	size_t line;
	BlockList blocks;
	DebugList debug;
	Array* globals;
	NameList gnames;

	// compiler helopers
	bool CompileNode (CompilerFunction* func, CompilerNode* node);
	// block areas
	bool push_block (Function* func);
	void pop_block ();
	bool add_break ();
	bool add_breakOnFalse ();
	bool add_breakOnTrue ();
	bool add_continue ();
	inline long BlockStart ();
};

enum {
	SXP_NOOP = 0,
	SXP_MATH,
	SXP_DATA,
	SXP_NEGATE,
	SXP_NOT,
	SXP_AND,
	SXP_OR,
	SXP_INVOKE,
	SXP_LOOKUP,
	SXP_DECLARE,
	SXP_ASSIGN,
	SXP_STATEMENT,
	SXP_IF,
	SXP_LOOP,
	SXP_SETINDEX,
	SXP_GETINDEX,
	SXP_ARRAY,
	SXP_PREOP,
	SXP_POSTOP,
	SXP_RETURN,
	SXP_BREAK,
	SXP_METHOD,
	SXP_CAST,
	SXP_FOR,
	SXP_CONTINUE,
	SXP_SMETHOD,
	SXP_YIELD,
	SXP_IN,
	SXP_NEW,
	SXP_SETPROPERTY,
	SXP_GETPROPERTY,
	SXP_FOREACH,
	SXP_STRINGCAST,
	SXP_INTCAST,
	SXP_COPY,
	SXP_CONCAT,
};

enum {
	SXP_LOOP_WHILE,
	SXP_LOOP_DOWHILE,
	SXP_LOOP_UNTIL,
	SXP_LOOP_DOUNTIL,
	SXP_LOOP_FOREVER,
};

struct CompilerBlock : public GCType::GC {
	Function* func;
	unsigned long start;
	GCType::vector<unsigned long> breaks;
	GCType::vector<unsigned long> continues;
};

struct CompilerFunction : public GCType::GC {
	Atom name;
	NameList vars;
	struct CompilerNode* body;
	Function* func;
	bool pub;
	bool staticm; // for extend methods only
};

struct CompilerExtend : public GCType::GC {
	TypeInfo* type;
	typedef GCType::vector<CompilerFunction*> MethodList;
	MethodList methods;
};

struct CompilerNode : public GCType::GC {
	int type;
	Compiler* info;
	CompilerNode* next;
	CompilerNode* inext;
	String* file;
	unsigned int line;
	struct {
		CompilerNode* nodes[4];
		Atom name;
		TypeInfo* type;
		Value value;
		int op;
	} parts;

	// create new node
	CompilerNode(Compiler* info,
		int type,
		CompilerNode* node1,
		CompilerNode* node2,
		CompilerNode* node3,
		CompilerNode* node4,
		Atom name,
		TypeInfo* type,
		Value value,
		int op);
	// append a new node to the list
	CompilerNode* Append(CompilerNode* node);
};

// compilation helpers
extern CompilerNode* sxp_transform (CompilerNode* node); // optimizer

inline
long
Compiler::BlockStart ()
{
	return blocks.front()->start;
}

} // namespace Scriptix::Compiler
} // namespace Scriptix

extern Scriptix::Compiler::Compiler* compiler;
extern FILE* yyin;
extern const char* sxp_compiler_inbuf;

// Node types
#define sxp_new_set_property(info,base,name,value) (new CompilerNode((info), SXP_SETPROPERTY, (base), (value), NULL, NULL, Atom(name), 0, Nil, 0))
#define sxp_new_get_property(info,base,name) (new CompilerNode((info), SXP_GETPROPERTY, (base), NULL, NULL, NULL, Atom(name), 0, Nil, 0))
#define sxp_new_continue(info) (new CompilerNode((info), SXP_CONTINUE, NULL, NULL, NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_break(info) (new CompilerNode((info), SXP_BREAK, NULL, NULL, NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_yield(info) (new CompilerNode((info), SXP_YIELD, NULL, NULL, NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_return(info,value) (new CompilerNode((info), SXP_RETURN, (value), NULL, NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_lookup(info,name) (new CompilerNode((info), SXP_LOOKUP, NULL, NULL, NULL, NULL, Atom(name), 0, Nil, 0))
#define sxp_new_new(info,type,args,call) (new CompilerNode((info), SXP_NEW, (args), NULL, NULL, NULL, Atom(), (type), Nil, (call)))
#define sxp_new_math(info,op,left,right) (new CompilerNode((info), SXP_MATH, (left), (right), NULL, NULL, Atom(), 0, Nil, (op)))
#define sxp_new_concat(info,left,right) (new CompilerNode((info), SXP_CONCAT, (left), (right), NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_data(info,data) (new CompilerNode((info), SXP_DATA, NULL, NULL, NULL, NULL, Atom(), 0, (data), 0))
#define sxp_new_loop(info,type,test,body) (new CompilerNode((info), SXP_LOOP, (test), (body), NULL, NULL, Atom(), 0, Nil, (type)))
#define sxp_new_if(info,test,then,els) (new CompilerNode((info), SXP_IF, (test), (then), (els), NULL, Atom(), 0, Nil, 0))
#define sxp_new_for(info,start,test,inc,body) (new CompilerNode((info), SXP_FOR, (start), (test), (inc), (body), Atom(), 0, Nil, 0))
#define sxp_new_foreach(info,name,list,body) (new CompilerNode((info), SXP_FOREACH, (list), (body), NULL, NULL, Atom(name), 0, Nil, 0))
#define sxp_new_statement(info,node) (new CompilerNode((info), SXP_STATEMENT, (node), NULL, NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_not(info,node) (new CompilerNode((info), SXP_NOT, (node), NULL, NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_array(info,node) (new CompilerNode((info), SXP_ARRAY, (node), NULL, NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_method(info,obj,name,args) (new CompilerNode((info), SXP_METHOD, (obj), (args), NULL, NULL, Atom(name), 0, Nil, 0))
#define sxp_new_smethod(info,type,name,args) (new CompilerNode((info), SXP_SMETHOD, (args), NULL, NULL, NULL, Atom(name), (type), Nil, 0))
#define sxp_new_and(info,left,right) (new CompilerNode((info), SXP_AND, (left), (right), NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_or(info,left,right) (new CompilerNode((info), SXP_OR, (left), (right), NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_in(info,node,list) (new CompilerNode((info), SXP_IN, (node), (list), NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_invoke(info,node,args) (new CompilerNode((info), SXP_INVOKE, (node), (args), NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_setindex(info,list,index,value) (new CompilerNode((info), SXP_SETINDEX, (list), (index), (value), NULL, Atom(), 0, Nil, 0))
#define sxp_new_getindex(info,list,index) (new CompilerNode((info), SXP_GETINDEX, (list), (index), NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_declare(info,name,value) (new CompilerNode((info), SXP_DECLARE, (value), NULL, NULL, NULL, Atom(name), 0, Nil, 0))
#define sxp_new_assign(info,name,value) (new CompilerNode((info), SXP_ASSIGN, (value), NULL, NULL, NULL, Atom(name), 0, Nil, 0))
#define sxp_new_cast(info,type,node) (new CompilerNode((info), SXP_CAST, (node), NULL, NULL, NULL, Atom(), (type), Nil, 0))
#define sxp_new_preop(info,expr,op,value) (new CompilerNode((info), SXP_PREOP, (expr), (value), NULL, NULL, Atom(), 0, Nil, (op)))
#define sxp_new_postop(info,expr,op,value) (new CompilerNode((info), SXP_POSTOP, (expr), (value), NULL, NULL, Atom(), 0, Nil, (op)))
#define sxp_new_negate(info,node) (new CompilerNode((info), SXP_NEGATE, (node), NULL, NULL, NULL, Atom(), 0, Nil, 0))
#define sxp_new_copy(info,loc) (new CompilerNode((info), SXP_COPY, NULL, NULL, NULL, NULL, Atom(), 0, Nil, (loc)))

#endif
