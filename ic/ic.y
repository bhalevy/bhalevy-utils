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

#include <stdlib.h>

#include "icintern.h"

static void ic_pow(ic_value_t *vp, ic_value_t *vp1);
static void ic_mul(ic_value_t *vp, ic_value_t *vp1);
static void ic_div(ic_value_t *vp, ic_value_t *vp1);
static void ic_mod(ic_value_t *vp, ic_value_t *vp1);
static void ic_add(ic_value_t *vp, ic_value_t *vp1);
static void ic_sub(ic_value_t *vp, ic_value_t *vp1);
static void ic_lsh(ic_value_t *vp, ic_value_t *vp1);
static void ic_rsh(ic_value_t *vp, ic_value_t *vp1);
static void ic_and(ic_value_t *vp, ic_value_t *vp1);
static void ic_or(ic_value_t *vp, ic_value_t *vp1);
static void ic_xor(ic_value_t *vp, ic_value_t *vp1);
static void ic_gt(ic_value_t *vp, ic_value_t *vp1);
static void ic_ge(ic_value_t *vp, ic_value_t *vp1);
static void ic_lt(ic_value_t *vp, ic_value_t *vp1);
static void ic_le(ic_value_t *vp, ic_value_t *vp1);
static void ic_eq(ic_value_t *vp, ic_value_t *vp1);
static void ic_ne(ic_value_t *vp, ic_value_t *vp1);
static int  ic_tobool(ic_value_t *vp);
static int  ic_tofloat(ic_value_t *vp);

int ibase = 10;
int obase = 10;
int Xflag;
char floatfmt[IC_MAX_FMT_SIZE] = IC_DEF_FLOAT_FMT "\n";
ic_value_t lastval[2];

int yydebug;

%}

%union {
	ic_value_t val;
	char *text;
}

%token IC_TOK_NUM
%token IC_TOK_NAME
%token IC_TOK_STRING
%token IC_TOK_DOT IC_TOK_DOTDOT

%left IC_TOK_GT IC_TOK_GE IC_TOK_LT IC_TOK_LE
%left IC_TOK_EQ IC_TOK_NE
%left IC_TOK_OR IC_TOK_XOR IC_TOK_AND
%left '|'
%left '^'
%left '&'
%left IC_TOK_LSH IC_TOK_RSH
%left '+' '-'
%left '*' '/' '%'
%left IC_TOK_POW
%right UNARY

%%

prog
	: stat
	| stat
	 '\n'
	 prog
	| stat ';' prog
	;

stat
	: exp
		{
			$$ = $1;
			lastval[1] = lastval[0];
			lastval[0] = $$.val;
			ic_print_val(&$1.val);
		}
	| IC_TOK_NAME '=' exp
		{
			ic_value_t *vp;

#ifdef YYDEBUG
			if (yydebug)
				printf("setting %s\n", $1.text);
#endif

			if ($3.val.type == IC_ERROR) {
				goto assign_done;
			}

			vp = ic_var_lookup($1.text);
			if (vp) {
				switch (vp->type) {
        			case IC_INT:
			        case IC_UNSIGNED:
			        case IC_FLOAT:
			        case IC_IPTR:
					break;
				default:
					yyerror("cannot assign to '%s'", $1.text);
					$$.val.type = IC_ERROR;
					goto assign_done;
				}
			}
			vp = ic_var_insert($1.text, &$3.val);
			if (vp != NULL) {
				$$.val = *vp;
			} else {
				yyerror("internal failed");
				$$.val.type = IC_ERROR;
			}
		assign_done:
			free($1.text);
			$1.text = 0;
		}
	|
	;

