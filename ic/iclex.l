/*
 * Copyright (c) 2000-2007
 *      Benny Halevy.  All rights reserved.
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

#include <sys/types.h>
#include <string.h>
#include <math.h>

#include "icintern.h"
#include "y.tab.h"

%}

SPACE	[ \t\r]
COMMENT	"#".*$
DIGIT	[0-9]
LETTER	[A-Za-z]
ALNUM	[0-9A-Za-z]
OP	[-/+*%|&^~!()=,;\n]
DOT	[.]
SIGN	[-+]

%%

{COMMENT}

{SPACE}+

"<<"	{ return IC_TOK_LSH; }

">>"	{ return IC_TOK_RSH; }

".."	{ return IC_TOK_DOTDOT; }

"."	{ return IC_TOK_DOT; }

"<"	{ return IC_TOK_LT; }
"<="	{ return IC_TOK_LE; }
">"	{ return IC_TOK_GT; }
">="	{ return IC_TOK_GE; }
"=="	{ return IC_TOK_EQ; }
"!="	{ return IC_TOK_NE; }
"<>"	{ return IC_TOK_NE; }

"&&"	{ return IC_TOK_AND; }
"||"	{ return IC_TOK_OR; }
"^^"	{ return IC_TOK_XOR; }

"**"	{ return IC_TOK_POW; }

{OP}	{ return *yytext; }

({LETTER}|_)({ALNUM}|_)*	{
		if ((yylval.text = strdup(yytext)) == NULL) {
			yyerror("out of memory");
			exit(1);
		}
		return IC_TOK_NAME;
	}

0{DIGIT}+	{ return ic_getnum(&yylval.val, yytext+1, 8); }

0x{ALNUM}+	{ return ic_getnum(&yylval.val, yytext+2, 16); }

0d{ALNUM}+	{ return ic_getnum(&yylval.val, yytext+2, 10); }

0b{ALNUM}+	{ return ic_getnum(&yylval.val, yytext+2, 2); }

{ALNUM}+	{ return ic_getnum(&yylval.val, yytext, 0); }

(({DIGIT}+{DOT}{DIGIT}*)|({DIGIT}*{DOT}{DIGIT}+)|{DIGIT}+)([Ee][-+]?{DIGIT}+)?	{
		return ic_getnum(&yylval.val, yytext, 0);
	}

%%

/*
({ALNUM}|{DOT}|{SIGN})+	{ return ic_getnum(&yylval.val, yytext, 0); }
*/
