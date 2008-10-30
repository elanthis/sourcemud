/*
 * Scriptix - Lite-weight scripting interface
 * Copyright (c) 2002, 2003, 2004, 2005  Sean Middleditch
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "scriptix/system.h"
#include "scriptix/compiler.h"

using namespace Scriptix;
using namespace Scriptix::Compiler;

void
Scriptix::Compiler::Compiler::Error(String msg)
{
	ScriptManager.handle_error (file ? file : S("<input>"), line, msg);
}

Scriptix::Compiler::Compiler::Compiler() : funcs(), returns(), blocks(), gnames()
{
	nodes = NULL;
	file = S("<input>");
	last_line = 0;
	last_op = 0;
	line = 1;
	globals = NULL;
	handler = NULL;
}

CompilerFunction *
Scriptix::Compiler::Compiler::add_func(Atom name, const NameList& args, CompilerNode* body, bool pub)
{
	CompilerFunction *func = new CompilerFunction();
	if (!func)
		return NULL;

	func->name = name;
	func->vars = args; /* arguments become variables... */
	func->body = body;
	func->pub = pub;

	funcs.push_back(func);

	return func;
}

// add block to state
bool
Scriptix::Compiler::Compiler::push_block (Function* func)
{
	CompilerBlock *block = new CompilerBlock;
	if (!block)
		return false;

	// data
	block->func = func;
	block->start = func->count;

	blocks.push_back(block);

	return true;
}

// pop block from state
void
Scriptix::Compiler::Compiler::pop_block (void)
{
	if (blocks.empty())
		return;

	CompilerBlock* block = blocks.back();

	// breaks
	while (!block->breaks.empty()) {
		block->func->nodes[block->breaks.front()] = (long)block->func->count - block->breaks.front();
		block->breaks.erase(block->breaks.begin());
	}

	// continues
	while (!block->continues.empty()) {
		block->func->nodes[block->continues.front()] = (long)block->start - block->continues.front();
		block->continues.erase(block->continues.begin());
	}

	blocks.pop_back();
}

// add a break command
bool
Scriptix::Compiler::Compiler::add_break (void)
{
	if (blocks.empty()) {
		Error(S("Break outside of loop"));
		return false;
	}
	
	blocks.back()->func->add_opcode(OP_JUMP);
	blocks.back()->breaks.push_back(blocks.back()->func->count);
	blocks.back()->func->add_oparg(0);

	return true;
}

// add a break command when test is true
bool
Scriptix::Compiler::Compiler::add_breakOnTrue (void)
{
	if (blocks.empty()) {
		Error(S("Break outside of loop"));
		return false;
	}
	
	blocks.back()->func->add_opcode(OP_TJUMP);
	blocks.back()->breaks.push_back(blocks.back()->func->count);
	blocks.back()->func->add_oparg(0);

	return true;
}

// add a break command when test is fasle
bool
Scriptix::Compiler::Compiler::add_breakOnFalse (void)
{
	if (blocks.empty()) {
		Error(S("Break outside of loop"));
		return false;
	}
	
	blocks.back()->func->add_opcode(OP_FJUMP);
	blocks.back()->breaks.push_back(blocks.back()->func->count);
	blocks.back()->func->add_oparg(0);

	return true;
}

// add a contineu command
bool
Scriptix::Compiler::Compiler::add_continue (void)
{
	if (blocks.empty()) {
		Error(S("Continue outside of loop"));
		return false;
	}

	blocks.back()->func->add_opcode(OP_JUMP);
	blocks.back()->func->add_oparg((long)blocks.back()->start - blocks.back()->func->count);

	return true;
}

long
Scriptix::Compiler::Compiler::add_var(CompilerFunction* func, Atom id)
{
	func->vars.push_back(id);
	return func->vars.size() - 1;
}

long
Scriptix::Compiler::Compiler::get_var(CompilerFunction* func, Atom id)
{
	if (func->vars.empty()) {
		return -1;
	} else {
		size_t index = 0;
		while (index < func->vars.size()) {
			if (func->vars[index] == id)
				return index;
			++ index;
		}
		return -1;
	}
}

void
Scriptix::Compiler::Compiler::set_global(Atom id, Value value)
{
	if (get_global(id) >= 0) {
		Error(S("Redefined global"));
		return;
	}

	if (globals == NULL) {
		globals = new Array(1, &value);
	} else {
		Array::append(globals, value);
	}
	gnames.push_back(id);
}

long
Scriptix::Compiler::Compiler::get_global(Atom id)
{
	size_t index;
	for (index = 0; index < gnames.size(); ++ index) {
		if (gnames[index] == id) {
			return index;
		}
	}
	return -1;
}

Scriptix::Compiler::CompilerNode::CompilerNode(Scriptix::Compiler::Compiler* s_info,
		int s_type,
		CompilerNode* s_node0,
		CompilerNode* s_node1,
		CompilerNode* s_node2,
		CompilerNode* s_node3,
		Atom s_name,
		TypeInfo* s_ttype,
		Value s_value,
		int s_op)
{
	info = s_info;
	type = s_type;
	next = NULL;

	file = info->file;
	line = info->line;

	inext = info->nodes;
	info->nodes = this;

	parts.nodes[0] = s_node0;
	parts.nodes[1] = s_node1;
	parts.nodes[2] = s_node2;
	parts.nodes[3] = s_node3;
	parts.name = s_name;
	parts.type = s_ttype;
	parts.value = s_value;
	parts.op = s_op;
}

CompilerNode* 
Scriptix::Compiler::CompilerNode::Append(CompilerNode* node) {
	CompilerNode* i;

	for (i = this; i->next != NULL; i = i->next)
		;
	i->next = node;

	return this;
}