exp
        : IC_TOK_NAME '(' exp ')'
                {
                        ic_value_t *vp = ic_var_lookup($1.text);

                        if (vp != NULL) {
                                if (vp->type != IC_FUN) {
                                        yyerror("\"%s\" %s", $1.text, vp->type == IC_FUN2 ? "not enough parameters" : "is not a function");
                                        $$.val.type = IC_ERROR;
                                } else {
                                        $$ = $3;
                                        vp->u.fun(&$$.val);
                                }
                        } else {
                                yyerror("\"%s\" not found", $1.text);
                                $$.val.type = IC_ERROR;
                        }
                        free($1.text);
                        $1.text = 0;
                }
	| IC_TOK_NAME '(' exp ',' exp ')'
		{
			ic_value_t *vp = ic_var_lookup($1.text);

			if (vp != NULL) {
				if (vp->type != IC_FUN2) {
					yyerror("\"%s\" %s", $1.text, vp->type == IC_FUN ? "too many parameters" : "is not a function");
					$$.val.type = IC_ERROR;
				} else {
					$$ = $3;
					vp->u.fun2(&$$.val, &$5.val);
				}
			} else {
				yyerror("\"%s\" not found", $1.text);
				$$.val.type = IC_ERROR;
			}
			free($1.text);
			$1.text = 0;
		}
	| '(' exp ')'
		{	$$ = $2; }

	| '-' exp	%prec UNARY
		{
			$$.val.type = $2.val.type;
			switch ($2.val.type) {
			case IC_INT:
			case IC_UNSIGNED:
			case IC_ERROR:
				$$.val.u.i = - $2.val.u.i;
				break;
			case IC_FLOAT:
				$$.val.u.f = - $2.val.u.f;
				break;
			default:
				yyerror("unknown type %d\n", $2.val.type);
			}
		}
        | '+' exp	%prec UNARY
		{	$$ = $2; }

	| '~' exp	%prec UNARY
		{
			$$.val.type = $2.val.type;
			switch ($2.val.type) {
			case IC_INT:
			case IC_UNSIGNED:
			case IC_ERROR:
				$$.val.u.ui = ~ $2.val.u.ui;
				break;
			case IC_FLOAT:
				yyerror("integer only operation");
				break;
			default:
				yyerror("unknown type %d\n", $2.val.type);
			}
		}

	| '!' exp	%prec UNARY
		{
			$$ = $2;
			if (!ic_tobool(&$$.val))
				$$.val.u.i = ! $2.val.u.i;
		}

	| exp IC_TOK_POW exp
		{
			$$ = $1;
			ic_pow(&$$.val, &$3.val);
		}
	| exp '*' exp
		{
			$$ = $1;
			ic_mul(&$$.val, &$3.val);
		}
	| exp '/' exp
		{
			$$ = $1;
			ic_div(&$$.val, &$3.val);
		}
	| exp '%' exp
		{
			$$ = $1;
			ic_mod(&$$.val, &$3.val);
		}
	| exp '+' exp
		{
			$$ = $1;
			ic_add(&$$.val, &$3.val);
		}
	| exp '-' exp
		{
			$$ = $1;
			ic_sub(&$$.val, &$3.val);
		}

	| exp IC_TOK_LSH exp
		{
			$$ = $1;
			ic_lsh(&$$.val, &$3.val);
		}
	| exp IC_TOK_RSH exp
		{
			$$ = $1;
			ic_rsh(&$$.val, &$3.val);
		}

	| exp '&' exp
		{
			$$ = $1;
			ic_and(&$$.val, &$3.val);
		}
	| exp '|' exp
		{
			$$ = $1;
			ic_or(&$$.val, &$3.val);
		}
	| exp '^' exp
		{
			$$ = $1;
			ic_xor(&$$.val, &$3.val);
		}

	| exp IC_TOK_AND exp
		{
			$$ = $1;
			if (!ic_tobool(&$$.val) &&
			    !ic_tobool(&$3.val))
				$$.val.u.i &= $3.val.u.i;
			else
				$$.val.type = IC_ERROR;
		}
	| exp IC_TOK_OR exp
		{
			$$ = $1;
			if (!ic_tobool(&$$.val) &&
			    !ic_tobool(&$3.val))
				$$.val.u.i |= $3.val.u.i;
			else
				$$.val.type = IC_ERROR;
		}
	| exp IC_TOK_XOR exp
		{
			$$ = $1;
			if (!ic_tobool(&$$.val) &&
			    !ic_tobool(&$3.val))
				$$.val.u.i ^= $3.val.u.i;
			else
				$$.val.type = IC_ERROR;
		}

	| exp IC_TOK_GT exp
		{
			$$ = $1;
			ic_gt(&$$.val, &$3.val);
		}
	| exp IC_TOK_GE exp
		{
			$$ = $1;
			ic_ge(&$$.val, &$3.val);
		}
	| exp IC_TOK_LT exp
		{
			$$ = $1;
			ic_lt(&$$.val, &$3.val);
		}
	| exp IC_TOK_LE exp
		{
			$$ = $1;
			ic_le(&$$.val, &$3.val);
		}
	| exp IC_TOK_EQ exp
		{
			$$ = $1;
			ic_eq(&$$.val, &$3.val);
		}
	| exp IC_TOK_NE exp
		{
			$$ = $1;
			ic_ne(&$$.val, &$3.val);
		}

	| IC_TOK_NUM
		{	$$ = $1; }
	| IC_TOK_DOT
		{	$$.val = lastval[0]; }
	| IC_TOK_DOTDOT
		{	$$.val = lastval[1]; }
	| IC_TOK_NAME
		{
			ic_value_t *vp = ic_var_lookup($1.text);

			if (vp != NULL) {
				switch (vp->type) {
				case IC_INT:
				case IC_UNSIGNED:
				case IC_FLOAT:
					$$.val = *vp;
					break;
				case IC_IPTR:
					$$.val.type = IC_INT;
					$$.val.u.i = *vp->u.p;
					break;
				case IC_FUN0:
					vp->u.fun0();
					$$.val.type = IC_ERROR;
					break;
				case IC_FUN:
					yyerror("\"%s\" expects a parameter", $1.text);
					$$.val.type = IC_ERROR;
					break;
				case IC_FUN2:
					yyerror("\"%s\" expects two parameters", $1.text);
					$$.val.type = IC_ERROR;
					break;
				default:
					$$.val.type = IC_ERROR;
					break;
				}
			} else {
				ic_getnum(&$$.val, $1.text, -1);
				if ($$.val.type == IC_ERROR)
					yyerror("\"%s\" not found", $1.text);
//				$$.val.type = IC_ERROR;
			}
			free($1.text);
		}

	/* dirty trick to gracefully handle parser errors */
	| error
		{
			$$.val.type = IC_ERROR;
			yyerrok;
			yyclearin;
		}
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

