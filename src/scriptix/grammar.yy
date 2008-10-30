/*
 * Scriptix - Lite-weight scripting interface
 * Copyright (c) 2002, Sean Middleditch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR ORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

%{
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include <errno.h>
	#include <gc/gc.h>

	#include <iostream>

	#include "scriptix/system.h"
	#include "scriptix/compiler.h"
	#include "scriptix/number.h"
	#include "scriptix/vimpl.h"
	
	using namespace Scriptix;
	using namespace Scriptix::Compiler;

	#define COMP_STACK_SIZE 20
	#define NAME_LIST_SIZE 20

	int yyerror (const char *);
	extern int yylex (void);
	int yyparse (void);

	Scriptix::Compiler::Compiler* compiler = NULL;

	#define YYERROR_VERBOSE 1
	#define SXERROR_VERBOSE 1

	/* stupid BISON fix */
	#define __attribute__(x)

	#define malloc(size) GC_MALLOC(size)
	#define realloc(ptr,size) GC_REALLOC(ptr,size)
	#define free(ptr)
%}

%union {
	CompilerNode* node;
	IValue* value;
	TypeInfo* type;
	size_t id;
	NameList* names;
}

%token<value> NUMBER
%token<value> STRING
%token<id> IDENTIFIER TSTREAMOP
%token<type> TYPE
%token IF "if"
%token ELSE "else"
%token WHILE "while"
%token DO "do"
%token AND "&&"
%token OR "||"
%token TGTE ">="
%token TLTE "<="
%token TNE "!="
%token TNIL "nil"
%token TIN "in"
%token TFOR "for"
%token TCONTINUE "continue"
%token TYIELD "yield"
%token TPUBLIC "public"
%token TVAR "var"
%token TTHEN "then"
%token TELIF "elif"
%token TEND "end"
%token TDEREFERENCE "."
%token TCONCAT ".."
%token TBREAK "break"
%token TRETURN "return"
%token TFUNCTION "function"
%token TSTREAM "<<"

%nonassoc TBREAK TRETURN 
%right '=' TADDASSIGN TSUBASSIGN TMULASSIGN TDIVASSIGN
%left AND OR
%left '>' '<' TGTE TLTE TIN
%left TNE TEQUALS
%left '+' '-' TCONCAT
%left '*' '/'
%nonassoc WHILE
%nonassoc '!' CUNARY
%nonassoc TINCREMENT TDECREMENT
%left TCAST
%left TDEREFERENCE ':' '['
%left '('

%nonassoc IF
%nonassoc ELSE

%type<node> args block stmt expr func_args elif body ctrl
%type<node> stream stream_item rval assign call declare
%type<names> arg_names_list arg_names
%type<value> data
%type<id> name
%type<type> type

%%
program:
	| program function
	| program global
	| program error
	;

function: TFUNCTION name '(' arg_names ')' body { compiler->add_func(Atom::create($2), ($4 ? *$4 : NameList()), $6, false); }
	| TPUBLIC TFUNCTION name '(' arg_names ')' body { compiler->add_func(Atom::create($3), ($5 ? *$5 : NameList()), $7, true); }
	;

global: TVAR name '=' data ';' { compiler->set_global(Atom::create($2), $4); }
	;

body: TEND { $$ = NULL; }
	| block TEND { $$ = $1; }
	;

block: stmt ';' { $$ = $1; }
	| ctrl { $$ = $1; }
	| block stmt ';' { if ($1 != NULL) { $$ = $1; $$->Append($2); } else { $$ = $2; } }
	| block ctrl { if ($1 != NULL) { $$ = $1; $$->Append($2); } else { $$ = $2; } }
	;

stmt: assign { $$ = sxp_new_statement(compiler, $1); }
	| declare { $$ = sxp_new_statement(compiler, $1); }
	| call { $$ = sxp_new_statement(compiler, $1); }

	| TRETURN expr { $$ = sxp_new_return (compiler, $2); }
	| TRETURN { $$ = sxp_new_return (compiler, NULL); }
	| TBREAK { $$ = sxp_new_break (compiler); }
	| TCONTINUE { $$ = sxp_new_continue (compiler); }
	| TYIELD { $$ = sxp_new_yield (compiler); }

	| expr stream { $$ = sxp_new_stream(compiler, $1, $2); }

	| error { $$ = NULL; }
	;

