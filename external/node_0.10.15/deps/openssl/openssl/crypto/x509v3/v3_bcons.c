/* v3_bcons.c */
/* Written by Dr Stephen N Henson (steve@openssl.org) for the OpenSSL
 * project 1999.
 */
/* ====================================================================
 * Copyright (c) 1999 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */


#include <stdio.h>
#include "cryptlib.h"
#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>

static STACK_OF(CONF_VALUE) *i2v_BASIC_CONSTRAINTS(X509V3_EXT_METHOD *method, BASIC_CONSTRAINTS *bcons, STACK_OF(CONF_VALUE) *extlist);
static BASIC_CONSTRAINTS *v2i_BASIC_CONSTRAINTS(X509V3_EXT_METHOD *method, X509V3_CTX *ctx, STACK_OF(CONF_VALUE) *values);

const X509V3_EXT_METHOD v3_bcons = {
NID_basic_constraints, 0,
ASN1_ITEM_ref(BASIC_CONSTRAINTS),
0,0,0,0,
0,0,
(X509V3_EXT_I2V)i2v_BASIC_CONSTRAINTS,
(X509V3_EXT_V2I)v2i_BASIC_CONSTRAINTS,
NULL,NULL,
NULL
};

ASN1_SEQUENCE(BASIC_CONSTRAINTS) = {
	ASN1_OPT(BASIC_CONSTRAINTS, ca, ASN1_FBOOLEAN),
	ASN1_OPT(BASIC_CONSTRAINTS, pathlen, ASN1_INTEGER)
} ASN1_SEQUENCE_END(BASIC_CONSTRAINTS)

IMPLEMENT_ASN1_FUNCTIONS(BASIC_CONSTRAINTS)


static STACK_OF(CONF_VALUE) *i2v_BASIC_CONSTRAINTS(X509V3_EXT_METHOD *method,
	     BASIC_CONSTRAINTS *bcons, STACK_OF(CONF_VALUE) *extlist)
{
	X509V3_add_value_bool("CA", bcons->ca, &extlist);
	X509V3_add_value_int("pathlen", bcons->pathlen, &extlist);
	return extlist;
}

static BASIC_CONSTRAINTS *v2i_BASIC_CONSTRAINTS(X509V3_EXT_METHOD *method,
	     X509V3_CTX *ctx, STACK_OF(CONF_VALUE) *values)
{
	BASIC_CONSTRAINTS *bcons=NULL;
	CONF_VALUE *val;
	int i;
	if(!(bcons = BASIC_CONSTRAINTS_new())) {
		X509V3err(X509V3_F_V2I_BASIC_CONSTRAINTS, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	for(i = 0; i < sk_CONF_VALUE_num(values); i++) {
		val = sk_CONF_VALUE_value(values, i);
		if(!strcmp(val->name, "CA")) {
			if(!X509V3_get_value_bool(val, &bcons->ca)) goto err;
		} else if(!strcmp(val->name, "pathlen")) {
			if(!X509V3_get_value_int(val, &bcons->pathlen)) goto err;
		} else {
			X509V3err(X509V3_F_V2I_BASIC_CONSTRAINTS, X509V3_R_INVALID_NAME);
			X509V3_conf_err(val);
			goto err;
		}
	}
	return bcons;
	err:
	BASIC_CONSTRAINTS_free(bcons);
	return NULL;
}