int
ic_getnum(ic_value_t *vlp, char *sin, int base)
{
	int silent = 0;
    char s[64];
	char *end = NULL;
	ic_value_t kmul;

	kmul.type = IC_UNSIGNED;
	kmul.u.ui = 1024;

	if (base > 0)
		vlp->type = IC_UNSIGNED;
	else {
		if (base < 0)
			silent = 1;
		base = ibase;
		vlp->type = IC_INT;
	}

	if (strlen(sin) >= sizeof(s)) {
		vlp->type = IC_ERROR;
		goto out;
	}
	char *dest = s;
	char *src = sin;
    char ch;
	do {
		while (*src == '_') {
			src++;
		}
        ch = *src++;
        *dest++ = ch;
	} while (ch);
	vlp->u.ui = strtoull(s, &end, base);
	if (end == NULL) {
		vlp->type = IC_ERROR;
		end = s;
		goto out;
	}
next:
	switch (*end) {
	case '\0':
		break;
	case 'P': case 'p':
		ic_mul(vlp, &kmul);
	case 'T': case 't':
		ic_mul(vlp, &kmul);
	case 'G': case 'g':
		ic_mul(vlp, &kmul);
	case 'M': case 'm':
		ic_mul(vlp, &kmul);
	case 'K': case 'k':
		ic_mul(vlp, &kmul);
		end++;
		goto next;

	case 'S': case 's':
		if (vlp->type == IC_FLOAT) {
			vlp->type = IC_ERROR;
			break;
		}
		vlp->type = IC_INT;
		end++;
		goto next;
	case 'U': case 'u':
		if (vlp->type == IC_FLOAT) {
			vlp->type = IC_ERROR;
			break;
		}
		vlp->type = IC_UNSIGNED;
		end++;
		goto next;

	case 'E': case 'e':
		if (vlp->type == IC_FLOAT) {
			vlp->u.f *= IC_MATH_FUN(pow)(10, strtoll(end+1, &end, 0));
			if (*end)
				vlp->type = IC_ERROR;
			break;
		}
		/*FALLTHROUGH*/
	case '+': case '-':
	case '.':
		vlp->type = IC_FLOAT;
		vlp->u.f = strtod(s, &end);
		goto next;

	default:
		vlp->type = IC_ERROR;
		break;
	}

out:
	if (vlp->type == IC_ERROR && !silent)
		yyerror("illegal input \"%s\" (base=%d)", end, base);

	return IC_TOK_NUM;
}

static void
ic_print_uint(ic_uint64_t v, int sign)
{
	int digit;
	char buf[65], *s;

	s = buf + sizeof(buf);
	*--s = '\0';
	do {
		digit = v % obase;
		v /= obase;
		*--s = digit < 10 ?
			'0' + digit : 
			(Xflag ? 'A' : 'a') + digit - 10;
	} while (v);

	if (sign)
		*--s = sign;
	puts(s);
}

void
ic_print_val(ic_value_t *vp)
{
	if (yydebug) {
		printf("ic_print_val: type %d\n", vp->type);
	}

	switch (vp->type) {
	case IC_INT:
		if (vp->u.i < 0LL) {
			ic_print_uint(-vp->u.i, '-');
			break;
		}
		/*FALLTHROUGH*/
	case IC_UNSIGNED:
		ic_print_uint(vp->u.ui, 0);
		break;
	case IC_FLOAT:
		printf(floatfmt, vp->u.f);
		break;
	case IC_IPTR:
		if (*vp->u.p < 0LL)
			ic_print_uint(-*vp->u.p, '-');
		else
			ic_print_uint(*vp->u.p, 0);
		break;
	case IC_ERROR:
		break;
	default:
		yyerror("<unknown type> %llu", vp->u.ui);
	}
}

static int
ic_check_type(ic_value_t *vp, ic_value_t *vp1, int maxtype)
{
	if (maxtype) {
		if (vp->type <= maxtype && vp1->type <= maxtype) {
			if (vp1->type == IC_FLOAT &&
			    vp->type < IC_FLOAT) {
				ic_tofloat(vp);
			}
			return 0;
		}
	}

	if (vp->type < IC_ERROR && vp1->type < IC_ERROR) {
		if (!maxtype)
			return 0;
		yyerror("unsupported typa for operatione");
	} else if (vp->type > IC_ERROR)
		yyerror("unknown type %d", vp->type);
	else if (vp1->type > IC_ERROR)
		yyerror("unknown type %d", vp1->type);
	vp->type = IC_ERROR;
	return -1;
}

static int
ic_check_int(ic_value_t *vp, ic_value_t *vp1)
{
	if (vp->type == IC_FLOAT || vp1->type == IC_FLOAT) {
		yyerror("integer only operation");
		vp->type = IC_ERROR;
		return -1;
	}

	return ic_check_type(vp, vp1, IC_UNSIGNED);
}