ctrl: IF expr TTHEN block TEND { $$ = sxp_new_if (compiler, $2, $4, NULL); }
	| IF expr TTHEN block elif TEND { $$ = sxp_new_if (compiler, $2, $4, $5); }
	| IF expr TTHEN block ELSE block TEND { $$ = sxp_new_if (compiler, $2, $4, $6); }
	| IF expr TTHEN block elif ELSE block TEND { $$ = sxp_new_if (compiler, $2, $4, $5); $5->parts.nodes[2] = $7; }
	| WHILE expr DO block TEND { $$ = sxp_new_while (compiler, $2, $4); }
	| WHILE declare DO block TEND { $$ = sxp_new_while (compiler, $2, $4); }
	
	| TFOR name ':' expr DO block TEND { $$ = sxp_new_forrange (compiler, Atom::create($2), sxp_new_data(compiler, Value(0)), $4, $6); }
	| TFOR name ':' expr ',' expr DO block TEND { $$ = sxp_new_forrange (compiler, Atom::create($2), $4, $6, $8); }
	| TFOR name TIN expr DO block TEND { $$ = sxp_new_foreach (compiler, Atom::create($2), $4, $6); }

	| error { $$ = NULL; }
	;

elif: TELIF expr TTHEN block { $$ = sxp_new_if (compiler, $2, $4, NULL); }
	| elif TELIF expr TTHEN block { $$ = $1; $1->parts.nodes[2] = sxp_new_if (compiler, $3, $5, NULL); }
	;

stream: stream_item { $$ = $1; }
	| stream stream_item { $$ = $1; $1->Append($2); }
	;

stream_item: TSTREAM expr { $$ = sxp_new_streamitem(compiler, $2); }
	| TSTREAM TSTREAMOP '(' func_args ')' { $$ = sxp_new_streamop(compiler, sxp_new_lookup(compiler, Atom::create($2)), $4); }
	;

args:	expr { $$ = $1; }
	| args ',' expr { $$ = $1; $$->Append($3); }
	;

arg_names: { $$ = NULL; }
	| arg_names_list { $$ = $1; }
	;

arg_names_list: name { $$ = new (UseGC) NameList(); $$->push_back(Atom::create($1)); }
	| arg_names_list ',' name { $$->push_back(Atom::create($3)); }
	;

func_args: args { $$ = $1; }
	| { $$ = NULL; }
	;

/*
lval: name { $$ = sxp_new_lookup(compiler, Atom::create($1)); }
	| expr '[' expr ']' { $$ = sxp_new_getindex(compiler, $1, $3); }
	| expr TDEREFERENCE name { $$ = sxp_new_get_property(compiler, $1, Atom::create($3)); }
	;
*/

rval: name { $$ = sxp_new_lookup(compiler, Atom::create($1)); }
	| expr '[' expr ']' { $$ = sxp_new_getindex(compiler, $1, $3); }
	| expr TDEREFERENCE name { $$ = sxp_new_get_property(compiler, $1, Atom::create($3)); }
	| data { $$ = sxp_new_data (compiler, $1); }
	;

declare: TVAR name '=' expr { $$ = sxp_new_declare (compiler, Atom::create($2), $4); }
/*	| TVAR name { $$ = sxp_new_declare (compiler, Atom::create($2), NULL); } */
	;

assign: name '=' expr { $$ = sxp_new_assign (compiler, Atom::create($1), $3); }
	| expr '[' expr ']' '=' expr %prec '=' { $$ = sxp_new_setindex (compiler, $1, $3, $6); }
	| expr TDEREFERENCE name '=' expr { $$ = sxp_new_set_property(compiler, $1, Atom::create($3), $5); }
	;

call: name '(' func_args ')' { $$ = sxp_new_invoke (compiler, sxp_new_lookup(compiler, Atom::create($1)), $3); }
	| '(' expr ')' '(' func_args ')' { $$ = sxp_new_invoke (compiler, $2, $5); }
	| expr TDEREFERENCE name '(' func_args ')' { $$ = sxp_new_method (compiler, $1, Atom::create($3), $5); }
	;

