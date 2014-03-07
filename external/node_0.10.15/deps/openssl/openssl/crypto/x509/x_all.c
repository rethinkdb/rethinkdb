/* crypto/x509/x_all.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include <openssl/stack.h>
#include "cryptlib.h"
#include <openssl/buffer.h>
#include <openssl/asn1.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#ifndef OPENSSL_NO_RSA
#include <openssl/rsa.h>
#endif
#ifndef OPENSSL_NO_DSA
#include <openssl/dsa.h>
#endif

int X509_verify(X509 *a, EVP_PKEY *r)
	{
	return(ASN1_item_verify(ASN1_ITEM_rptr(X509_CINF),a->sig_alg,
		a->signature,a->cert_info,r));
	}

int X509_REQ_verify(X509_REQ *a, EVP_PKEY *r)
	{
	return( ASN1_item_verify(ASN1_ITEM_rptr(X509_REQ_INFO),
		a->sig_alg,a->signature,a->req_info,r));
	}

int NETSCAPE_SPKI_verify(NETSCAPE_SPKI *a, EVP_PKEY *r)
	{
	return(ASN1_item_verify(ASN1_ITEM_rptr(NETSCAPE_SPKAC),
		a->sig_algor,a->signature,a->spkac,r));
	}

int X509_sign(X509 *x, EVP_PKEY *pkey, const EVP_MD *md)
	{
	x->cert_info->enc.modified = 1;
	return(ASN1_item_sign(ASN1_ITEM_rptr(X509_CINF), x->cert_info->signature,
		x->sig_alg, x->signature, x->cert_info,pkey,md));
	}

int X509_sign_ctx(X509 *x, EVP_MD_CTX *ctx)
	{
	return ASN1_item_sign_ctx(ASN1_ITEM_rptr(X509_CINF),
		x->cert_info->signature,
		x->sig_alg, x->signature, x->cert_info, ctx);
	}

int X509_REQ_sign(X509_REQ *x, EVP_PKEY *pkey, const EVP_MD *md)
	{
	return(ASN1_item_sign(ASN1_ITEM_rptr(X509_REQ_INFO),x->sig_alg, NULL,
		x->signature, x->req_info,pkey,md));
	}

int X509_REQ_sign_ctx(X509_REQ *x, EVP_MD_CTX *ctx)
	{
	return ASN1_item_sign_ctx(ASN1_ITEM_rptr(X509_REQ_INFO),
		x->sig_alg, NULL, x->signature, x->req_info, ctx);
	}

int X509_CRL_sign(X509_CRL *x, EVP_PKEY *pkey, const EVP_MD *md)
	{
	x->crl->enc.modified = 1;
	return(ASN1_item_sign(ASN1_ITEM_rptr(X509_CRL_INFO),x->crl->sig_alg,
		x->sig_alg, x->signature, x->crl,pkey,md));
	}

int X509_CRL_sign_ctx(X509_CRL *x, EVP_MD_CTX *ctx)
	{
	return ASN1_item_sign_ctx(ASN1_ITEM_rptr(X509_CRL_INFO),
		x->crl->sig_alg, x->sig_alg, x->signature, x->crl, ctx);
	}

int NETSCAPE_SPKI_sign(NETSCAPE_SPKI *x, EVP_PKEY *pkey, const EVP_MD *md)
	{
	return(ASN1_item_sign(ASN1_ITEM_rptr(NETSCAPE_SPKAC), x->sig_algor,NULL,
		x->signature, x->spkac,pkey,md));
	}

#ifndef OPENSSL_NO_FP_API
X509 *d2i_X509_fp(FILE *fp, X509 **x509)
	{
	return ASN1_item_d2i_fp(ASN1_ITEM_rptr(X509), fp, x509);
	}

int i2d_X509_fp(FILE *fp, X509 *x509)
	{
	return ASN1_item_i2d_fp(ASN1_ITEM_rptr(X509), fp, x509);
	}
#endif

X509 *d2i_X509_bio(BIO *bp, X509 **x509)
	{
	return ASN1_item_d2i_bio(ASN1_ITEM_rptr(X509), bp, x509);
	}

int i2d_X509_bio(BIO *bp, X509 *x509)
	{
	return ASN1_item_i2d_bio(ASN1_ITEM_rptr(X509), bp, x509);
	}

#ifndef OPENSSL_NO_FP_API
X509_CRL *d2i_X509_CRL_fp(FILE *fp, X509_CRL **crl)
	{
	return ASN1_item_d2i_fp(ASN1_ITEM_rptr(X509_CRL), fp, crl);
	}

int i2d_X509_CRL_fp(FILE *fp, X509_CRL *crl)
	{
	return ASN1_item_i2d_fp(ASN1_ITEM_rptr(X509_CRL), fp, crl);
	}
#endif

X509_CRL *d2i_X509_CRL_bio(BIO *bp, X509_CRL **crl)
	{
	return ASN1_item_d2i_bio(ASN1_ITEM_rptr(X509_CRL), bp, crl);
	}

int i2d_X509_CRL_bio(BIO *bp, X509_CRL *crl)
	{
	return ASN1_item_i2d_bio(ASN1_ITEM_rptr(X509_CRL), bp, crl);
	}

#ifndef OPENSSL_NO_FP_API
PKCS7 *d2i_PKCS7_fp(FILE *fp, PKCS7 **p7)
	{
	return ASN1_item_d2i_fp(ASN1_ITEM_rptr(PKCS7), fp, p7);
	}

int i2d_PKCS7_fp(FILE *fp, PKCS7 *p7)
	{
	return ASN1_item_i2d_fp(ASN1_ITEM_rptr(PKCS7), fp, p7);
	}
#endif

PKCS7 *d2i_PKCS7_bio(BIO *bp, PKCS7 **p7)
	{
	return ASN1_item_d2i_bio(ASN1_ITEM_rptr(PKCS7), bp, p7);
	}

int i2d_PKCS7_bio(BIO *bp, PKCS7 *p7)
	{
	return ASN1_item_i2d_bio(ASN1_ITEM_rptr(PKCS7), bp, p7);
	}

#ifndef OPENSSL_NO_FP_API
X509_REQ *d2i_X509_REQ_fp(FILE *fp, X509_REQ **req)
	{
	return ASN1_item_d2i_fp(ASN1_ITEM_rptr(X509_REQ), fp, req);
	}

int i2d_X509_REQ_fp(FILE *fp, X509_REQ *req)
	{
	return ASN1_item_i2d_fp(ASN1_ITEM_rptr(X509_REQ), fp, req);
	}
#endif

X509_REQ *d2i_X509_REQ_bio(BIO *bp, X509_REQ **req)
	{
	return ASN1_item_d2i_bio(ASN1_ITEM_rptr(X509_REQ), bp, req);
	}

int i2d_X509_REQ_bio(BIO *bp, X509_REQ *req)
	{
	return ASN1_item_i2d_bio(ASN1_ITEM_rptr(X509_REQ), bp, req);
	}

#ifndef OPENSSL_NO_RSA

#ifndef OPENSSL_NO_FP_API
RSA *d2i_RSAPrivateKey_fp(FILE *fp, RSA **rsa)
	{
	return ASN1_item_d2i_fp(ASN1_ITEM_rptr(RSAPrivateKey), fp, rsa);
	}

int i2d_RSAPrivateKey_fp(FILE *fp, RSA *rsa)
	{
	return ASN1_item_i2d_fp(ASN1_ITEM_rptr(RSAPrivateKey), fp, rsa);
	}

RSA *d2i_RSAPublicKey_fp(FILE *fp, RSA **rsa)
	{
	return ASN1_item_d2i_fp(ASN1_ITEM_rptr(RSAPublicKey), fp, rsa);
	}


RSA *d2i_RSA_PUBKEY_fp(FILE *fp, RSA **rsa)
	{
	return ASN1_d2i_fp((void *(*)(void))
			   RSA_new,(D2I_OF(void))d2i_RSA_PUBKEY, fp,
			   (void **)rsa);
	}

int i2d_RSAPublicKey_fp(FILE *fp, RSA *rsa)
	{
	return ASN1_item_i2d_fp(ASN1_ITEM_rptr(RSAPublicKey), fp, rsa);
	}

int i2d_RSA_PUBKEY_fp(FILE *fp, RSA *rsa)
	{
	return ASN1_i2d_fp((I2D_OF(void))i2d_RSA_PUBKEY,fp,rsa);
	}
#endif

RSA *d2i_RSAPrivateKey_bio(BIO *bp, RSA **rsa)
	{
	return ASN1_item_d2i_bio(ASN1_ITEM_rptr(RSAPrivateKey), bp, rsa);
	}

int i2d_RSAPrivateKey_bio(BIO *bp, RSA *rsa)
	{
	return ASN1_item_i2d_bio(ASN1_ITEM_rptr(RSAPrivateKey), bp, rsa);
	}

RSA *d2i_RSAPublicKey_bio(BIO *bp, RSA **rsa)
	{
	return ASN1_item_d2i_bio(ASN1_ITEM_rptr(RSAPublicKey), bp, rsa);
	}


RSA *d2i_RSA_PUBKEY_bio(BIO *bp, RSA **rsa)
	{
	return ASN1_d2i_bio_of(RSA,RSA_new,d2i_RSA_PUBKEY,bp,rsa);
	}

int i2d_RSAPublicKey_bio(BIO *bp, RSA *rsa)
	{
	return ASN1_item_i2d_bio(ASN1_ITEM_rptr(RSAPublicKey), bp, rsa);
	}

int i2d_RSA_PUBKEY_bio(BIO *bp, RSA *rsa)
	{
	return ASN1_i2d_bio_of(RSA,i2d_RSA_PUBKEY,bp,rsa);
	}
#endif

#ifndef OPENSSL_NO_DSA
#ifndef OPENSSL_NO_FP_API
DSA *d2i_DSAPrivateKey_fp(FILE *fp, DSA **dsa)
	{
	return ASN1_d2i_fp_of(DSA,DSA_new,d2i_DSAPrivateKey,fp,dsa);
	}

int i2d_DSAPrivateKey_fp(FILE *fp, DSA *dsa)
	{
	return ASN1_i2d_fp_of_const(DSA,i2d_DSAPrivateKey,fp,dsa);
	}

DSA *d2i_DSA_PUBKEY_fp(FILE *fp, DSA **dsa)
	{
	return ASN1_d2i_fp_of(DSA,DSA_new,d2i_DSA_PUBKEY,fp,dsa);
	}

int i2d_DSA_PUBKEY_fp(FILE *fp, DSA *dsa)
	{
	return ASN1_i2d_fp_of(DSA,i2d_DSA_PUBKEY,fp,dsa);
	}
#endif

DSA *d2i_DSAPrivateKey_bio(BIO *bp, DSA **dsa)
	{
	return ASN1_d2i_bio_of(DSA,DSA_new,d2i_DSAPrivateKey,bp,dsa
);
	}

int i2d_DSAPrivateKey_bio(BIO *bp, DSA *dsa)
	{
	return ASN1_i2d_bio_of_const(DSA,i2d_DSAPrivateKey,bp,dsa);
	}

DSA *d2i_DSA_PUBKEY_bio(BIO *bp, DSA **dsa)
	{
	return ASN1_d2i_bio_of(DSA,DSA_new,d2i_DSA_PUBKEY,bp,dsa);
	}

int i2d_DSA_PUBKEY_bio(BIO *bp, DSA *dsa)
	{
	return ASN1_i2d_bio_of(DSA,i2d_DSA_PUBKEY,bp,dsa);
	}

#endif

#ifndef OPENSSL_NO_EC
#ifndef OPENSSL_NO_FP_API
EC_KEY *d2i_EC_PUBKEY_fp(FILE *fp, EC_KEY **eckey)
	{
	return ASN1_d2i_fp_of(EC_KEY,EC_KEY_new,d2i_EC_PUBKEY,fp,eckey);
	}
  
int i2d_EC_PUBKEY_fp(FILE *fp, EC_KEY *eckey)
	{
	return ASN1_i2d_fp_of(EC_KEY,i2d_EC_PUBKEY,fp,eckey);
	}

EC_KEY *d2i_ECPrivateKey_fp(FILE *fp, EC_KEY **eckey)
	{
	return ASN1_d2i_fp_of(EC_KEY,EC_KEY_new,d2i_ECPrivateKey,fp,eckey);
	}
  
int i2d_ECPrivateKey_fp(FILE *fp, EC_KEY *eckey)
	{
	return ASN1_i2d_fp_of(EC_KEY,i2d_ECPrivateKey,fp,eckey);
	}
#endif
EC_KEY *d2i_EC_PUBKEY_bio(BIO *bp, EC_KEY **eckey)
	{
	return ASN1_d2i_bio_of(EC_KEY,EC_KEY_new,d2i_EC_PUBKEY,bp,eckey);
	}
  
int i2d_EC_PUBKEY_bio(BIO *bp, EC_KEY *ecdsa)
	{
	return ASN1_i2d_bio_of(EC_KEY,i2d_EC_PUBKEY,bp,ecdsa);
	}

EC_KEY *d2i_ECPrivateKey_bio(BIO *bp, EC_KEY **eckey)
	{
	return ASN1_d2i_bio_of(EC_KEY,EC_KEY_new,d2i_ECPrivateKey,bp,eckey);
	}
  
int i2d_ECPrivateKey_bio(BIO *bp, EC_KEY *eckey)
	{
	return ASN1_i2d_bio_of(EC_KEY,i2d_ECPrivateKey,bp,eckey);
	}
#endif


int X509_pubkey_digest(const X509 *data, const EVP_MD *type, unsigned char *md,
	     unsigned int *len)
	{
	ASN1_BIT_STRING *key;
	key = X509_get0_pubkey_bitstr(data);
	if(!key) return 0;
	return EVP_Digest(key->data, key->length, md, len, type, NULL);
	}

int X509_digest(const X509 *data, const EVP_MD *type, unsigned char *md,
	     unsigned int *len)
	{
	return(ASN1_item_digest(ASN1_ITEM_rptr(X509),type,(char *)data,md,len));
	}

int X509_CRL_digest(const X509_CRL *data, const EVP_MD *type, unsigned char *md,
	     unsigned int *len)
	{
	return(ASN1_item_digest(ASN1_ITEM_rptr(X509_CRL),type,(char *)data,md,len));
	}

int X509_REQ_digest(const X509_REQ *data, const EVP_MD *type, unsigned char *md,
	     unsigned int *len)
	{
	return(ASN1_item_digest(ASN1_ITEM_rptr(X509_REQ),type,(char *)data,md,len));
	}

int X509_NAME_digest(const X509_NAME *data, const EVP_MD *type, unsigned char *md,
	     unsigned int *len)
	{
	return(ASN1_item_digest(ASN1_ITEM_rptr(X509_NAME),type,(char *)data,md,len));
	}

int PKCS7_ISSUER_AND_SERIAL_digest(PKCS7_ISSUER_AND_SERIAL *data, const EVP_MD *type,
	     unsigned char *md, unsigned int *len)
	{
	return(ASN1_item_digest(ASN1_ITEM_rptr(PKCS7_ISSUER_AND_SERIAL),type,
		(char *)data,md,len));
	}


#ifndef OPENSSL_NO_FP_API
X509_SIG *d2i_PKCS8_fp(FILE *fp, X509_SIG **p8)
	{
	return ASN1_d2i_fp_of(X509_SIG,X509_SIG_new,d2i_X509_SIG,fp,p8);
	}

int i2d_PKCS8_fp(FILE *fp, X509_SIG *p8)
	{
	return ASN1_i2d_fp_of(X509_SIG,i2d_X509_SIG,fp,p8);
	}
#endif

X509_SIG *d2i_PKCS8_bio(BIO *bp, X509_SIG **p8)
	{
	return ASN1_d2i_bio_of(X509_SIG,X509_SIG_new,d2i_X509_SIG,bp,p8);
	}

int i2d_PKCS8_bio(BIO *bp, X509_SIG *p8)
	{
	return ASN1_i2d_bio_of(X509_SIG,i2d_X509_SIG,bp,p8);
	}

#ifndef OPENSSL_NO_FP_API
PKCS8_PRIV_KEY_INFO *d2i_PKCS8_PRIV_KEY_INFO_fp(FILE *fp,
						 PKCS8_PRIV_KEY_INFO **p8inf)
	{
	return ASN1_d2i_fp_of(PKCS8_PRIV_KEY_INFO,PKCS8_PRIV_KEY_INFO_new,
			      d2i_PKCS8_PRIV_KEY_INFO,fp,p8inf);
	}

int i2d_PKCS8_PRIV_KEY_INFO_fp(FILE *fp, PKCS8_PRIV_KEY_INFO *p8inf)
	{
	return ASN1_i2d_fp_of(PKCS8_PRIV_KEY_INFO,i2d_PKCS8_PRIV_KEY_INFO,fp,
			      p8inf);
	}

int i2d_PKCS8PrivateKeyInfo_fp(FILE *fp, EVP_PKEY *key)
	{
	PKCS8_PRIV_KEY_INFO *p8inf;
	int ret;
	p8inf = EVP_PKEY2PKCS8(key);
	if(!p8inf) return 0;
	ret = i2d_PKCS8_PRIV_KEY_INFO_fp(fp, p8inf);
	PKCS8_PRIV_KEY_INFO_free(p8inf);
	return ret;
	}

int i2d_PrivateKey_fp(FILE *fp, EVP_PKEY *pkey)
	{
	return ASN1_i2d_fp_of(EVP_PKEY,i2d_PrivateKey,fp,pkey);
	}

EVP_PKEY *d2i_PrivateKey_fp(FILE *fp, EVP_PKEY **a)
{
	return ASN1_d2i_fp_of(EVP_PKEY,EVP_PKEY_new,d2i_AutoPrivateKey,fp,a);
}

int i2d_PUBKEY_fp(FILE *fp, EVP_PKEY *pkey)
	{
	return ASN1_i2d_fp_of(EVP_PKEY,i2d_PUBKEY,fp,pkey);
	}

EVP_PKEY *d2i_PUBKEY_fp(FILE *fp, EVP_PKEY **a)
{
	return ASN1_d2i_fp_of(EVP_PKEY,EVP_PKEY_new,d2i_PUBKEY,fp,a);
}

#endif

PKCS8_PRIV_KEY_INFO *d2i_PKCS8_PRIV_KEY_INFO_bio(BIO *bp,
						 PKCS8_PRIV_KEY_INFO **p8inf)
	{
	return ASN1_d2i_bio_of(PKCS8_PRIV_KEY_INFO,PKCS8_PRIV_KEY_INFO_new,
			    d2i_PKCS8_PRIV_KEY_INFO,bp,p8inf);
	}

int i2d_PKCS8_PRIV_KEY_INFO_bio(BIO *bp, PKCS8_PRIV_KEY_INFO *p8inf)
	{
	return ASN1_i2d_bio_of(PKCS8_PRIV_KEY_INFO,i2d_PKCS8_PRIV_KEY_INFO,bp,
			       p8inf);
	}

int i2d_PKCS8PrivateKeyInfo_bio(BIO *bp, EVP_PKEY *key)
	{
	PKCS8_PRIV_KEY_INFO *p8inf;
	int ret;
	p8inf = EVP_PKEY2PKCS8(key);
	if(!p8inf) return 0;
	ret = i2d_PKCS8_PRIV_KEY_INFO_bio(bp, p8inf);
	PKCS8_PRIV_KEY_INFO_free(p8inf);
	return ret;
	}

int i2d_PrivateKey_bio(BIO *bp, EVP_PKEY *pkey)
	{
	return ASN1_i2d_bio_of(EVP_PKEY,i2d_PrivateKey,bp,pkey);
	}

EVP_PKEY *d2i_PrivateKey_bio(BIO *bp, EVP_PKEY **a)
	{
	return ASN1_d2i_bio_of(EVP_PKEY,EVP_PKEY_new,d2i_AutoPrivateKey,bp,a);
	}

int i2d_PUBKEY_bio(BIO *bp, EVP_PKEY *pkey)
	{
	return ASN1_i2d_bio_of(EVP_PKEY,i2d_PUBKEY,bp,pkey);
	}

EVP_PKEY *d2i_PUBKEY_bio(BIO *bp, EVP_PKEY **a)
	{
	return ASN1_d2i_bio_of(EVP_PKEY,EVP_PKEY_new,d2i_PUBKEY,bp,a);
	}