static void
ic_mul(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.i *= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.i *= vp1->u.ui;
			break;
		case IC_FLOAT:
			vp->type = IC_FLOAT;
			vp->u.f = vp->u.i * vp1->u.f;
			break;
		}
		break;
	case IC_UNSIGNED:
		switch (vp1->type) {
		case IC_INT:
			vp->u.ui *= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.ui *= vp1->u.ui;
			break;
		case IC_FLOAT:
			vp->type = IC_FLOAT;
			vp->u.f = vp->u.ui * vp1->u.f;
			break;
		}
		break;
	case IC_FLOAT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.f *= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.f *= vp1->u.ui;
			break;
		case IC_FLOAT:
			vp->u.f *= vp1->u.f;
			break;
		}
		break;
	}
}

static int
ic_divbyzero(ic_value_t *vp, ic_value_t *vp1)
{
	int notzero = vp1->type == IC_FLOAT ? vp1->u.f != 0. : vp1->u.ui != 0;

	if (notzero)
		return 0;

	yyerror("division by zero");
	vp->type = IC_ERROR;
	return 1;
}

static void
ic_div(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	if (ic_divbyzero(vp, vp1))
		return;

	switch (vp->type) {
	case IC_INT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.i /= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.i /= vp1->u.ui;
			break;
		case IC_FLOAT:
			vp->type = IC_FLOAT;
			vp->u.f = vp->u.i / vp1->u.f;
			break;
		}
		break;
	case IC_UNSIGNED:
		switch (vp1->type) {
		case IC_INT:
			vp->u.ui /= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.ui /= vp1->u.ui;
			break;
		case IC_FLOAT:
			vp->type = IC_FLOAT;
			vp->u.f = vp->u.i / vp1->u.f;
		}
		break;
	case IC_FLOAT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.f /= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.f /= vp1->u.ui;
			break;
		case IC_FLOAT:
			vp->u.f /= vp1->u.f;
			break;
		}
		break;
	}
}

static void
ic_mod(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_int(vp, vp1))
		return;

	if (ic_divbyzero(vp, vp1))
		return;

	switch (vp->type) {
	case IC_INT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.i %= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.i %= vp1->u.ui;
			break;
		}
		break;
	case IC_UNSIGNED:
		switch (vp1->type) {
		case IC_INT:
			vp->u.ui %= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.ui %= vp1->u.ui;
			break;
		}
		break;
	}
}

static void
ic_add(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
	case IC_UNSIGNED:
		vp->u.ui += vp1->u.ui;
		break;
	case IC_FLOAT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.f += vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.f += vp1->u.ui;
			break;
		case IC_FLOAT:
			vp->u.f += vp1->u.f;
			break;
		}
		break;
	}
}

static void
ic_sub(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
	case IC_UNSIGNED:
		vp->u.ui -= vp1->u.ui;
		break;
	case IC_FLOAT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.f -= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.f -= vp1->u.ui;
			break;
		case IC_FLOAT:
			vp->u.f -= vp1->u.f;
			break;
		}
		break;
	}
}

static void
ic_lsh(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_int(vp, vp1))
		return;

	switch (vp->type) {
	case IC_INT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.i <<= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.i <<= vp1->u.ui;
			break;
		}
		break;
	case IC_UNSIGNED:
		switch (vp1->type) {
		case IC_INT:
			vp->u.ui <<= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.ui <<= vp1->u.ui;
			break;
		}
		break;
	}
}

static void
ic_rsh(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_int(vp, vp1))
		return;

	switch (vp->type) {
	case IC_INT:
		switch (vp1->type) {
		case IC_INT:
			vp->u.i >>= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.i >>= vp1->u.ui;
			break;
		}
		break;
	case IC_UNSIGNED:
		switch (vp1->type) {
		case IC_INT:
			vp->u.ui >>= vp1->u.i;
			break;
		case IC_UNSIGNED:
			vp->u.ui >>= vp1->u.ui;
			break;
		}
		break;
	}
}

static void
ic_and(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_int(vp, vp1))
		return;

	vp->u.ui &= vp1->u.ui;
}

static void
ic_or(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_int(vp, vp1))
		return;

	vp->u.ui |= vp1->u.ui;
}

static void
ic_xor(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_int(vp, vp1))
		return;

	vp->u.ui ^= vp1->u.ui;
}

static void
ic_gt(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
		vp->u.i = vp->u.i > vp1->u.i;
		break;
	case IC_UNSIGNED:
		vp->u.i = vp->u.ui > vp1->u.ui;
		break;
	case IC_FLOAT:
		ic_tofloat(vp1);
		vp->u.i = vp->u.f > vp1->u.f;
		break;
	}

	vp->type = IC_INT;
}

