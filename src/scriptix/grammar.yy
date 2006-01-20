/*
 * Scriptix - Lite-weight scripting interface
 * Copyright (c) 2002, AwesomePlay Productions, Inc.
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

	#include "scriptix/scriptix.h"
	#include "scriptix/system.h"
	#include "scriptix/compiler.h"
	
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
%token<id> IDENTIFIER
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
%token TFOREACH "foreach"
%token TADDASSIGN "+="
%token TSUBASSIGN "-="
%token TINCREMENT "++"
%token TDECREMENT "--"
%token TNEW "new"
%token TUNTIL "until"
%token TNIL "nil"
%token TIN "in"
%token TFOR "for"
%token TCONTINUE "continue"
%token TYIELD "yield"
%token TPUBLIC "public"
%token TMULASSIGN "*="
%token TDIVASSIGN "/="
%token TVAR "var"
%token TDEREFERENCE "."
%token TCONCAT "::"
%token TBREAK "break"
%token TRETURN "return"
%token TFUNCTION "function"

%nonassoc TBREAK TRETURN 
%right '=' TADDASSIGN TSUBASSIGN TMULASSIGN TDIVASSIGN
%left AND OR
%left '>' '<' TGTE TLTE TIN
%left TNE TEQUALS
%left '+' '-' TCONCAT
%left '*' '/'
%nonassoc WHILE TUNTIL DO
%nonassoc '!' CUNARY
%nonassoc TINCREMENT TDECREMENT
%left TCAST
%left TDEREFERENCE ':' '[' TNEW
%left '('

%nonassoc IF
%nonassoc ELSE

%type<node> node args block stmts stmt expr func_args lval
%type<names> arg_names_list arg_names
%type<value> data
%type<id> name
%type<type> type

%%
program:
	| program function
	| program global
	| program error
	| program new
	;

function: TFUNCTION name '(' arg_names ')' '{' block '}' { compiler->add_func(Atom::create($2), ($4 ? *$4 : NameList()), $7, false); }
	| TPUBLIC TFUNCTION name '(' arg_names ')' '{' block '}' { compiler->add_func(Atom::create($3), ($5 ? *$5 : NameList()), $8, true); }
	;

global: TVAR name '=' data ';' { compiler->set_global(Atom::create($2), $4); }
	| TVAR name ';' { compiler->set_global(Atom::create($2), Nil); }
	;

method: name '(' arg_names ')' '{' block '}' { compiler->add_method(Atom::create($1), ($3 ? *$3 : NameList()), $6); }
	| TNEW '(' arg_names ')' '{' block '}' { compiler->add_method(Atom("new"), ($3 ? *$3 : NameList()), $6); }
	;

methods: method
	| methods method
	;

new: TNEW name { if (!compiler->add_type (Atom::create($2), ScriptManager.get_script_class_type())) YYERROR; } '{' methods '}'
	| TNEW name ':' type { if (!compiler->add_type (Atom::create($2), $4)) YYERROR; } '{' methods '}'
	;

block: { $$ = NULL; }
	| stmts { $$ = $1; }
	;

stmts:	stmt { $$ = $1; }
	| stmts stmt { if ($1 != NULL) { $$ = $1; $$->Append($2); } else { $$ = $2; } }
	;

stmt:	node ';' { $$ = $1; }
	| TRETURN expr ';' { $$ = sxp_new_return (compiler, $2); }
	| TRETURN ';' { $$ = sxp_new_return (compiler, NULL); }
	| TBREAK ';' { $$ = sxp_new_break (compiler); }
	| TCONTINUE ';' { $$ = sxp_new_continue (compiler); }
	| TYIELD ';' { $$ = sxp_new_yield (compiler); }

	| IF '(' expr ')' stmt %prec IF { $$ = sxp_new_if (compiler, $3, $5, NULL); }
	| IF '(' expr ')' stmt ELSE stmt %prec ELSE { $$ = sxp_new_if (compiler, $3, $5, $7); }
	| WHILE '(' expr ')' stmt { $$ = sxp_new_loop (compiler, SXP_LOOP_WHILE, $3, $5); }
	| TUNTIL '(' expr ')' stmt { $$ = sxp_new_loop (compiler, SXP_LOOP_UNTIL, $3, $5); }
	| DO stmt WHILE '(' expr ')' ';' { $$ = sxp_new_loop (compiler, SXP_LOOP_DOWHILE, $5, $2); }
	| DO stmt TUNTIL '(' expr ')' ';' { $$ = sxp_new_loop (compiler, SXP_LOOP_DOUNTIL, $5, $2); }
	| DO stmt { $$ = sxp_new_loop (compiler, SXP_LOOP_FOREVER, NULL, $2); }
	
	| TFOR '(' node ';' expr ';' node ')' stmt { $$ = sxp_new_for (compiler, $3, $5, $7, $9); }
/*	| TFOREACH '(' name TIN expr ')' stmt { $$ = sxp_new_foreach (compiler, $3, $5, $7); } */
	| TFOREACH '(' TVAR name TIN expr ')' stmt { $$ = sxp_new_foreach (compiler, Atom::create($4), $6, $8); }

	| '{' block '}' { $$ = $2; }

	| error { $$ = NULL; }
	;

