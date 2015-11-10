/*
 * Copyright (c) 2000, 2003
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

#ifndef IC_INTERN_H
#define IC_INTERN_H

#include <sys/types.h>

#ifdef __linux__

#define IC_HAVE_LONG_DOUBLE HAVE_LONG_DOUBLE
#if (__STDC__ - 0 || __GNUC__ - 0) && defined(__NO_LONG_DOUBLE_MATH)
#undef IC_HAVE_LONG_DOUBLE 0
#endif
#endif /* __linux__ */

#define IC_MAX_FMT_SIZE		10
#define IC_VAR_MAX_LEN		256
#define IC_VAR_HASH_SIZE	1023

typedef enum ic_types_e {
	IC_INT,
	IC_UNSIGNED,
	IC_FLOAT,
	IC_IPTR,
	IC_FUN0,
	IC_FUN,
	IC_FUN2,
	IC_ERROR
} ic_types_t;

typedef struct ic_value_s ic_value_t;

#ifdef IC_HAVE_LONG_DOUBLE

#define IC_DEF_FLOAT_FMT "%LF"
#define IC_MATH_FUN(_name_) (_name_##l)
typedef long double ic_float_t;

#else

#define IC_DEF_FLOAT_FMT "%f"
#define IC_MATH_FUN(_name_) (_name_)
typedef double ic_float_t;

#endif

typedef long long int ic_int64_t;
typedef long long unsigned int ic_uint64_t;

struct ic_value_s {
	union {
		ic_int64_t i;
		ic_uint64_t ui;
		ic_float_t f;
		int *p;
		void (*fun0)(void);
		void (*fun)(ic_value_t *vp);
		void (*fun2)(ic_value_t *vp, ic_value_t *vp2);
	} u;
	unsigned type;
};

typedef struct ic_var_s ic_var_t;
struct ic_var_s {
	ic_var_t *next;
	char text[IC_VAR_MAX_LEN];
	ic_value_t val;
};

extern int ibase, obase;
extern int ic_errors;

extern int ic_getnum(ic_value_t *vlp, char *s, int base);
extern void ic_print_val(ic_value_t *vp);
extern ic_value_t *ic_var_lookup(char *text);
extern ic_value_t *ic_var_insert(char *text, ic_value_t *vp);

extern ic_var_t *ic_var_hash[IC_VAR_HASH_SIZE];

/*
 * some yacc declarations
 */
extern int yydebug;

int yyparse(void);
int yylex(void);
int yyerror(char *, ...);

#define YYDEBUG 1

#endif /* IC_INTERN_H */