static void
ic_ge(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
		vp->u.i = vp->u.i >= vp1->u.i;
		break;
	case IC_UNSIGNED:
		vp->u.i = vp->u.ui >= vp1->u.ui;
		break;
	case IC_FLOAT:
		ic_tofloat(vp1);
		vp->u.i = vp->u.f >= vp1->u.f;
		break;
	}

	vp->type = IC_INT;
}

static void
ic_lt(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
		vp->u.i = vp->u.i < vp1->u.i;
		break;
	case IC_UNSIGNED:
		vp->u.i = vp->u.ui < vp1->u.ui;
		break;
	case IC_FLOAT:
		ic_tofloat(vp1);
		vp->u.i = vp->u.f < vp1->u.f;
		break;
	}

	vp->type = IC_INT;
}

static void
ic_le(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
		vp->u.i = vp->u.i <= vp1->u.i;
		break;
	case IC_UNSIGNED:
		vp->u.i = vp->u.ui <= vp1->u.ui;
		break;
	case IC_FLOAT:
		ic_tofloat(vp1);
		vp->u.i = vp->u.f <= vp1->u.f;
		break;
	}

	vp->type = IC_INT;
}

static void
ic_eq(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
	case IC_UNSIGNED:
		vp->u.i = vp->u.ui == vp1->u.ui;
		break;
	case IC_FLOAT:
		ic_tofloat(vp1);
		vp->u.i = vp->u.f == vp1->u.f;
		break;
	}

	vp->type = IC_INT;
}

static void
ic_ne(ic_value_t *vp, ic_value_t *vp1)
{
	if (ic_check_type(vp, vp1, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
	case IC_UNSIGNED:
		vp->u.i = vp->u.ui != vp1->u.ui;
		break;
	case IC_FLOAT:
		ic_tofloat(vp1);
		vp->u.i = vp->u.f != vp1->u.f;
		break;
	}

	vp->type = IC_INT;
}

static int
ic_tobool(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return -1;

	switch (vp->type) {
	case IC_INT:
	case IC_UNSIGNED:
		vp->u.i = vp->u.ui != 0;
		break;
	case IC_FLOAT:
		vp->u.i = vp->u.f != 0.;
		break;
	}

	vp->type = IC_INT;
	return 0;
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

static void
ic_help(void)
{
	puts(
	"Statments:\n"
	"  A statement can be an expression or an assignment\n"
	"  in the form of \"name = expression\".\n"
	"  Statements are normally separated by new lines.\n"
	"  Assignments and statements terminaterd by semicolons (';')\n"
	"  are silent while expression values are printed to the\n"
	"  standard output.\n"
	"\n"
	"Expressions:\n"
	"  An expression can be one of: a constant, a variable, \".\",\n"
	"  \"..\", or an arithmetic expression made of a unary operator and\n"
	"  an expression operand or two expressions surrounding a binary\n"
	"  operator. Parenthesis can be used to group sub expressions.\n"
	"  Built in functions are invoked by func(expression). Two or\n"
	"  more parameters are separated by commas (',').\n"
	"  \".\" amd \"..\" refer to the values of the most recent expression\n"
	"  and to the one before it respectively.\n"
	"\n"
	"Constants:\n"
	"  All constants are numeric and begin with a decimal digit [0-9].\n"
	"  The special variables \"ibase\" and \"obase\" determine the base\n"
	"  in which integer constants are read from the input and written\n"
	"  to the output.  Digits in the range [a-z] and [A-Z] can be used\n"
	"  when the input base is greater than 10. Integer constants can be\n"
	"  prefixed with \"0b\", \"0d\", or \"0x\" for explicit binary,\n"
	"  decimal, or hexadecimal base; in that case the constants will be\n"
	"  unsigned.  Numbers can be followed by [Uu] to explicitly denote an\n"
	"  unsigned value, or one of [KMGTP] to multiply by Kilo (2**10), Mega\n"
	"  (2**20), Giga (2**30), Tera (2**40), or Peta (2**50).\n"
	"  A number of the form /(([0-9]*\".\"[0-9]*)|[0-9]+)([Ee][+-]?[0-9]+)?/\n"
	"  is a floating point constant.\n"
	"\n"
	"Types:\n"
	"  An evaluated expression can be a 64-bit signed or unsigned integer,\n"
	"  or a floating point number. signed(n), unsigned(n), float(n) can be\n"
	"  used to typecast a value. short(n), ushort(n), int(n), and uint(n)\n"
	"  set the values type and truncate it to 16 or 32 bits and extend\n"
	"  its sign bit if the result is a signed integer.\n"
	"\n"
	"Operators (in increasing precedence):\n"
	"  ()\n"
	"  >  >= <  <=\n"
	"  == !=\n"
	"  || ^^ &&\n"
	"  |  ^  &\n"
	"  << >>\n"
	"  +  -\n"
	"  *  /  %\n"
	"  **\n"
	"  unary -  +  ~  !\n"
	"\n"
	"Built-in functions:\n"
	"  help    print this messages\n"
	"  funcs   print all built-in functions\n"
	"  mem     print all variables in memory\n"
	);
}

static void
ic_funcs(void)
{
	puts(
	"  swab2   swab a 2-byte value\n"
	"  swab4   swab a 4-byte value\n"
	"  swab8   swab a 8-byte value\n"
	"\n"
	"  abs     absolute value\n"
	"  acos    inverse cosine\n"
	"  acosh   inverse hyperbolic cosinen"
	"  asin    inverse sine\n"
	"  asinh   inverse hyperbolic sine\n"
	"  atan    inverse tangent\n"
	"  atanh   inverse hyperbolic tangent\n"
	"  cbrt    cube root\n"
	"  ceil    integer no less than\n"
	"  cos     cosine\n"
	"  cosh    hyperbolic cosine\n"
	"  exp     exponential\n"
	"  expm1   exp(x)-1\n"
	"  fabs    absolute value\n"
	"  floor   integer no greater than\n"
	"  log     natural logarithm\n"
	"  log10   logarithm to base 10\n"
	"  log1p   log(1+x)\n"
	"  pow     exponential x**y\n"
	"  rint    round to nearest integer\n"
	"  sin     trigonometric function\n"
	"  sinh    hyperbolic function\n"
	"  sqrt    square root\n"
	"  tan     trigonometric function\n"
	"  tanh    hyperbolic function\n"
	"\n"
	"  See also math library manual pages"
	);
}

void
ic_mem(void)
{
	ic_var_t *vp, **vpp;

	for (vpp = ic_var_hash; vpp < ic_var_hash + IC_VAR_HASH_SIZE; vpp++) {
		for (vp = *vpp; vp; vp = vp->next) {
			if (vp->val.type > IC_FLOAT)
				continue;
			printf("  %-7s ", vp->text);
			ic_print_val(&vp->val);
		}
	}
}

static void
ic_abs(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return;

	switch (vp->type) {
	case IC_INT:
		if (vp->u.i < 0)
			vp->u.i = -vp->u.i;
		break;
	case IC_UNSIGNED:
		break;
	case IC_FLOAT:
		if (vp->u.f < 0.)
			vp->u.f = -vp->u.f;
		break;
	}
}

static void
ic_short(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return;

	if (vp->type == IC_FLOAT)
		vp->u.i = floor(vp->u.f);
	else
		vp->u.i = (vp->u.i << 48) >> 48;

	vp->type = IC_INT;
}

static void
ic_int(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return;

	if (vp->type == IC_FLOAT)
		vp->u.i = floor(vp->u.f);
	else
		vp->u.i = (vp->u.i << 32) >> 32;

	vp->type = IC_INT;
}

static void
ic_signed(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return;

	if (vp->type == IC_FLOAT)
		vp->u.i = floor(vp->u.f);

	vp->type = IC_INT;
}

static void
ic_ushort(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return;

	if (vp->type == IC_FLOAT)
		vp->u.ui = floor(vp->u.f);
	else
		vp->u.ui &= 0xffff;

	vp->type = IC_UNSIGNED;
}

static void
ic_uint(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return;

	if (vp->type == IC_FLOAT)
		vp->u.ui = floor(vp->u.f);
	else
		vp->u.ui &= 0xffffffff;

	vp->type = IC_UNSIGNED;
}

static void
ic_unsigned(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return;

	if (vp->type == IC_FLOAT)
		vp->u.ui = floor(vp->u.f);

	vp->type = IC_UNSIGNED;
}

static int
ic_tofloat(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_FLOAT))
		return -1;

	switch (vp->type) {
	case IC_INT:
		vp->u.f = vp->u.i;
		break;
	case IC_UNSIGNED:
		vp->u.f = vp->u.ui;
		break;
	}

	vp->type = IC_FLOAT;
	return 0;
}