expr: expr '+' expr { $$ = sxp_new_math (compiler, OP_ADD, $1, $3); }
	| expr '-' expr { $$ = sxp_new_math (compiler, OP_SUBTRACT, $1, $3); }
	| expr '*' expr { $$ = sxp_new_math (compiler, OP_MULTIPLY, $1, $3); }
	| expr '/' expr { $$ = sxp_new_math (compiler, OP_DIVIDE, $1, $3); }
	| expr TCONCAT expr { $$ = sxp_new_concat (compiler, $1, $3); }
	| '(' expr ')' { $$ = $2; }

	| expr TIN expr { $$ = sxp_new_in (compiler, $1, $3); }

	| '-' expr %prec CUNARY {
			if ($2->type == SXP_DATA && Value($2->parts.value).is_int())
				$$ = sxp_new_data(compiler,Value(-Number::to_int($2->parts.value)));
			else
				$$ = sxp_new_negate (compiler, $2); 
		}

	| '!' expr { $$ = sxp_new_not (compiler, $2); }
	| expr AND expr { $$ = sxp_new_and (compiler, $1, $3); }
	| expr OR expr { $$ = sxp_new_or (compiler, $1, $3); }

	| expr '>' expr { $$ = sxp_new_math (compiler, OP_GT, $1, $3); }
	| expr '<' expr { $$ = sxp_new_math (compiler, OP_LT, $1, $3); }
	| expr TNE expr { $$ = sxp_new_math (compiler, OP_NEQUAL, $1, $3); }
	| expr TGTE expr { $$ = sxp_new_math (compiler, OP_GTE, $1, $3); }
	| expr TLTE expr { $$ = sxp_new_math (compiler, OP_LTE, $1, $3); }
	| expr TEQUALS expr { $$ = sxp_new_math (compiler, OP_EQUAL, $1, $3); }

	| assign { $$ = $1; }

	| call { $$ = $1; }

	| type '(' expr ')' %prec TCAST { $$ = sxp_new_cast (compiler, $1, $3); }

	| '[' args ']' { $$ = sxp_new_array (compiler, $2); }
	| '[' ']' { $$ = sxp_new_array (compiler, NULL); }

	| rval { $$ = $1; }
	;

	
data:	NUMBER { $$ = $1;  }
	| STRING { $$ = $1; }
	| TNIL { $$ = NULL; }
	| type { $$ = new TypeValue($1); }
	;

name:	IDENTIFIER { $$ = $1; }
	;

type:	TYPE { $$ = $1; }
	;

%%

int
yyerror (const char *str) {
	compiler->Error(String(str));
	return 1;
}

extern "C"
int
yywrap (void) {
	return 1;
}

int
Scriptix::SScriptManager::load_file(String file, CompilerHandler* handler) {
	int ret;

	if (file.empty()) {
		yyin = stdin;
	} else {
		yyin = fopen (file.c_str(), "rt");
		if (yyin == NULL) {
			std::cerr << "Could not open '" << file.c_str() << "'" << std::endl;
			return SXE_INVALID;
		}
	}

	compiler = new Scriptix::Compiler::Compiler();
	if (compiler == NULL) {
		if (!file.empty())
			fclose (yyin);
		std::cerr << "Failed to create Compiler context" << std::endl;
		return SXE_INTERNAL;
	}
	if (!file.empty())
		compiler->set_file(file);
	else
		compiler->set_file(S("<stdin>"));
	compiler->set_handler(handler);

	sxp_compiler_inbuf = NULL;

	ret = yyparse ();
	if (yynerrs > 0)
		ret = SXE_INVALID;

	if (!file.empty())
		fclose (yyin);

	if (!ret)
		ret = compiler->Compile();

	return ret;
}

int
Scriptix::SScriptManager::load_string(String buf, String name, size_t lineno, CompilerHandler* handler) {
	int ret;

	if (buf.empty())
		return SXE_INVALID;

	compiler = new Scriptix::Compiler::Compiler();
	if (compiler == NULL) {
		std::cerr << "Failed to create Compiler context" << std::endl;
		return SXE_INTERNAL;
	}
	compiler->set_file(name);
	compiler->set_line(lineno);
	compiler->set_handler(handler);

	yyin = NULL;
	sxp_compiler_inbuf = buf.c_str();

	ret = yyparse ();
	if (yynerrs > 0)
		ret = SXE_INVALID;

	if (!ret)
		ret = compiler->Compile();

	return ret;
}
