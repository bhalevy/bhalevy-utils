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

#include <stdlib.h>
#include <string.h>
#include "icintern.h"

ic_var_t *ic_var_hash[IC_VAR_HASH_SIZE];

static ic_var_t **
ic_var_hash_val(char *s)
{
  int h;

  for (h = 0; *s; s++) {
    h <<= 1;
    h += *s;
  }

  return ic_var_hash + h % IC_VAR_HASH_SIZE;
}

ic_value_t *
ic_var_lookup(char *text)
{
  ic_var_t *vp, **vpp = ic_var_hash_val(text);

  for (vp = *vpp; vp; vp = vp->next) {
    if (!strncmp(vp->text, text, IC_VAR_MAX_LEN)) {
      return &vp->val;
    }
  }

  return 0;
}

ic_value_t *
ic_var_insert(char *text, ic_value_t *vlp)
{
  ic_var_t *vp, **vpp = ic_var_hash_val(text);

  for (vp = *vpp; vp; vp = vp->next) {
    if (!strncmp(vp->text, text, IC_VAR_MAX_LEN)) {
      if (vlp != NULL) {
	switch (vp->val.type) {
	case IC_INT:
	case IC_UNSIGNED:
	case IC_FLOAT:
		vp->val = *vlp;
		break;
	case IC_IPTR:
		*vp->val.u.p = vlp->type == IC_FLOAT ? (int)(vlp->u.f) : vlp->u.i;
		break;
	default:
		return NULL;
	}
      }
      return &vp->val;
    }
  }

  vp = calloc(1, sizeof(*vp));
  if (vp == NULL)
    return NULL;

  strncpy(vp->text, text, IC_VAR_MAX_LEN);
  if (vlp)
    vp->val = *vlp;
  vp->next = *vpp;
  *vpp = vp;

  return &vp->val;
}
