/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef BINDINGS_H
#define BINDINGS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "scriptix/scriptix.h"
#include "common/gcbase.h"
#include "common/gcmap.h"
#include "common/awestr.h"
#include "common/imanager.h"

// initialize bindings
class SScriptBindings : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();

	void bind ();
};
extern SScriptBindings ScriptBindings;

// scriptable processor
class ScriptProcessor : public Scriptix::Native
{
	private:
	Scriptix::Value s_prompt;
	Scriptix::ScriptFunction i_init;
	Scriptix::ScriptFunction i_close;
	Scriptix::ScriptFunction i_update;

	public:
	ScriptProcessor(const Scriptix::TypeInfo* s_type) : Scriptix::Native(s_type), s_prompt() {}

	inline void set_prompt(StringArg string) { s_prompt = Scriptix::Value(string); }

	friend class ScriptProcessorWrapper;
};

#endif
