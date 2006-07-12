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

#include <stdio.h>

#include "scriptix/system.h"
#include "scriptix/compiler.h"
#include "scriptix/number.h"
#include "scriptix/vimpl.h"

using namespace Scriptix;
using namespace Scriptix::Compiler;

static void sxp_transform_node (CompilerNode** node);

static
void
sxp_do_transform (CompilerNode** node_ptr)
{
	CompilerNode* node = *node_ptr;
	if (node == NULL)
		return;

	// part transformations
	sxp_transform_node(&node->parts.nodes[0]);
	sxp_transform_node(&node->parts.nodes[1]);
	sxp_transform_node(&node->parts.nodes[2]);
	sxp_transform_node(&node->parts.nodes[3]);

	switch (node->type) {
		/* pre-perform math */
		case SXP_MATH:
			// numeric math
			if (node->parts.nodes[0]->type == SXP_DATA &&
					node->parts.nodes[0]->parts.value.is_int() &&
					node->parts.nodes[1]->type == SXP_DATA &&
					node->parts.nodes[1]->parts.value.is_int()) {
				int left = Number::to_int(node->parts.nodes[0]->parts.value);
				int right = Number::to_int(node->parts.nodes[1]->parts.value);
				switch (node->parts.op) {
					case OP_ADD:
						node->type = SXP_DATA;
						node->parts.value = Value (left + right);
						break;
					case OP_SUBTRACT:
						node->type = SXP_DATA;
						node->parts.value = Value (left - right);
						break;
					case OP_MULTIPLY:
						node->type = SXP_DATA;
						node->parts.value = Value (left * right);
						break;
					case OP_DIVIDE:
						if (right == 0) {
							node->info->Error(S("Division by zero"));
						} else {
							*node_ptr = sxp_new_data (node->info, Value (left / right));
							node->type = SXP_MATH;
							node->parts.value = Value (left / right);
						}
						break;
					default:
						// do nothing - FIXME
						break;
				}
			}
			// string concat
			if (node->parts.op == OP_ADD &&
					node->parts.nodes[0]->type == SXP_DATA &&
					node->parts.nodes[0]->parts.value.is_string() &&
					node->parts.nodes[1]->type == SXP_DATA &&
					node->parts.nodes[1]->parts.value.is_string()) {
				node->type = SXP_DATA;
				node->parts.value = String(node->parts.nodes[0]->parts.value.get_string() + node->parts.nodes[1]->parts.value.get_string());
			}
			break;
		/* string concatenation */
		case SXP_CONCAT:
			if (node->parts.nodes[0]->type == SXP_DATA &&
					node->parts.nodes[0]->parts.value.is_string() &&
					node->parts.nodes[1]->type == SXP_DATA &&
					node->parts.nodes[1]->parts.value.is_string()) {
				node->type = SXP_DATA;
				node->parts.value = String(node->parts.nodes[0]->parts.value.get_string() + node->parts.nodes[1]->parts.value.get_string());
			}
			break;
		/* breaks, returns, and continues have nothing afterwards */
		case SXP_BREAK:
		case SXP_RETURN:
		case SXP_CONTINUE:
			/* remove next pointer */
			node->next = NULL;
			break;
		/* if - turn into a block if true, or cut if false */
		case SXP_IF:
			if (node->parts.nodes[0]->type == SXP_DATA) {
				CompilerNode* block = NULL;
				/* is always true */
				if (node->parts.nodes[0]->parts.value.is_true()) {
					block = node->parts.nodes[1];
				} else {
					block = node->parts.nodes[2];
				}
				if (block != NULL) {
					/* append next to block list, update current */
					*node_ptr = block->Append(node->next);
				} else {
					node->type = SXP_NOOP;
				}
			}
			break;
		/* while/until/do loops - test is true, perma loop, false, noop */
		case SXP_LOOP:
			if (node->parts.op != SXP_LOOP_FOREVER) {
				if (node->parts.nodes[0]->type == SXP_DATA) {
					/* is always true */
					if (node->parts.nodes[0]->parts.value.is_true()) {
						node->parts.op = SXP_LOOP_FOREVER;
					/* always false */
					} else {
						node->type = SXP_NOOP;
					}
				}
			}
			break;
		/* specialize casts */
		case SXP_CAST:
			if (node->parts.type == ScriptManager.get_string_type()) {
				node->type = SXP_STRINGCAST;
				if (node->parts.nodes[0]->type == SXP_DATA) {
					if (!(node->parts.value = node->parts.nodes[0]->parts.value.to_string()).is_nil())
						node->type = SXP_DATA;
				}
			} else if (node->parts.type == ScriptManager.get_number_type()) {
				node->type = SXP_INTCAST;
				if (node->parts.nodes[0]->type == SXP_DATA) {
					if (!(node->parts.value = node->parts.nodes[0]->parts.value.to_int()).is_nil())
						node->type = SXP_DATA;
				}
			}
			break;
		default:
			/* do nothing */
			break;
	}
}

static
void
sxp_transform_node (CompilerNode** node)
{
	CompilerNode* ret = sxp_transform(*node);
	*node = ret;
}

CompilerNode*
Scriptix::Compiler::sxp_transform (CompilerNode* node)
{
	CompilerNode* last = NULL;
	CompilerNode* ret = NULL;

	while (node != NULL) {
		/* do transform */
		sxp_do_transform (&node);

		/* if the last's next is not the current node
		 * (i.e., it changed), update the last next pointer to the
		 * new node */
		if (last != NULL && last->next != node)
			last->next = node;

		/* if we have no return pointer, update it */
		if (ret == NULL)
			ret = node;

		/* looping update */
		last = node;
		node = node->next;
	}

	return ret;
}