node:	{ $$ = NULL; }
	| expr { $$ = sxp_new_statement (compiler, $1); }
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

lval: name { $$ = sxp_new_lookup(compiler, Atom::create($1)); }
	| expr '[' expr ']' { $$ = sxp_new_getindex(compiler, $1, $3); }
	| expr TDEREFERENCE name { $$ = sxp_new_get_property(compiler, $1, Atom::create($3)); }
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

	| TVAR name { $$ = sxp_new_declare (compiler, Atom::create($2), NULL); }
	| TVAR name '=' expr { $$ = sxp_new_declare (compiler, Atom::create($2), $4); }
	| name '=' expr { $$ = sxp_new_assign (compiler, Atom::create($1), $3); }
	| expr '[' expr ']' '=' expr %prec '=' { $$ = sxp_new_setindex (compiler, $1, $3, $6); }
	| expr TDEREFERENCE name '=' expr { $$ = sxp_new_set_property(compiler, $1, Atom::create($3), $5); }

	| lval TADDASSIGN expr { $$ = sxp_new_preop (compiler, $1, OP_ADD, $3); }
	| lval TSUBASSIGN expr { $$ = sxp_new_preop (compiler, $1, OP_SUBTRACT, $3); }
	| lval TMULASSIGN expr { $$ = sxp_new_preop (compiler, $1, OP_MULTIPLY, $3); }
	| lval TDIVASSIGN expr { $$ = sxp_new_preop (compiler, $1, OP_DIVIDE, $3); }
	| lval TINCREMENT { $$ = sxp_new_postop (compiler, $1, OP_ADD, sxp_new_data (compiler, Value (1))); }
	| lval TDECREMENT { $$ = sxp_new_postop (compiler, $1, OP_SUBTRACT, sxp_new_data (compiler, Value (1))); }

	// FIXME: problem lines: reduce/reduce errors, can't figure out why or out to fix
	| TINCREMENT lval { $$ = sxp_new_preop (compiler, $2, OP_ADD, sxp_new_data (compiler, Value (1))); }
	| TDECREMENT lval { $$ = sxp_new_preop (compiler, $2, OP_SUBTRACT, sxp_new_data (compiler, Value (1))); }
	
	| type '(' expr ')' %prec TCAST { $$ = sxp_new_cast (compiler, $1, $3); }

	| name '(' func_args ')' { $$ = sxp_new_invoke (compiler, sxp_new_lookup(compiler, Atom::create($1)), $3); }
	| '(' expr ')' '(' func_args ')' { $$ = sxp_new_invoke (compiler, $2, $5); }

	| TNEW type { $$ = sxp_new_new (compiler, $2, NULL, false); }
	| TNEW type '(' func_args ')' { $$ = sxp_new_new (compiler, $2, $4, true); }
	| expr TDEREFERENCE name '(' func_args ')' { $$ = sxp_new_method (compiler, $1, Atom::create($3), $5); }

	| '[' args ']' { $$ = sxp_new_array (compiler, $2); }
	| '[' ']' { $$ = sxp_new_array (compiler, NULL); }

	| data { $$ = sxp_new_data (compiler, $1); }

	| lval { $$ = $1; }
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
	compiler->Error(str);
	return 1;
}

extern "C"
int
yywrap (void) {
	return 1;
}

int
Scriptix::SScriptManager::load_file(const BaseString& file, CompilerHandler* handler) {
	int ret;

	if (file.empty()) {
		yyin = stdin;
	} else {
		yyin = fopen (file.c_str(), "rt");
		if (yyin == NULL) {
			std::cerr << "Could not open '" << file << "'" << std::endl;
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
		compiler->set_file("<stdin>");
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
Scriptix::SScriptManager::load_string(const BaseString& buf, const BaseString& name, size_t lineno, CompilerHandler* handler) {
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
