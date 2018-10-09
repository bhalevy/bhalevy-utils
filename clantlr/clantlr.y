/*
 * Copyright (c) 2018 Benny Halevy. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Any commercial redistribution and use in source and binary forms,
 *    with or without modification, are prohibited without specific prior
 *    written permission from the author.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BENNY HALEVY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BENNY HALEVY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

%{

#include <stdlib.h>

#include "intern.h"

int yydebug;

%}

%union {
	char *text;
}

%token TOK_CHAR_LITERAL
%token TOK_STRING_LITERAL
%token TOK_FRAGMENT
%token TOK_RETURNS
%token TOK_OPTIONS
%token TOK_PREFIX
%token TOK_NAME
%token TOK_PARAMS
%token TOK_CODE

%left '|'
%right REGEX_OP

%%

prog
	: grammar_stat
	  opt_prog_code
	  stats
	;

grammar_stat
    : grammar_stat_list
      ';'                                   { printf(";"); }
    ;

grammar_stat_list
    : grammar_stat_name 
    | grammar_stat_list grammar_stat_name
    ;

grammar_stat_name
    : TOK_NAME                              { printf("%s%s", yytext[-1] == '\n' ? "" : " ", yytext); }
    ;

opt_prog_code
    : prog_code
    |
    ;

prog_code
    : prog_code_stat
    | prog_code prog_code_stat
    ;

prog_code_stat
    : prog_code_prefix                      { printf("%s", yytext); }
      TOK_CODE                              { printf(" {}"); }
    ;

prog_code_prefix
    : TOK_PREFIX
    | TOK_OPTIONS
    ;

stats
    : stat
    | stats stat
    ;

stat
	: stat_hdr
	  opt_prefixed_code_block
      ':'                                   { printf(":"); }
      stat_expr
	  stat_alternatives
	  ';'                                   { printf(";"); }
	;

stat_hdr
    : opt_fragment
      TOK_NAME                              { printf("%s", yytext); }
      opt_params
      opt_returns
    ;

opt_fragment
    : TOK_FRAGMENT                          { printf("fragment "); }
    |
    ;

opt_params
    : params
    |
    ;

params
    : TOK_PARAMS                            { printf(" []"); }
    ;

opt_returns
    : TOK_RETURNS                           { printf(" returns"); }
      params
    |
    ;

opt_prefixed_code_block
    : prefixed_code_block
    |
    ;

prefixed_code_block
    : prefixed_code
    | prefixed_code_block prefixed_code
    ;

prefixed_code
    : TOK_PREFIX                            { printf("%s", yytext); }
      TOK_CODE                              { printf(" {}"); }
    ;

stat_alternatives
    : stat_alt_list
    |
    ;

stat_alt_list
    : stat_alt
    | stat_alt_list stat_alt
    ;

stat_alt
    : '|'                                   { printf("|"); }
      stat_expr
    ;

stat_expr
    : expr
    |
    ;

expr
    : expr_token
    | expr expr_token
    ;

expr_token
    : token
    | '('                                   { printf(" ("); }
      expr_list
      ')'                                   { printf(" )"); }
    | expr_token regex_op   %prec REGEX_OP  { printf("%s", yytext); }
    | TOK_CODE                              { printf(" {}"); }
    ;

expr_list
    : expr
    | expr_list
      '|'                                   { printf(" |"); }
      expr
    ;

token
    : TOK_CHAR_LITERAL                      { printf(" %s", yytext); }
    | lvalue
    | lvalue
      '='                                   { printf("="); }
      rvalue
    ;

lvalue
    : TOK_NAME                              { printf(" %s", yytext); }
      opt_params
    ;

rvalue
    : TOK_NAME                              { printf("%s", yytext); }
      opt_params
    | '('                                   { printf("("); }
      expr_list
      ')'                                   { printf(")"); }
    ;

regex_op
    : '?'
    | '*'
    | '+'
    ;

%%

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

int ic_errors;

int
yyerror(char *fmt, ...)
{
	va_list args;
	char buf[256+1];

	ic_errors++;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, args);
	va_end(args);

	return puts(buf);
}

static char **yyfiles;

int
yywrap(void)
{
	extern FILE *yyin;

	if (yyfiles == NULL || *yyfiles == NULL)
		return 1;

	yyin = fopen(*yyfiles, "r");
	if (yyin == NULL) {
		perror(*yyfiles);
		ic_errors++;
		return -1;
	}
	yyfiles++;
	return 0;
}

int
main(int argc, char **argv)
{
    int ch;

    while ((ch = getopt(argc, argv, "d")) != -1) {
        switch (ch) {
        case 'd':
            yydebug++;
            break;
        }
    }
 
	argc -= optind;
	argv += optind;

	if (*argv) {
		yyfiles = argv;
		yywrap();
	}

	yyparse();

	return ic_errors != 0;
}