static void
ic_swab2(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_UNSIGNED))
		return;

	if (vp->u.ui > 0xffff)
		yyerror("value too big");

	vp->u.ui = ((vp->u.ui & (0xff << 0)) << 8) |
	           ((vp->u.ui & (0xff << 8)) >> 8);
}

static void
ic_swab4(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_UNSIGNED))
		return;

	if (vp->u.ui > 0xffffffff)
		yyerror("value too big");

	vp->u.ui = ((vp->u.ui & (0xff <<  0)) << 24) |
	           ((vp->u.ui & (0xff <<  8)) <<  8) |
	           ((vp->u.ui & (0xff << 16)) >>  8) |
	           ((vp->u.ui & (0xff << 24)) >> 24);
}

static void
ic_swab8(ic_value_t *vp)
{
	if (ic_check_type(vp, vp, IC_UNSIGNED))
		return;

	vp->u.ui = ((vp->u.ui & (0xffULL <<  0)) << 56) |
	           ((vp->u.ui & (0xffULL <<  8)) << 40) |
	           ((vp->u.ui & (0xffULL << 16)) << 24) |
	           ((vp->u.ui & (0xffULL << 24)) <<  8) |
	           ((vp->u.ui & (0xffULL << 32)) >>  8) |
	           ((vp->u.ui & (0xffULL << 40)) >> 24) |
	           ((vp->u.ui & (0xffULL << 48)) >> 40) |
	           ((vp->u.ui & (0xffULL << 56)) >> 56);
}

