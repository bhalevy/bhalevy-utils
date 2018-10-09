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

#include <sys/types.h>
#include <string.h>
#include <math.h>

#include "intern.h"
#include "y.tab.h"

static void print_comment(char *s)
{
    int pnl = s[-1] == '\n';
    printf("%s%s", pnl ? "" : " ", s);
}

%}

PRAGMA #.*$
LN_COMMENT \/\/.*$
COMMENT	\/\*([^*]|(\*+[^*/]))*\*+\/
SPACE   [ \t\r]
CODE    \{[^\}]*\}
PARAMS  \[[^\]]*\]
DIGIT   [0-9]
LETTER  [A-Za-z]
ALNUM	[0-9A-Za-z]
NAME    ({LETTER}|_)({ALNUM}|_)*
OP      [:|()=;?*+@]

%%

{PRAGMA}     { printf("%s", yytext); }
{LN_COMMENT} { print_comment(yytext); }
{COMMENT}    { print_comment(yytext); }

\n           { putchar('\n'); }

^{SPACE}+    { printf("%s", yytext); }

{SPACE}+

\'([^\']*(\\\')?)*\'        { return TOK_CHAR_LITERAL; }

\"([^\"]*(\\\")?)*\"        { return TOK_STRING_LITERAL; }

{OP}	                    { return *yytext; }

FRAGMENT|fragment           { return TOK_FRAGMENT; }

RETURNS|returns             { return TOK_RETURNS; }

OPTIONS|options             { return TOK_OPTIONS; }

@{NAME}(\:\:{NAME})?          { return TOK_PREFIX; }

{NAME}                      { return TOK_NAME; }

{PARAMS}                    { return TOK_PARAMS; }

{CODE}                      { return TOK_CODE; }

%%