#define IC_DEF_MATH_FUN(_name_) \
	static void ic_##_name_(ic_value_t *vp) \
	{ \
		if (ic_tofloat(vp)) \
			return; \
		vp->u.f = IC_MATH_FUN(_name_)(vp->u.f); \
	}

#define IC_DEF_MATH_FUN_COND(_name_, _cond_, _ref_) \
	static void ic_##_name_(ic_value_t *vp) \
	{ \
		if (ic_tofloat(vp)) \
			return; \
		if (vp->u.f _cond_ _ref_) { \
			vp->u.f = IC_MATH_FUN(_name_)(vp->u.f); \
		} else { \
			yyerror("\"" #_name_ "\" operand sould be " #_cond_ " " #_ref_); \
			vp->type = IC_ERROR; \
		} \
	}

#define IC_DEF_MATH_FUN_RANGE(_name_, _lcond_, _min_, _rcond_, _max_) \
	static void ic_##_name_(ic_value_t *vp) \
	{ \
		if (ic_tofloat(vp)) \
			return; \
		if (vp->u.f _lcond_ _min_ && vp->u.f _rcond_ _max_) { \
			vp->u.f = IC_MATH_FUN(_name_)(vp->u.f); \
		} else { \
			yyerror("\"" #_name_ "\" operand sould be " #_lcond_ " " #_min_ " and " #_rcond_ " " #_max_); \
			vp->type = IC_ERROR; \
		} \
	}

#define IC_DEF_MATH_FUN2(_name_) \
	static void ic_##_name_(ic_value_t *vp, ic_value_t *vp2) \
	{ \
		if (ic_tofloat(vp2)) { \
			vp->type = IC_ERROR; \
			return; \
		} \
		if (ic_tofloat(vp)) \
			return; \
		vp->u.f = IC_MATH_FUN(_name_)(vp->u.f, vp2->u.f); \
	}

#define IC_DEF_MATH_FUN2_COND(_name_, _cond_, _ref_) \
	static void ic_##_name_(ic_value_t *vp, ic_value_t *vp2) \
	{ \
		if (ic_tofloat(vp2)) { \
			vp->type = IC_ERROR; \
			return; \
		} \
		if (ic_tofloat(vp)) \
			return; \
		if (vp2->u.f _cond_ _ref_) { \
			vp->u.f = IC_MATH_FUN(_name_)(vp->u.f, vp2->u.f); \
		} else { \
			yyerror("\"" #_name_ "\" operand2 sould be " #_cond_ " " #_ref_); \
			vp->type = IC_ERROR; \
		} \
	}

IC_DEF_MATH_FUN_RANGE(acos, >=, -1., <=, 1.)
IC_DEF_MATH_FUN_RANGE(asin, >=, -1., <=, 1.)
IC_DEF_MATH_FUN(atan)

IC_DEF_MATH_FUN(cos)
IC_DEF_MATH_FUN(sin)
IC_DEF_MATH_FUN(tan)

IC_DEF_MATH_FUN_COND(acosh, >=, 1.)
IC_DEF_MATH_FUN(asinh)
IC_DEF_MATH_FUN_COND(atanh, <, 1.)

IC_DEF_MATH_FUN(cosh)
IC_DEF_MATH_FUN(sinh)
IC_DEF_MATH_FUN(tanh)

IC_DEF_MATH_FUN(exp)
IC_DEF_MATH_FUN(expm1)
IC_DEF_MATH_FUN(log)
IC_DEF_MATH_FUN(log10)
IC_DEF_MATH_FUN(log1p)

IC_DEF_MATH_FUN2(pow)
IC_DEF_MATH_FUN_COND(sqrt, >, 0.)
IC_DEF_MATH_FUN_COND(cbrt, >, 0.)

IC_DEF_MATH_FUN(fabs)
IC_DEF_MATH_FUN(ceil)
IC_DEF_MATH_FUN(floor)
IC_DEF_MATH_FUN(rint)
IC_DEF_MATH_FUN2_COND(fmod, !=, 0.)

static int
ic_init_vars(int *argcp, char ***argvp)
{
	char *argv0 = **argvp;
	ic_value_t val, *vp;

	val.type = IC_IPTR;
	val.u.p = &ibase;
	if ((vp = ic_var_insert("ibase", &val)) == NULL) {
		fprintf(stderr, "%s: internal error\n", argv0);
		return 1;
	}

	val.type = IC_IPTR;
	val.u.p = &obase;
	if ((vp = ic_var_insert("obase", &val)) == NULL) {
		fprintf(stderr, "%s: internal error\n", argv0);
		return 1;
	}

#define FUN0_INSERT(_name_) { \
	val.type = IC_FUN0; \
	val.u.fun0 = (void (*)(void))ic_##_name_; \
	ic_var_insert(#_name_, &val); \
}

#define FUN_INSERT(_name_) { \
	val.type = IC_FUN; \
	val.u.fun = (void (*)(ic_value_t*))ic_##_name_; \
	ic_var_insert(#_name_, &val); \
}

#define FUN_INSERT_ALIAS(_alias_, _name_) { \
	val.type = IC_FUN; \
	val.u.fun = (void (*)(ic_value_t*))ic_##_name_; \
	ic_var_insert(#_alias_, &val); \
}

#define FUN2_INSERT(_name_) { \
	val.type = IC_FUN2; \
	val.u.fun2 = ic_##_name_; \
	ic_var_insert(#_name_, &val); \
}

	FUN0_INSERT(help)
	FUN0_INSERT(funcs)
	FUN0_INSERT(mem)

	FUN_INSERT(abs)

	FUN_INSERT(short)
	FUN_INSERT_ALIAS(toshort, short)
	FUN_INSERT_ALIAS(int16, short)
	FUN_INSERT_ALIAS(toint16, short)

	FUN_INSERT(int)
	FUN_INSERT_ALIAS(toint, int)
	FUN_INSERT_ALIAS(int32, int)
	FUN_INSERT_ALIAS(int32_t, int)
	FUN_INSERT_ALIAS(long, int)
	FUN_INSERT_ALIAS(tolong, int)

	FUN_INSERT(signed)
	FUN_INSERT_ALIAS(tosigned, signed)

	FUN_INSERT(ushort)
	FUN_INSERT_ALIAS(toushort, ushort)
	FUN_INSERT_ALIAS(uint16, ushort)
	FUN_INSERT_ALIAS(uint16_t, ushort)
	FUN_INSERT_ALIAS(u_int16, ushort)
	FUN_INSERT_ALIAS(u_int16_t, ushort)

	FUN_INSERT(uint)
	FUN_INSERT_ALIAS(touint, uint)
	FUN_INSERT_ALIAS(u_int, uint)
	FUN_INSERT_ALIAS(uint32, uint)
	FUN_INSERT_ALIAS(uint32_t, uint)
	FUN_INSERT_ALIAS(u_int32, uint)
	FUN_INSERT_ALIAS(u_int32_t, uint)

	FUN_INSERT(unsigned)
	FUN_INSERT_ALIAS(tounsigned, unsigned)

	FUN_INSERT(tofloat)
	FUN_INSERT_ALIAS(float, tofloat)
	FUN_INSERT_ALIAS(double, tofloat)

	FUN_INSERT(acos)
	FUN_INSERT(acosh)
	FUN_INSERT(asin)
	FUN_INSERT(asinh)
	FUN_INSERT(atan)
	FUN_INSERT(atanh)
	FUN_INSERT(cbrt)
	FUN_INSERT(ceil)
	FUN_INSERT(cos)
	FUN_INSERT(cosh)
	FUN_INSERT(exp)
	FUN_INSERT(expm1)
	FUN_INSERT(fabs)
	FUN_INSERT(floor)
	FUN2_INSERT(fmod)
	FUN_INSERT(log)
	FUN_INSERT(log10)
	FUN_INSERT(log1p)
	FUN2_INSERT(pow)
	FUN_INSERT(rint)
	FUN_INSERT_ALIAS(round, rint)
	FUN_INSERT(sin)
	FUN_INSERT(sinh)
	FUN_INSERT(sqrt)
	FUN_INSERT(tan)
	FUN_INSERT(tanh)

	FUN_INSERT(swab2)
	FUN_INSERT(swab4)
	FUN_INSERT(swab8)

#undef FUN0_INSERT
#undef FUN_INSERT
#undef FUN_INSERT_ALIAS
#undef FUN2_INSERT

#define CONST_INSERT(_name_, _type_, _field_, _val_) { \
	val.type = IC_##_type_; \
	val.u._field_ = (_val_); \
	ic_var_insert(#_name_, &val); \
}
	
	CONST_INSERT(K, UNSIGNED, ui, 1LL << 10)
	CONST_INSERT(M, UNSIGNED, ui, 1LL << 20)
	CONST_INSERT(G, UNSIGNED, ui, 1LL << 30)
	CONST_INSERT(T, UNSIGNED, ui, 1LL << 40)
	CONST_INSERT(P, UNSIGNED, ui, 1LL << 50)

	CONST_INSERT(E, FLOAT, f, M_E)
	CONST_INSERT(PI, FLOAT, f, M_PI)

#undef CONST_INSERT

	return 0;
}

int
main(int argc, char **argv)
{
	int ch;

	if (ic_init_vars(&argc, &argv))
		return 1;

	while ((ch = getopt(argc, argv, ":dhXf:i:o:")) != -1)
		switch (ch) {
		case 'd':
			yydebug++;
			break;
		case 'X':
			Xflag++;
			break;
		case 'f':
			snprintf(floatfmt, sizeof(floatfmt), "%s\n", optarg);
			break;
		case 'i':
			ibase = atoi(optarg);
			break;
		case 'o':
			obase = atoi(optarg);
			break;
		case 'h':
		case '?':
		default:
			fprintf(stderr, "Usage: %s [-dhX] [-f floatfmt] [-i ibase] [-o obase] [file ...]\n", argv[0]);
			fprintf(stderr, "use %s's help function for detailed information\n", argv[0]);
			return 2;
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
