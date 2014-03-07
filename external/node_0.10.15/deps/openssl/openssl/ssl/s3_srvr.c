/* ssl/s3_srvr.c -*- mode:C; c-file-style: "eay" -*- */
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
/* ====================================================================
 * Copyright (c) 1998-2007 The OpenSSL Project.  All rights reserved.
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
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
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
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 *
 * Portions of the attached software ("Contribution") are developed by 
 * SUN MICROSYSTEMS, INC., and are contributed to the OpenSSL project.
 *
 * The Contribution is licensed pursuant to the OpenSSL open source
 * license provided above.
 *
 * ECC cipher suite support in OpenSSL originally written by
 * Vipul Gupta and Sumit Gupta of Sun Microsystems Laboratories.
 *
 */
/* ====================================================================
 * Copyright 2005 Nokia. All rights reserved.
 *
 * The portions of the attached software ("Contribution") is developed by
 * Nokia Corporation and is licensed pursuant to the OpenSSL open source
 * license.
 *
 * The Contribution, originally written by Mika Kousa and Pasi Eronen of
 * Nokia Corporation, consists of the "PSK" (Pre-Shared Key) ciphersuites
 * support (see RFC 4279) to OpenSSL.
 *
 * No patent licenses or other rights except those expressly stated in
 * the OpenSSL open source license shall be deemed granted or received
 * expressly, by implication, estoppel, or otherwise.
 *
 * No assurances are provided by Nokia that the Contribution does not
 * infringe the patent or other intellectual property rights of any third
 * party or that the license provides you with all the necessary rights
 * to make use of the Contribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. IN
 * ADDITION TO THE DISCLAIMERS INCLUDED IN THE LICENSE, NOKIA
 * SPECIFICALLY DISCLAIMS ANY LIABILITY FOR CLAIMS BROUGHT BY YOU OR ANY
 * OTHER ENTITY BASED ON INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS OR
 * OTHERWISE.
 */

#define REUSE_CIPHER_BUG
#define NETSCAPE_HANG_BUG

#include <stdio.h>
#include "ssl_locl.h"
#include "kssl_lcl.h"
#include <openssl/buffer.h>
#include <openssl/rand.h>
#include <openssl/objects.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/x509.h>
#ifndef OPENSSL_NO_DH
#include <openssl/dh.h>
#endif
#include <openssl/bn.h>
#ifndef OPENSSL_NO_KRB5
#include <openssl/krb5_asn.h>
#endif
#include <openssl/md5.h>

static const SSL_METHOD *ssl3_get_server_method(int ver);

static const SSL_METHOD *ssl3_get_server_method(int ver)
	{
	if (ver == SSL3_VERSION)
		return(SSLv3_server_method());
	else
		return(NULL);
	}

#ifndef OPENSSL_NO_SRP
static int ssl_check_srp_ext_ClientHello(SSL *s, int *al)
	{
	int ret = SSL_ERROR_NONE;

	*al = SSL_AD_UNRECOGNIZED_NAME;

	if ((s->s3->tmp.new_cipher->algorithm_mkey & SSL_kSRP) &&
	    (s->srp_ctx.TLS_ext_srp_username_callback != NULL))
		{
		if(s->srp_ctx.login == NULL)
			{
			/* RFC 5054 says SHOULD reject, 
			   we do so if There is no srp login name */
			ret = SSL3_AL_FATAL;
			*al = SSL_AD_UNKNOWN_PSK_IDENTITY;
			}
		else
			{
			ret = SSL_srp_server_param_with_username(s,al);
			}
		}
	return ret;
	}
#endif

IMPLEMENT_ssl3_meth_func(SSLv3_server_method,
			ssl3_accept,
			ssl_undefined_function,
			ssl3_get_server_method)

int ssl3_accept(SSL *s)
	{
	BUF_MEM *buf;
	unsigned long alg_k,Time=(unsigned long)time(NULL);
	void (*cb)(const SSL *ssl,int type,int val)=NULL;
	int ret= -1;
	int new_state,state,skip=0;

	RAND_add(&Time,sizeof(Time),0);
	ERR_clear_error();
	clear_sys_error();

	if (s->info_callback != NULL)
		cb=s->info_callback;
	else if (s->ctx->info_callback != NULL)
		cb=s->ctx->info_callback;

	/* init things to blank */
	s->in_handshake++;
	if (!SSL_in_init(s) || SSL_in_before(s)) SSL_clear(s);

	if (s->cert == NULL)
		{
		SSLerr(SSL_F_SSL3_ACCEPT,SSL_R_NO_CERTIFICATE_SET);
		return(-1);
		}

#ifndef OPENSSL_NO_HEARTBEATS
	/* If we're awaiting a HeartbeatResponse, pretend we
	 * already got and don't await it anymore, because
	 * Heartbeats don't make sense during handshakes anyway.
	 */
	if (s->tlsext_hb_pending)
		{
		s->tlsext_hb_pending = 0;
		s->tlsext_hb_seq++;
		}
#endif

	for (;;)
		{
		state=s->state;

		switch (s->state)
			{
		case SSL_ST_RENEGOTIATE:
			s->renegotiate=1;
			/* s->state=SSL_ST_ACCEPT; */

		case SSL_ST_BEFORE:
		case SSL_ST_ACCEPT:
		case SSL_ST_BEFORE|SSL_ST_ACCEPT:
		case SSL_ST_OK|SSL_ST_ACCEPT:

			s->server=1;
			if (cb != NULL) cb(s,SSL_CB_HANDSHAKE_START,1);

			if ((s->version>>8) != 3)
				{
				SSLerr(SSL_F_SSL3_ACCEPT, ERR_R_INTERNAL_ERROR);
				return -1;
				}
			s->type=SSL_ST_ACCEPT;

			if (s->init_buf == NULL)
				{
				if ((buf=BUF_MEM_new()) == NULL)
					{
					ret= -1;
					goto end;
					}
				if (!BUF_MEM_grow(buf,SSL3_RT_MAX_PLAIN_LENGTH))
					{
					ret= -1;
					goto end;
					}
				s->init_buf=buf;
				}

			if (!ssl3_setup_buffers(s))
				{
				ret= -1;
				goto end;
				}

			s->init_num=0;
			s->s3->flags &= ~SSL3_FLAGS_SGC_RESTART_DONE;

			if (s->state != SSL_ST_RENEGOTIATE)
				{
				/* Ok, we now need to push on a buffering BIO so that
				 * the output is sent in a way that TCP likes :-)
				 */
				if (!ssl_init_wbio_buffer(s,1)) { ret= -1; goto end; }
				
				ssl3_init_finished_mac(s);
				s->state=SSL3_ST_SR_CLNT_HELLO_A;
				s->ctx->stats.sess_accept++;
				}
			else if (!s->s3->send_connection_binding &&
				!(s->options & SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION))
				{
				/* Server attempting to renegotiate with
				 * client that doesn't support secure
				 * renegotiation.
				 */
				SSLerr(SSL_F_SSL3_ACCEPT, SSL_R_UNSAFE_LEGACY_RENEGOTIATION_DISABLED);
				ssl3_send_alert(s,SSL3_AL_FATAL,SSL_AD_HANDSHAKE_FAILURE);
				ret = -1;
				goto end;
				}
			else
				{
				/* s->state == SSL_ST_RENEGOTIATE,
				 * we will just send a HelloRequest */
				s->ctx->stats.sess_accept_renegotiate++;
				s->state=SSL3_ST_SW_HELLO_REQ_A;
				}
			break;

		case SSL3_ST_SW_HELLO_REQ_A:
		case SSL3_ST_SW_HELLO_REQ_B:

			s->shutdown=0;
			ret=ssl3_send_hello_request(s);
			if (ret <= 0) goto end;
			s->s3->tmp.next_state=SSL3_ST_SW_HELLO_REQ_C;
			s->state=SSL3_ST_SW_FLUSH;
			s->init_num=0;

			ssl3_init_finished_mac(s);
			break;

		case SSL3_ST_SW_HELLO_REQ_C:
			s->state=SSL_ST_OK;
			break;

		case SSL3_ST_SR_CLNT_HELLO_A:
		case SSL3_ST_SR_CLNT_HELLO_B:
		case SSL3_ST_SR_CLNT_HELLO_C:

			s->shutdown=0;
			if (s->rwstate != SSL_X509_LOOKUP)
			{
				ret=ssl3_get_client_hello(s);
				if (ret <= 0) goto end;
			}
#ifndef OPENSSL_NO_SRP
			{
			int al;
			if ((ret = ssl_check_srp_ext_ClientHello(s,&al))  < 0)
					{
					/* callback indicates firther work to be done */
					s->rwstate=SSL_X509_LOOKUP;
					goto end;
					}
			if (ret != SSL_ERROR_NONE)
				{
				ssl3_send_alert(s,SSL3_AL_FATAL,al);	
				/* This is not really an error but the only means to
                                   for a client to detect whether srp is supported. */
 				   if (al != TLS1_AD_UNKNOWN_PSK_IDENTITY) 	
					SSLerr(SSL_F_SSL3_ACCEPT,SSL_R_CLIENTHELLO_TLSEXT);			
				ret = SSL_TLSEXT_ERR_ALERT_FATAL;			
				ret= -1;
				goto end;	
				}
			}
#endif		
			
			s->renegotiate = 2;
			s->state=SSL3_ST_SW_SRVR_HELLO_A;
			s->init_num=0;
			break;

		case SSL3_ST_SW_SRVR_HELLO_A:
		case SSL3_ST_SW_SRVR_HELLO_B:
			ret=ssl3_send_server_hello(s);
			if (ret <= 0) goto end;
#ifndef OPENSSL_NO_TLSEXT
			if (s->hit)
				{
				if (s->tlsext_ticket_expected)
					s->state=SSL3_ST_SW_SESSION_TICKET_A;
				else
					s->state=SSL3_ST_SW_CHANGE_A;
				}
#else
			if (s->hit)
					s->state=SSL3_ST_SW_CHANGE_A;
#endif
			else
				s->state=SSL3_ST_SW_CERT_A;
			s->init_num=0;
			break;

		case SSL3_ST_SW_CERT_A:
		case SSL3_ST_SW_CERT_B:
			/* Check if it is anon DH or anon ECDH, */
			/* normal PSK or KRB5 or SRP */
			if (!(s->s3->tmp.new_cipher->algorithm_auth & SSL_aNULL)
				&& !(s->s3->tmp.new_cipher->algorithm_mkey & SSL_kPSK)
				&& !(s->s3->tmp.new_cipher->algorithm_auth & SSL_aKRB5))
				{
				ret=ssl3_send_server_certificate(s);
				if (ret <= 0) goto end;
#ifndef OPENSSL_NO_TLSEXT
				if (s->tlsext_status_expected)
					s->state=SSL3_ST_SW_CERT_STATUS_A;
				else
					s->state=SSL3_ST_SW_KEY_EXCH_A;
				}
			else
				{
				skip = 1;
				s->state=SSL3_ST_SW_KEY_EXCH_A;
				}
#else
				}
			else
				skip=1;

			s->state=SSL3_ST_SW_KEY_EXCH_A;
#endif
			s->init_num=0;
			break;

		case SSL3_ST_SW_KEY_EXCH_A:
		case SSL3_ST_SW_KEY_EXCH_B:
			alg_k = s->s3->tmp.new_cipher->algorithm_mkey;

			/* clear this, it may get reset by
			 * send_server_key_exchange */
			if ((s->options & SSL_OP_EPHEMERAL_RSA)
#ifndef OPENSSL_NO_KRB5
				&& !(alg_k & SSL_kKRB5)
#endif /* OPENSSL_NO_KRB5 */
				)
				/* option SSL_OP_EPHEMERAL_RSA sends temporary RSA key
				 * even when forbidden by protocol specs
				 * (handshake may fail as clients are not required to
				 * be able to handle this) */
				s->s3->tmp.use_rsa_tmp=1;
			else
				s->s3->tmp.use_rsa_tmp=0;


			/* only send if a DH key exchange, fortezza or
			 * RSA but we have a sign only certificate
			 *
			 * PSK: may send PSK identity hints
			 *
			 * For ECC ciphersuites, we send a serverKeyExchange
			 * message only if the cipher suite is either
			 * ECDH-anon or ECDHE. In other cases, the
			 * server certificate contains the server's
			 * public key for key exchange.
			 */
			if (s->s3->tmp.use_rsa_tmp
			/* PSK: send ServerKeyExchange if PSK identity
			 * hint if provided */
#ifndef OPENSSL_NO_PSK
			    || ((alg_k & SSL_kPSK) && s->ctx->psk_identity_hint)
#endif
#ifndef OPENSSL_NO_SRP
			    /* SRP: send ServerKeyExchange */
			    || (alg_k & SSL_kSRP)
#endif
			    || (alg_k & (SSL_kDHr|SSL_kDHd|SSL_kEDH))
			    || (alg_k & SSL_kEECDH)
			    || ((alg_k & SSL_kRSA)
				&& (s->cert->pkeys[SSL_PKEY_RSA_ENC].privatekey == NULL
				    || (SSL_C_IS_EXPORT(s->s3->tmp.new_cipher)
					&& EVP_PKEY_size(s->cert->pkeys[SSL_PKEY_RSA_ENC].privatekey)*8 > SSL_C_EXPORT_PKEYLENGTH(s->s3->tmp.new_cipher)
					)
				    )
				)
			    )
				{
				ret=ssl3_send_server_key_exchange(s);
				if (ret <= 0) goto end;
				}
			else
				skip=1;

			s->state=SSL3_ST_SW_CERT_REQ_A;
			s->init_num=0;
			break;

		case SSL3_ST_SW_CERT_REQ_A:
		case SSL3_ST_SW_CERT_REQ_B:
			if (/* don't request cert unless asked for it: */
				!(s->verify_mode & SSL_VERIFY_PEER) ||
				/* if SSL_VERIFY_CLIENT_ONCE is set,
				 * don't request cert during re-negotiation: */
				((s->session->peer != NULL) &&
				 (s->verify_mode & SSL_VERIFY_CLIENT_ONCE)) ||
				/* never request cert in anonymous ciphersuites
				 * (see section "Certificate request" in SSL 3 drafts
				 * and in RFC 2246): */
				((s->s3->tmp.new_cipher->algorithm_auth & SSL_aNULL) &&
				 /* ... except when the application insists on verification
				  * (against the specs, but s3_clnt.c accepts this for SSL 3) */
				 !(s->verify_mode & SSL_VERIFY_FAIL_IF_NO_PEER_CERT)) ||
				 /* never request cert in Kerberos ciphersuites */
				(s->s3->tmp.new_cipher->algorithm_auth & SSL_aKRB5)
				/* With normal PSK Certificates and
				 * Certificate Requests are omitted */
				|| (s->s3->tmp.new_cipher->algorithm_mkey & SSL_kPSK))
				{
				/* no cert request */
				skip=1;
				s->s3->tmp.cert_request=0;
				s->state=SSL3_ST_SW_SRVR_DONE_A;
				if (s->s3->handshake_buffer)
					if (!ssl3_digest_cached_records(s))
						return -1;
				}
			else
				{
				s->s3->tmp.cert_request=1;
				ret=ssl3_send_certificate_request(s);
				if (ret <= 0) goto end;
#ifndef NETSCAPE_HANG_BUG
				s->state=SSL3_ST_SW_SRVR_DONE_A;
#else
				s->state=SSL3_ST_SW_FLUSH;
				s->s3->tmp.next_state=SSL3_ST_SR_CERT_A;
#endif
				s->init_num=0;
				}
			break;

		case SSL3_ST_SW_SRVR_DONE_A:
		case SSL3_ST_SW_SRVR_DONE_B:
			ret=ssl3_send_server_done(s);
			if (ret <= 0) goto end;
			s->s3->tmp.next_state=SSL3_ST_SR_CERT_A;
			s->state=SSL3_ST_SW_FLUSH;
			s->init_num=0;
			break;
		
		case SSL3_ST_SW_FLUSH:

			/* This code originally checked to see if
			 * any data was pending using BIO_CTRL_INFO
			 * and then flushed. This caused problems
			 * as documented in PR#1939. The proposed
			 * fix doesn't completely resolve this issue
			 * as buggy implementations of BIO_CTRL_PENDING
			 * still exist. So instead we just flush
			 * unconditionally.
			 */

			s->rwstate=SSL_WRITING;
			if (BIO_flush(s->wbio) <= 0)
				{
				ret= -1;
				goto end;
				}
			s->rwstate=SSL_NOTHING;

			s->state=s->s3->tmp.next_state;
			break;

		case SSL3_ST_SR_CERT_A:
		case SSL3_ST_SR_CERT_B:
			/* Check for second client hello (MS SGC) */
			ret = ssl3_check_client_hello(s);
			if (ret <= 0)
				goto end;
			if (ret == 2)
				s->state = SSL3_ST_SR_CLNT_HELLO_C;
			else {
				if (s->s3->tmp.cert_request)
					{
					ret=ssl3_get_client_certificate(s);
					if (ret <= 0) goto end;
					}
				s->init_num=0;
				s->state=SSL3_ST_SR_KEY_EXCH_A;
			}
			break;

		case SSL3_ST_SR_KEY_EXCH_A:
		case SSL3_ST_SR_KEY_EXCH_B:
			ret=ssl3_get_client_key_exchange(s);
			if (ret <= 0)
				goto end;
			if (ret == 2)
				{
				/* For the ECDH ciphersuites when
				 * the client sends its ECDH pub key in
				 * a certificate, the CertificateVerify
				 * message is not sent.
				 * Also for GOST ciphersuites when
				 * the client uses its key from the certificate
				 * for key exchange.
				 */
#if defined(OPENSSL_NO_TLSEXT) || defined(OPENSSL_NO_NEXTPROTONEG)
				s->state=SSL3_ST_SR_FINISHED_A;
#else
				if (s->s3->next_proto_neg_seen)
					s->state=SSL3_ST_SR_NEXT_PROTO_A;
				else
					s->state=SSL3_ST_SR_FINISHED_A;
#endif
				s->init_num = 0;
				}
			else if (TLS1_get_version(s) >= TLS1_2_VERSION)
				{
				s->state=SSL3_ST_SR_CERT_VRFY_A;
				s->init_num=0;
				if (!s->session->peer)
					break;
				/* For TLS v1.2 freeze the handshake buffer
				 * at this point and digest cached records.
				 */
				if (!s->s3->handshake_buffer)
					{
					SSLerr(SSL_F_SSL3_ACCEPT,ERR_R_INTERNAL_ERROR);
					return -1;
					}
				s->s3->flags |= TLS1_FLAGS_KEEP_HANDSHAKE;
				if (!ssl3_digest_cached_records(s))
					return -1;
				}
			else
				{
				int offset=0;
				int dgst_num;

				s->state=SSL3_ST_SR_CERT_VRFY_A;
				s->init_num=0;

				/* We need to get hashes here so if there is
				 * a client cert, it can be verified
				 * FIXME - digest processing for CertificateVerify
				 * should be generalized. But it is next step
				 */
				if (s->s3->handshake_buffer)
					if (!ssl3_digest_cached_records(s))
						return -1;
				for (dgst_num=0; dgst_num<SSL_MAX_DIGEST;dgst_num++)	
					if (s->s3->handshake_dgst[dgst_num]) 
						{
						int dgst_size;

						s->method->ssl3_enc->cert_verify_mac(s,EVP_MD_CTX_type(s->s3->handshake_dgst[dgst_num]),&(s->s3->tmp.cert_verify_md[offset]));
						dgst_size=EVP_MD_CTX_size(s->s3->handshake_dgst[dgst_num]);
						if (dgst_size < 0)
							{
							ret = -1;
							goto end;
							}
						offset+=dgst_size;
						}		
				}
			break;

		case SSL3_ST_SR_CERT_VRFY_A:
		case SSL3_ST_SR_CERT_VRFY_B:

			/* we should decide if we expected this one */
			ret=ssl3_get_cert_verify(s);
			if (ret <= 0) goto end;

#if defined(OPENSSL_NO_TLSEXT) || defined(OPENSSL_NO_NEXTPROTONEG)
			s->state=SSL3_ST_SR_FINISHED_A;
#else
			if (s->s3->next_proto_neg_seen)
				s->state=SSL3_ST_SR_NEXT_PROTO_A;
			else
				s->state=SSL3_ST_SR_FINISHED_A;
#endif
			s->init_num=0;
			break;

#if !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)
		case SSL3_ST_SR_NEXT_PROTO_A:
		case SSL3_ST_SR_NEXT_PROTO_B:
			ret=ssl3_get_next_proto(s);
			if (ret <= 0) goto end;
			s->init_num = 0;
			s->state=SSL3_ST_SR_FINISHED_A;
			break;
#endif

		case SSL3_ST_SR_FINISHED_A:
		case SSL3_ST_SR_FINISHED_B:
			ret=ssl3_get_finished(s,SSL3_ST_SR_FINISHED_A,
				SSL3_ST_SR_FINISHED_B);
			if (ret <= 0) goto end;
			if (s->hit)
				s->state=SSL_ST_OK;
#ifndef OPENSSL_NO_TLSEXT
			else if (s->tlsext_ticket_expected)
				s->state=SSL3_ST_SW_SESSION_TICKET_A;
#endif
			else
				s->state=SSL3_ST_SW_CHANGE_A;
			s->init_num=0;
			break;

#ifndef OPENSSL_NO_TLSEXT
		case SSL3_ST_SW_SESSION_TICKET_A:
		case SSL3_ST_SW_SESSION_TICKET_B:
			ret=ssl3_send_newsession_ticket(s);
			if (ret <= 0) goto end;
			s->state=SSL3_ST_SW_CHANGE_A;
			s->init_num=0;
			break;

		case SSL3_ST_SW_CERT_STATUS_A:
		case SSL3_ST_SW_CERT_STATUS_B:
			ret=ssl3_send_cert_status(s);
			if (ret <= 0) goto end;
			s->state=SSL3_ST_SW_KEY_EXCH_A;
			s->init_num=0;
			break;

#endif

		case SSL3_ST_SW_CHANGE_A:
		case SSL3_ST_SW_CHANGE_B:

			s->session->cipher=s->s3->tmp.new_cipher;
			if (!s->method->ssl3_enc->setup_key_block(s))
				{ ret= -1; goto end; }

			ret=ssl3_send_change_cipher_spec(s,
				SSL3_ST_SW_CHANGE_A,SSL3_ST_SW_CHANGE_B);

			if (ret <= 0) goto end;
			s->state=SSL3_ST_SW_FINISHED_A;
			s->init_num=0;

			if (!s->method->ssl3_enc->change_cipher_state(s,
				SSL3_CHANGE_CIPHER_SERVER_WRITE))
				{
				ret= -1;
				goto end;
				}

			break;

		case SSL3_ST_SW_FINISHED_A:
		case SSL3_ST_SW_FINISHED_B:
			ret=ssl3_send_finished(s,
				SSL3_ST_SW_FINISHED_A,SSL3_ST_SW_FINISHED_B,
				s->method->ssl3_enc->server_finished_label,
				s->method->ssl3_enc->server_finished_label_len);
			if (ret <= 0) goto end;
			s->state=SSL3_ST_SW_FLUSH;
			if (s->hit)
				{
#if defined(OPENSSL_NO_TLSEXT) || defined(OPENSSL_NO_NEXTPROTONEG)
				s->s3->tmp.next_state=SSL3_ST_SR_FINISHED_A;
#else
				if (s->s3->next_proto_neg_seen)
					s->s3->tmp.next_state=SSL3_ST_SR_NEXT_PROTO_A;
				else
					s->s3->tmp.next_state=SSL3_ST_SR_FINISHED_A;
#endif
				}
			else
				s->s3->tmp.next_state=SSL_ST_OK;
			s->init_num=0;
			break;

		case SSL_ST_OK:
			/* clean a few things up */
			ssl3_cleanup_key_block(s);

			BUF_MEM_free(s->init_buf);
			s->init_buf=NULL;

			/* remove buffering on output */
			ssl_free_wbio_buffer(s);

			s->init_num=0;

			if (s->renegotiate == 2) /* skipped if we just sent a HelloRequest */
				{
				s->renegotiate=0;
				s->new_session=0;
				
				ssl_update_cache(s,SSL_SESS_CACHE_SERVER);
				
				s->ctx->stats.sess_accept_good++;
				/* s->server=1; */
				s->handshake_func=ssl3_accept;

				if (cb != NULL) cb(s,SSL_CB_HANDSHAKE_DONE,1);
				}
			
			ret = 1;
			goto end;
			/* break; */

		default:
			SSLerr(SSL_F_SSL3_ACCEPT,SSL_R_UNKNOWN_STATE);
			ret= -1;
			goto end;
			/* break; */
			}
		
		if (!s->s3->tmp.reuse_message && !skip)
			{
			if (s->debug)
				{
				if ((ret=BIO_flush(s->wbio)) <= 0)
					goto end;
				}


			if ((cb != NULL) && (s->state != state))
				{
				new_state=s->state;
				s->state=state;
				cb(s,SSL_CB_ACCEPT_LOOP,1);
				s->state=new_state;
				}
			}
		skip=0;
		}
end:
	/* BIO_flush(s->wbio); */

	s->in_handshake--;
	if (cb != NULL)
		cb(s,SSL_CB_ACCEPT_EXIT,ret);
	return(ret);
	}

int ssl3_send_hello_request(SSL *s)
	{
	unsigned char *p;

	if (s->state == SSL3_ST_SW_HELLO_REQ_A)
		{
		p=(unsigned char *)s->init_buf->data;
		*(p++)=SSL3_MT_HELLO_REQUEST;
		*(p++)=0;
		*(p++)=0;
		*(p++)=0;

		s->state=SSL3_ST_SW_HELLO_REQ_B;
		/* number of bytes to write */
		s->init_num=4;
		s->init_off=0;
		}

	/* SSL3_ST_SW_HELLO_REQ_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
	}

int ssl3_check_client_hello(SSL *s)
	{
	int ok;
	long n;

	/* this function is called when we really expect a Certificate message,
	 * so permit appropriate message length */
	n=s->method->ssl_get_message(s,
		SSL3_ST_SR_CERT_A,
		SSL3_ST_SR_CERT_B,
		-1,
		s->max_cert_list,
		&ok);
	if (!ok) return((int)n);
	s->s3->tmp.reuse_message = 1;
	if (s->s3->tmp.message_type == SSL3_MT_CLIENT_HELLO)
		{
		/* We only allow the client to restart the handshake once per
		 * negotiation. */
		if (s->s3->flags & SSL3_FLAGS_SGC_RESTART_DONE)
			{
			SSLerr(SSL_F_SSL3_CHECK_CLIENT_HELLO, SSL_R_MULTIPLE_SGC_RESTARTS);
			return -1;
			}
		/* Throw away what we have done so far in the current handshake,
		 * which will now be aborted. (A full SSL_clear would be too much.) */
#ifndef OPENSSL_NO_DH
		if (s->s3->tmp.dh != NULL)
			{
			DH_free(s->s3->tmp.dh);
			s->s3->tmp.dh = NULL;
			}
#endif
#ifndef OPENSSL_NO_ECDH
		if (s->s3->tmp.ecdh != NULL)
			{
			EC_KEY_free(s->s3->tmp.ecdh);
			s->s3->tmp.ecdh = NULL;
			}
#endif
		s->s3->flags |= SSL3_FLAGS_SGC_RESTART_DONE;
		return 2;
		}
	return 1;
}

int ssl3_get_client_hello(SSL *s)
	{
	int i,j,ok,al,ret= -1;
	unsigned int cookie_len;
	long n;
	unsigned long id;
	unsigned char *p,*d,*q;
	SSL_CIPHER *c;
#ifndef OPENSSL_NO_COMP
	SSL_COMP *comp=NULL;
#endif
	STACK_OF(SSL_CIPHER) *ciphers=NULL;

	/* We do this so that we will respond with our native type.
	 * If we are TLSv1 and we get SSLv3, we will respond with TLSv1,
	 * This down switching should be handled by a different method.
	 * If we are SSLv3, we will respond with SSLv3, even if prompted with
	 * TLSv1.
	 */
	if (s->state == SSL3_ST_SR_CLNT_HELLO_A
		)
		{
		s->state=SSL3_ST_SR_CLNT_HELLO_B;
		}
	s->first_packet=1;
	n=s->method->ssl_get_message(s,
		SSL3_ST_SR_CLNT_HELLO_B,
		SSL3_ST_SR_CLNT_HELLO_C,
		SSL3_MT_CLIENT_HELLO,
		SSL3_RT_MAX_PLAIN_LENGTH,
		&ok);

	if (!ok) return((int)n);
	s->first_packet=0;
	d=p=(unsigned char *)s->init_msg;

	/* use version from inside client hello, not from record header
	 * (may differ: see RFC 2246, Appendix E, second paragraph) */
	s->client_version=(((int)p[0])<<8)|(int)p[1];
	p+=2;

	if ((s->version == DTLS1_VERSION && s->client_version > s->version) ||
	    (s->version != DTLS1_VERSION && s->client_version < s->version))
		{
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, SSL_R_WRONG_VERSION_NUMBER);
		if ((s->client_version>>8) == SSL3_VERSION_MAJOR)
			{
			/* similar to ssl3_get_record, send alert using remote version number */
			s->version = s->client_version;
			}
		al = SSL_AD_PROTOCOL_VERSION;
		goto f_err;
		}

	/* If we require cookies and this ClientHello doesn't
	 * contain one, just return since we do not want to
	 * allocate any memory yet. So check cookie length...
	 */
	if (SSL_get_options(s) & SSL_OP_COOKIE_EXCHANGE)
		{
		unsigned int session_length, cookie_length;
		
		session_length = *(p + SSL3_RANDOM_SIZE);
		cookie_length = *(p + SSL3_RANDOM_SIZE + session_length + 1);

		if (cookie_length == 0)
			return 1;
		}

	/* load the client random */
	memcpy(s->s3->client_random,p,SSL3_RANDOM_SIZE);
	p+=SSL3_RANDOM_SIZE;

	/* get the session-id */
	j= *(p++);

	s->hit=0;
	/* Versions before 0.9.7 always allow clients to resume sessions in renegotiation.
	 * 0.9.7 and later allow this by default, but optionally ignore resumption requests
	 * with flag SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION (it's a new flag rather
	 * than a change to default behavior so that applications relying on this for security
	 * won't even compile against older library versions).
	 *
	 * 1.0.1 and later also have a function SSL_renegotiate_abbreviated() to request
	 * renegotiation but not a new session (s->new_session remains unset): for servers,
	 * this essentially just means that the SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
	 * setting will be ignored.
	 */
	if ((s->new_session && (s->options & SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION)))
		{
		if (!ssl_get_new_session(s,1))
			goto err;
		}
	else
		{
		i=ssl_get_prev_session(s, p, j, d + n);
		if (i == 1)
			{ /* previous session */
			s->hit=1;
			}
		else if (i == -1)
			goto err;
		else /* i == 0 */
			{
			if (!ssl_get_new_session(s,1))
				goto err;
			}
		}

	p+=j;

	if (s->version == DTLS1_VERSION || s->version == DTLS1_BAD_VER)
		{
		/* cookie stuff */
		cookie_len = *(p++);

		/* 
		 * The ClientHello may contain a cookie even if the
		 * HelloVerify message has not been sent--make sure that it
		 * does not cause an overflow.
		 */
		if ( cookie_len > sizeof(s->d1->rcvd_cookie))
			{
			/* too much data */
			al = SSL_AD_DECODE_ERROR;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, SSL_R_COOKIE_MISMATCH);
			goto f_err;
			}

		/* verify the cookie if appropriate option is set. */
		if ((SSL_get_options(s) & SSL_OP_COOKIE_EXCHANGE) &&
			cookie_len > 0)
			{
			memcpy(s->d1->rcvd_cookie, p, cookie_len);

			if ( s->ctx->app_verify_cookie_cb != NULL)
				{
				if ( s->ctx->app_verify_cookie_cb(s, s->d1->rcvd_cookie,
					cookie_len) == 0)
					{
					al=SSL_AD_HANDSHAKE_FAILURE;
					SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, 
						SSL_R_COOKIE_MISMATCH);
					goto f_err;
					}
				/* else cookie verification succeeded */
				}
			else if ( memcmp(s->d1->rcvd_cookie, s->d1->cookie, 
						  s->d1->cookie_len) != 0) /* default verification */
				{
					al=SSL_AD_HANDSHAKE_FAILURE;
					SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, 
						SSL_R_COOKIE_MISMATCH);
					goto f_err;
				}

			ret = 2;
			}

		p += cookie_len;
		}

	n2s(p,i);
	if ((i == 0) && (j != 0))
		{
		/* we need a cipher if we are not resuming a session */
		al=SSL_AD_ILLEGAL_PARAMETER;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_CIPHERS_SPECIFIED);
		goto f_err;
		}
	if ((p+i) >= (d+n))
		{
		/* not enough data */
		al=SSL_AD_DECODE_ERROR;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_LENGTH_MISMATCH);
		goto f_err;
		}
	if ((i > 0) && (ssl_bytes_to_cipher_list(s,p,i,&(ciphers))
		== NULL))
		{
		goto err;
		}
	p+=i;

	/* If it is a hit, check that the cipher is in the list */
	if ((s->hit) && (i > 0))
		{
		j=0;
		id=s->session->cipher->id;

#ifdef CIPHER_DEBUG
		printf("client sent %d ciphers\n",sk_num(ciphers));
#endif
		for (i=0; i<sk_SSL_CIPHER_num(ciphers); i++)
			{
			c=sk_SSL_CIPHER_value(ciphers,i);
#ifdef CIPHER_DEBUG
			printf("client [%2d of %2d]:%s\n",
				i,sk_num(ciphers),SSL_CIPHER_get_name(c));
#endif
			if (c->id == id)
				{
				j=1;
				break;
				}
			}
/* Disabled because it can be used in a ciphersuite downgrade
 * attack: CVE-2010-4180.
 */
#if 0
		if (j == 0 && (s->options & SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG) && (sk_SSL_CIPHER_num(ciphers) == 1))
			{
			/* Special case as client bug workaround: the previously used cipher may
			 * not be in the current list, the client instead might be trying to
			 * continue using a cipher that before wasn't chosen due to server
			 * preferences.  We'll have to reject the connection if the cipher is not
			 * enabled, though. */
			c = sk_SSL_CIPHER_value(ciphers, 0);
			if (sk_SSL_CIPHER_find(SSL_get_ciphers(s), c) >= 0)
				{
				s->session->cipher = c;
				j = 1;
				}
			}
#endif
		if (j == 0)
			{
			/* we need to have the cipher in the cipher
			 * list if we are asked to reuse it */
			al=SSL_AD_ILLEGAL_PARAMETER;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_REQUIRED_CIPHER_MISSING);
			goto f_err;
			}
		}

	/* compression */
	i= *(p++);
	if ((p+i) > (d+n))
		{
		/* not enough data */
		al=SSL_AD_DECODE_ERROR;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_LENGTH_MISMATCH);
		goto f_err;
		}
	q=p;
	for (j=0; j<i; j++)
		{
		if (p[j] == 0) break;
		}

	p+=i;
	if (j >= i)
		{
		/* no compress */
		al=SSL_AD_DECODE_ERROR;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_COMPRESSION_SPECIFIED);
		goto f_err;
		}

#ifndef OPENSSL_NO_TLSEXT
	/* TLS extensions*/
	if (s->version >= SSL3_VERSION)
		{
		if (!ssl_parse_clienthello_tlsext(s,&p,d,n, &al))
			{
			/* 'al' set by ssl_parse_clienthello_tlsext */
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_PARSE_TLSEXT);
			goto f_err;
			}
		}
		if (ssl_check_clienthello_tlsext_early(s) <= 0) {
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_CLIENTHELLO_TLSEXT);
			goto err;
		}

	/* Check if we want to use external pre-shared secret for this
	 * handshake for not reused session only. We need to generate
	 * server_random before calling tls_session_secret_cb in order to allow
	 * SessionTicket processing to use it in key derivation. */
	{
		unsigned long Time;
		unsigned char *pos;
		Time=(unsigned long)time(NULL);			/* Time */
		pos=s->s3->server_random;
		l2n(Time,pos);
		if (RAND_pseudo_bytes(pos,SSL3_RANDOM_SIZE-4) <= 0)
			{
			al=SSL_AD_INTERNAL_ERROR;
			goto f_err;
			}
	}

	if (!s->hit && s->version >= TLS1_VERSION && s->tls_session_secret_cb)
		{
		SSL_CIPHER *pref_cipher=NULL;

		s->session->master_key_length=sizeof(s->session->master_key);
		if(s->tls_session_secret_cb(s, s->session->master_key, &s->session->master_key_length,
			ciphers, &pref_cipher, s->tls_session_secret_cb_arg))
			{
			s->hit=1;
			s->session->ciphers=ciphers;
			s->session->verify_result=X509_V_OK;

			ciphers=NULL;

			/* check if some cipher was preferred by call back */
			pref_cipher=pref_cipher ? pref_cipher : ssl3_choose_cipher(s, s->session->ciphers, SSL_get_ciphers(s));
			if (pref_cipher == NULL)
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_SHARED_CIPHER);
				goto f_err;
				}

			s->session->cipher=pref_cipher;

			if (s->cipher_list)
				sk_SSL_CIPHER_free(s->cipher_list);

			if (s->cipher_list_by_id)
				sk_SSL_CIPHER_free(s->cipher_list_by_id);

			s->cipher_list = sk_SSL_CIPHER_dup(s->session->ciphers);
			s->cipher_list_by_id = sk_SSL_CIPHER_dup(s->session->ciphers);
			}
		}
#endif

	/* Worst case, we will use the NULL compression, but if we have other
	 * options, we will now look for them.  We have i-1 compression
	 * algorithms from the client, starting at q. */
	s->s3->tmp.new_compression=NULL;
#ifndef OPENSSL_NO_COMP
	/* This only happens if we have a cache hit */
	if (s->session->compress_meth != 0)
		{
		int m, comp_id = s->session->compress_meth;
		/* Perform sanity checks on resumed compression algorithm */
		/* Can't disable compression */
		if (s->options & SSL_OP_NO_COMPRESSION)
			{
			al=SSL_AD_INTERNAL_ERROR;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_INCONSISTENT_COMPRESSION);
			goto f_err;
			}
		/* Look for resumed compression method */
		for (m = 0; m < sk_SSL_COMP_num(s->ctx->comp_methods); m++)
			{
			comp=sk_SSL_COMP_value(s->ctx->comp_methods,m);
			if (comp_id == comp->id)
				{
				s->s3->tmp.new_compression=comp;
				break;
				}
			}
		if (s->s3->tmp.new_compression == NULL)
			{
			al=SSL_AD_INTERNAL_ERROR;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_INVALID_COMPRESSION_ALGORITHM);
			goto f_err;
			}
		/* Look for resumed method in compression list */
		for (m = 0; m < i; m++)
			{
			if (q[m] == comp_id)
				break;
			}
		if (m >= i)
			{
			al=SSL_AD_ILLEGAL_PARAMETER;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_REQUIRED_COMPRESSSION_ALGORITHM_MISSING);
			goto f_err;
			}
		}
	else if (s->hit)
		comp = NULL;
	else if (!(s->options & SSL_OP_NO_COMPRESSION) && s->ctx->comp_methods)
		{ /* See if we have a match */
		int m,nn,o,v,done=0;

		nn=sk_SSL_COMP_num(s->ctx->comp_methods);
		for (m=0; m<nn; m++)
			{
			comp=sk_SSL_COMP_value(s->ctx->comp_methods,m);
			v=comp->id;
			for (o=0; o<i; o++)
				{
				if (v == q[o])
					{
					done=1;
					break;
					}
				}
			if (done) break;
			}
		if (done)
			s->s3->tmp.new_compression=comp;
		else
			comp=NULL;
		}
#else
	/* If compression is disabled we'd better not try to resume a session
	 * using compression.
	 */
	if (s->session->compress_meth != 0)
		{
		al=SSL_AD_INTERNAL_ERROR;
		SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_INCONSISTENT_COMPRESSION);
		goto f_err;
		}
#endif

	/* Given s->session->ciphers and SSL_get_ciphers, we must
	 * pick a cipher */

	if (!s->hit)
		{
#ifdef OPENSSL_NO_COMP
		s->session->compress_meth=0;
#else
		s->session->compress_meth=(comp == NULL)?0:comp->id;
#endif
		if (s->session->ciphers != NULL)
			sk_SSL_CIPHER_free(s->session->ciphers);
		s->session->ciphers=ciphers;
		if (ciphers == NULL)
			{
			al=SSL_AD_ILLEGAL_PARAMETER;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_CIPHERS_PASSED);
			goto f_err;
			}
		ciphers=NULL;
		c=ssl3_choose_cipher(s,s->session->ciphers,
				     SSL_get_ciphers(s));

		if (c == NULL)
			{
			al=SSL_AD_HANDSHAKE_FAILURE;
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO,SSL_R_NO_SHARED_CIPHER);
			goto f_err;
			}
		s->s3->tmp.new_cipher=c;
		}
	else
		{
		/* Session-id reuse */
#ifdef REUSE_CIPHER_BUG
		STACK_OF(SSL_CIPHER) *sk;
		SSL_CIPHER *nc=NULL;
		SSL_CIPHER *ec=NULL;

		if (s->options & SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG)
			{
			sk=s->session->ciphers;
			for (i=0; i<sk_SSL_CIPHER_num(sk); i++)
				{
				c=sk_SSL_CIPHER_value(sk,i);
				if (c->algorithm_enc & SSL_eNULL)
					nc=c;
				if (SSL_C_IS_EXPORT(c))
					ec=c;
				}
			if (nc != NULL)
				s->s3->tmp.new_cipher=nc;
			else if (ec != NULL)
				s->s3->tmp.new_cipher=ec;
			else
				s->s3->tmp.new_cipher=s->session->cipher;
			}
		else
#endif
		s->s3->tmp.new_cipher=s->session->cipher;
		}

	if (TLS1_get_version(s) < TLS1_2_VERSION || !(s->verify_mode & SSL_VERIFY_PEER))
		{
		if (!ssl3_digest_cached_records(s))
			{
			al = SSL_AD_INTERNAL_ERROR;
			goto f_err;
			}
		}
	
	/* we now have the following setup. 
	 * client_random
	 * cipher_list 		- our prefered list of ciphers
	 * ciphers 		- the clients prefered list of ciphers
	 * compression		- basically ignored right now
	 * ssl version is set	- sslv3
	 * s->session		- The ssl session has been setup.
	 * s->hit		- session reuse flag
	 * s->tmp.new_cipher	- the new cipher to use.
	 */

	/* Handles TLS extensions that we couldn't check earlier */
	if (s->version >= SSL3_VERSION)
		{
		if (ssl_check_clienthello_tlsext_late(s) <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_HELLO, SSL_R_CLIENTHELLO_TLSEXT);
			goto err;
			}
		}

	if (ret < 0) ret=1;
	if (0)
		{
f_err:
		ssl3_send_alert(s,SSL3_AL_FATAL,al);
		}
err:
	if (ciphers != NULL) sk_SSL_CIPHER_free(ciphers);
	return(ret);
	}

int ssl3_send_server_hello(SSL *s)
	{
	unsigned char *buf;
	unsigned char *p,*d;
	int i,sl;
	unsigned long l;
#ifdef OPENSSL_NO_TLSEXT
	unsigned long Time;
#endif

	if (s->state == SSL3_ST_SW_SRVR_HELLO_A)
		{
		buf=(unsigned char *)s->init_buf->data;
#ifdef OPENSSL_NO_TLSEXT
		p=s->s3->server_random;
		/* Generate server_random if it was not needed previously */
		Time=(unsigned long)time(NULL);			/* Time */
		l2n(Time,p);
		if (RAND_pseudo_bytes(p,SSL3_RANDOM_SIZE-4) <= 0)
			return -1;
#endif
		/* Do the message type and length last */
		d=p= &(buf[4]);

		*(p++)=s->version>>8;
		*(p++)=s->version&0xff;

		/* Random stuff */
		memcpy(p,s->s3->server_random,SSL3_RANDOM_SIZE);
		p+=SSL3_RANDOM_SIZE;

		/* There are several cases for the session ID to send
		 * back in the server hello:
		 * - For session reuse from the session cache,
		 *   we send back the old session ID.
		 * - If stateless session reuse (using a session ticket)
		 *   is successful, we send back the client's "session ID"
		 *   (which doesn't actually identify the session).
		 * - If it is a new session, we send back the new
		 *   session ID.
		 * - However, if we want the new session to be single-use,
		 *   we send back a 0-length session ID.
		 * s->hit is non-zero in either case of session reuse,
		 * so the following won't overwrite an ID that we're supposed
		 * to send back.
		 */
		if (!(s->ctx->session_cache_mode & SSL_SESS_CACHE_SERVER)
			&& !s->hit)
			s->session->session_id_length=0;

		sl=s->session->session_id_length;
		if (sl > (int)sizeof(s->session->session_id))
			{
			SSLerr(SSL_F_SSL3_SEND_SERVER_HELLO, ERR_R_INTERNAL_ERROR);
			return -1;
			}
		*(p++)=sl;
		memcpy(p,s->session->session_id,sl);
		p+=sl;

		/* put the cipher */
		i=ssl3_put_cipher_by_char(s->s3->tmp.new_cipher,p);
		p+=i;

		/* put the compression method */
#ifdef OPENSSL_NO_COMP
			*(p++)=0;
#else
		if (s->s3->tmp.new_compression == NULL)
			*(p++)=0;
		else
			*(p++)=s->s3->tmp.new_compression->id;
#endif
#ifndef OPENSSL_NO_TLSEXT
		if (ssl_prepare_serverhello_tlsext(s) <= 0)
			{
			SSLerr(SSL_F_SSL3_SEND_SERVER_HELLO,SSL_R_SERVERHELLO_TLSEXT);
			return -1;
			}
		if ((p = ssl_add_serverhello_tlsext(s, p, buf+SSL3_RT_MAX_PLAIN_LENGTH)) == NULL)
			{
			SSLerr(SSL_F_SSL3_SEND_SERVER_HELLO,ERR_R_INTERNAL_ERROR);
			return -1;
			}
#endif
		/* do the header */
		l=(p-d);
		d=buf;
		*(d++)=SSL3_MT_SERVER_HELLO;
		l2n3(l,d);

		s->state=SSL3_ST_SW_SRVR_HELLO_B;
		/* number of bytes to write */
		s->init_num=p-buf;
		s->init_off=0;
		}

	/* SSL3_ST_SW_SRVR_HELLO_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
	}

int ssl3_send_server_done(SSL *s)
	{
	unsigned char *p;

	if (s->state == SSL3_ST_SW_SRVR_DONE_A)
		{
		p=(unsigned char *)s->init_buf->data;

		/* do the header */
		*(p++)=SSL3_MT_SERVER_DONE;
		*(p++)=0;
		*(p++)=0;
		*(p++)=0;

		s->state=SSL3_ST_SW_SRVR_DONE_B;
		/* number of bytes to write */
		s->init_num=4;
		s->init_off=0;
		}

	/* SSL3_ST_SW_SRVR_DONE_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
	}

int ssl3_send_server_key_exchange(SSL *s)
	{
#ifndef OPENSSL_NO_RSA
	unsigned char *q;
	int j,num;
	RSA *rsa;
	unsigned char md_buf[MD5_DIGEST_LENGTH+SHA_DIGEST_LENGTH];
	unsigned int u;
#endif
#ifndef OPENSSL_NO_DH
	DH *dh=NULL,*dhp;
#endif
#ifndef OPENSSL_NO_ECDH
	EC_KEY *ecdh=NULL, *ecdhp;
	unsigned char *encodedPoint = NULL;
	int encodedlen = 0;
	int curve_id = 0;
	BN_CTX *bn_ctx = NULL; 
#endif
	EVP_PKEY *pkey;
	const EVP_MD *md = NULL;
	unsigned char *p,*d;
	int al,i;
	unsigned long type;
	int n;
	CERT *cert;
	BIGNUM *r[4];
	int nr[4],kn;
	BUF_MEM *buf;
	EVP_MD_CTX md_ctx;

	EVP_MD_CTX_init(&md_ctx);
	if (s->state == SSL3_ST_SW_KEY_EXCH_A)
		{
		type=s->s3->tmp.new_cipher->algorithm_mkey;
		cert=s->cert;

		buf=s->init_buf;

		r[0]=r[1]=r[2]=r[3]=NULL;
		n=0;
#ifndef OPENSSL_NO_RSA
		if (type & SSL_kRSA)
			{
			rsa=cert->rsa_tmp;
			if ((rsa == NULL) && (s->cert->rsa_tmp_cb != NULL))
				{
				rsa=s->cert->rsa_tmp_cb(s,
				      SSL_C_IS_EXPORT(s->s3->tmp.new_cipher),
				      SSL_C_EXPORT_PKEYLENGTH(s->s3->tmp.new_cipher));
				if(rsa == NULL)
				{
					al=SSL_AD_HANDSHAKE_FAILURE;
					SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_ERROR_GENERATING_TMP_RSA_KEY);
					goto f_err;
				}
				RSA_up_ref(rsa);
				cert->rsa_tmp=rsa;
				}
			if (rsa == NULL)
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_MISSING_TMP_RSA_KEY);
				goto f_err;
				}
			r[0]=rsa->n;
			r[1]=rsa->e;
			s->s3->tmp.use_rsa_tmp=1;
			}
		else
#endif
#ifndef OPENSSL_NO_DH
			if (type & SSL_kEDH)
			{
			dhp=cert->dh_tmp;
			if ((dhp == NULL) && (s->cert->dh_tmp_cb != NULL))
				dhp=s->cert->dh_tmp_cb(s,
				      SSL_C_IS_EXPORT(s->s3->tmp.new_cipher),
				      SSL_C_EXPORT_PKEYLENGTH(s->s3->tmp.new_cipher));
			if (dhp == NULL)
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_MISSING_TMP_DH_KEY);
				goto f_err;
				}

			if (s->s3->tmp.dh != NULL)
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE, ERR_R_INTERNAL_ERROR);
				goto err;
				}

			if ((dh=DHparams_dup(dhp)) == NULL)
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_DH_LIB);
				goto err;
				}

			s->s3->tmp.dh=dh;
			if ((dhp->pub_key == NULL ||
			     dhp->priv_key == NULL ||
			     (s->options & SSL_OP_SINGLE_DH_USE)))
				{
				if(!DH_generate_key(dh))
				    {
				    SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,
					   ERR_R_DH_LIB);
				    goto err;
				    }
				}
			else
				{
				dh->pub_key=BN_dup(dhp->pub_key);
				dh->priv_key=BN_dup(dhp->priv_key);
				if ((dh->pub_key == NULL) ||
					(dh->priv_key == NULL))
					{
					SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_DH_LIB);
					goto err;
					}
				}
			r[0]=dh->p;
			r[1]=dh->g;
			r[2]=dh->pub_key;
			}
		else 
#endif
#ifndef OPENSSL_NO_ECDH
			if (type & SSL_kEECDH)
			{
			const EC_GROUP *group;

			ecdhp=cert->ecdh_tmp;
			if ((ecdhp == NULL) && (s->cert->ecdh_tmp_cb != NULL))
				{
				ecdhp=s->cert->ecdh_tmp_cb(s,
				      SSL_C_IS_EXPORT(s->s3->tmp.new_cipher),
				      SSL_C_EXPORT_PKEYLENGTH(s->s3->tmp.new_cipher));
				}
			if (ecdhp == NULL)
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_MISSING_TMP_ECDH_KEY);
				goto f_err;
				}

			if (s->s3->tmp.ecdh != NULL)
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE, ERR_R_INTERNAL_ERROR);
				goto err;
				}

			/* Duplicate the ECDH structure. */
			if (ecdhp == NULL)
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_ECDH_LIB);
				goto err;
				}
			if ((ecdh = EC_KEY_dup(ecdhp)) == NULL)
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_ECDH_LIB);
				goto err;
				}

			s->s3->tmp.ecdh=ecdh;
			if ((EC_KEY_get0_public_key(ecdh) == NULL) ||
			    (EC_KEY_get0_private_key(ecdh) == NULL) ||
			    (s->options & SSL_OP_SINGLE_ECDH_USE))
				{
				if(!EC_KEY_generate_key(ecdh))
				    {
				    SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_ECDH_LIB);
				    goto err;
				    }
				}

			if (((group = EC_KEY_get0_group(ecdh)) == NULL) ||
			    (EC_KEY_get0_public_key(ecdh)  == NULL) ||
			    (EC_KEY_get0_private_key(ecdh) == NULL))
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_ECDH_LIB);
				goto err;
				}

			if (SSL_C_IS_EXPORT(s->s3->tmp.new_cipher) &&
			    (EC_GROUP_get_degree(group) > 163)) 
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_ECGROUP_TOO_LARGE_FOR_CIPHER);
				goto err;
				}

			/* XXX: For now, we only support ephemeral ECDH
			 * keys over named (not generic) curves. For 
			 * supported named curves, curve_id is non-zero.
			 */
			if ((curve_id = 
			    tls1_ec_nid2curve_id(EC_GROUP_get_curve_name(group)))
			    == 0)
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_UNSUPPORTED_ELLIPTIC_CURVE);
				goto err;
				}

			/* Encode the public key.
			 * First check the size of encoding and
			 * allocate memory accordingly.
			 */
			encodedlen = EC_POINT_point2oct(group, 
			    EC_KEY_get0_public_key(ecdh),
			    POINT_CONVERSION_UNCOMPRESSED, 
			    NULL, 0, NULL);

			encodedPoint = (unsigned char *) 
			    OPENSSL_malloc(encodedlen*sizeof(unsigned char)); 
			bn_ctx = BN_CTX_new();
			if ((encodedPoint == NULL) || (bn_ctx == NULL))
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_MALLOC_FAILURE);
				goto err;
				}


			encodedlen = EC_POINT_point2oct(group, 
			    EC_KEY_get0_public_key(ecdh), 
			    POINT_CONVERSION_UNCOMPRESSED, 
			    encodedPoint, encodedlen, bn_ctx);

			if (encodedlen == 0) 
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_ECDH_LIB);
				goto err;
				}

			BN_CTX_free(bn_ctx);  bn_ctx=NULL;

			/* XXX: For now, we only support named (not 
			 * generic) curves in ECDH ephemeral key exchanges.
			 * In this situation, we need four additional bytes
			 * to encode the entire ServerECDHParams
			 * structure. 
			 */
			n = 4 + encodedlen;

			/* We'll generate the serverKeyExchange message
			 * explicitly so we can set these to NULLs
			 */
			r[0]=NULL;
			r[1]=NULL;
			r[2]=NULL;
			r[3]=NULL;
			}
		else 
#endif /* !OPENSSL_NO_ECDH */
#ifndef OPENSSL_NO_PSK
			if (type & SSL_kPSK)
				{
				/* reserve size for record length and PSK identity hint*/
				n+=2+strlen(s->ctx->psk_identity_hint);
				}
			else
#endif /* !OPENSSL_NO_PSK */
#ifndef OPENSSL_NO_SRP
		if (type & SSL_kSRP)
			{
			if ((s->srp_ctx.N == NULL) ||
				(s->srp_ctx.g == NULL) ||
				(s->srp_ctx.s == NULL) ||
				(s->srp_ctx.B == NULL))
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_MISSING_SRP_PARAM);
				goto err;
				}
			r[0]=s->srp_ctx.N;
			r[1]=s->srp_ctx.g;
			r[2]=s->srp_ctx.s;
			r[3]=s->srp_ctx.B;
			}
		else 
#endif
			{
			al=SSL_AD_HANDSHAKE_FAILURE;
			SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_UNKNOWN_KEY_EXCHANGE_TYPE);
			goto f_err;
			}
		for (i=0; r[i] != NULL && i<4; i++)
			{
			nr[i]=BN_num_bytes(r[i]);
#ifndef OPENSSL_NO_SRP
			if ((i == 2) && (type & SSL_kSRP))
				n+=1+nr[i];
			else
#endif
			n+=2+nr[i];
			}

		if (!(s->s3->tmp.new_cipher->algorithm_auth & SSL_aNULL)
			&& !(s->s3->tmp.new_cipher->algorithm_mkey & SSL_kPSK))
			{
			if ((pkey=ssl_get_sign_pkey(s,s->s3->tmp.new_cipher,&md))
				== NULL)
				{
				al=SSL_AD_DECODE_ERROR;
				goto f_err;
				}
			kn=EVP_PKEY_size(pkey);
			}
		else
			{
			pkey=NULL;
			kn=0;
			}

		if (!BUF_MEM_grow_clean(buf,n+4+kn))
			{
			SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_LIB_BUF);
			goto err;
			}
		d=(unsigned char *)s->init_buf->data;
		p= &(d[4]);

		for (i=0; r[i] != NULL && i<4; i++)
			{
#ifndef OPENSSL_NO_SRP
			if ((i == 2) && (type & SSL_kSRP))
				{
				*p = nr[i];
				p++;
				}
			else
#endif
			s2n(nr[i],p);
			BN_bn2bin(r[i],p);
			p+=nr[i];
			}

#ifndef OPENSSL_NO_ECDH
		if (type & SSL_kEECDH) 
			{
			/* XXX: For now, we only support named (not generic) curves.
			 * In this situation, the serverKeyExchange message has:
			 * [1 byte CurveType], [2 byte CurveName]
			 * [1 byte length of encoded point], followed by
			 * the actual encoded point itself
			 */
			*p = NAMED_CURVE_TYPE;
			p += 1;
			*p = 0;
			p += 1;
			*p = curve_id;
			p += 1;
			*p = encodedlen;
			p += 1;
			memcpy((unsigned char*)p, 
			    (unsigned char *)encodedPoint, 
			    encodedlen);
			OPENSSL_free(encodedPoint);
			encodedPoint = NULL;
			p += encodedlen;
			}
#endif

#ifndef OPENSSL_NO_PSK
		if (type & SSL_kPSK)
			{
			/* copy PSK identity hint */
			s2n(strlen(s->ctx->psk_identity_hint), p); 
			strncpy((char *)p, s->ctx->psk_identity_hint, strlen(s->ctx->psk_identity_hint));
			p+=strlen(s->ctx->psk_identity_hint);
			}
#endif

		/* not anonymous */
		if (pkey != NULL)
			{
			/* n is the length of the params, they start at &(d[4])
			 * and p points to the space at the end. */
#ifndef OPENSSL_NO_RSA
			if (pkey->type == EVP_PKEY_RSA
					&& TLS1_get_version(s) < TLS1_2_VERSION)
				{
				q=md_buf;
				j=0;
				for (num=2; num > 0; num--)
					{
					EVP_MD_CTX_set_flags(&md_ctx,
						EVP_MD_CTX_FLAG_NON_FIPS_ALLOW);
					EVP_DigestInit_ex(&md_ctx,(num == 2)
						?s->ctx->md5:s->ctx->sha1, NULL);
					EVP_DigestUpdate(&md_ctx,&(s->s3->client_random[0]),SSL3_RANDOM_SIZE);
					EVP_DigestUpdate(&md_ctx,&(s->s3->server_random[0]),SSL3_RANDOM_SIZE);
					EVP_DigestUpdate(&md_ctx,&(d[4]),n);
					EVP_DigestFinal_ex(&md_ctx,q,
						(unsigned int *)&i);
					q+=i;
					j+=i;
					}
				if (RSA_sign(NID_md5_sha1, md_buf, j,
					&(p[2]), &u, pkey->pkey.rsa) <= 0)
					{
					SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_LIB_RSA);
					goto err;
					}
				s2n(u,p);
				n+=u+2;
				}
			else
#endif
			if (md)
				{
				/* For TLS1.2 and later send signature
				 * algorithm */
				if (TLS1_get_version(s) >= TLS1_2_VERSION)
					{
					if (!tls12_get_sigandhash(p, pkey, md))
						{
						/* Should never happen */
						al=SSL_AD_INTERNAL_ERROR;
						SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_R_INTERNAL_ERROR);
						goto f_err;
						}
					p+=2;
					}
#ifdef SSL_DEBUG
				fprintf(stderr, "Using hash %s\n",
							EVP_MD_name(md));
#endif
				EVP_SignInit_ex(&md_ctx, md, NULL);
				EVP_SignUpdate(&md_ctx,&(s->s3->client_random[0]),SSL3_RANDOM_SIZE);
				EVP_SignUpdate(&md_ctx,&(s->s3->server_random[0]),SSL3_RANDOM_SIZE);
				EVP_SignUpdate(&md_ctx,&(d[4]),n);
				if (!EVP_SignFinal(&md_ctx,&(p[2]),
					(unsigned int *)&i,pkey))
					{
					SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,ERR_LIB_EVP);
					goto err;
					}
				s2n(i,p);
				n+=i+2;
				if (TLS1_get_version(s) >= TLS1_2_VERSION)
					n+= 2;
				}
			else
				{
				/* Is this error check actually needed? */
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_SEND_SERVER_KEY_EXCHANGE,SSL_R_UNKNOWN_PKEY_TYPE);
				goto f_err;
				}
			}

		*(d++)=SSL3_MT_SERVER_KEY_EXCHANGE;
		l2n3(n,d);

		/* we should now have things packed up, so lets send
		 * it off */
		s->init_num=n+4;
		s->init_off=0;
		}

	s->state = SSL3_ST_SW_KEY_EXCH_B;
	EVP_MD_CTX_cleanup(&md_ctx);
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
f_err:
	ssl3_send_alert(s,SSL3_AL_FATAL,al);
err:
#ifndef OPENSSL_NO_ECDH
	if (encodedPoint != NULL) OPENSSL_free(encodedPoint);
	BN_CTX_free(bn_ctx);
#endif
	EVP_MD_CTX_cleanup(&md_ctx);
	return(-1);
	}

int ssl3_send_certificate_request(SSL *s)
	{
	unsigned char *p,*d;
	int i,j,nl,off,n;
	STACK_OF(X509_NAME) *sk=NULL;
	X509_NAME *name;
	BUF_MEM *buf;

	if (s->state == SSL3_ST_SW_CERT_REQ_A)
		{
		buf=s->init_buf;

		d=p=(unsigned char *)&(buf->data[4]);

		/* get the list of acceptable cert types */
		p++;
		n=ssl3_get_req_cert_type(s,p);
		d[0]=n;
		p+=n;
		n++;

		if (TLS1_get_version(s) >= TLS1_2_VERSION)
			{
			nl = tls12_get_req_sig_algs(s, p + 2);
			s2n(nl, p);
			p += nl + 2;
			n += nl + 2;
			}

		off=n;
		p+=2;
		n+=2;

		sk=SSL_get_client_CA_list(s);
		nl=0;
		if (sk != NULL)
			{
			for (i=0; i<sk_X509_NAME_num(sk); i++)
				{
				name=sk_X509_NAME_value(sk,i);
				j=i2d_X509_NAME(name,NULL);
				if (!BUF_MEM_grow_clean(buf,4+n+j+2))
					{
					SSLerr(SSL_F_SSL3_SEND_CERTIFICATE_REQUEST,ERR_R_BUF_LIB);
					goto err;
					}
				p=(unsigned char *)&(buf->data[4+n]);
				if (!(s->options & SSL_OP_NETSCAPE_CA_DN_BUG))
					{
					s2n(j,p);
					i2d_X509_NAME(name,&p);
					n+=2+j;
					nl+=2+j;
					}
				else
					{
					d=p;
					i2d_X509_NAME(name,&p);
					j-=2; s2n(j,d); j+=2;
					n+=j;
					nl+=j;
					}
				}
			}
		/* else no CA names */
		p=(unsigned char *)&(buf->data[4+off]);
		s2n(nl,p);

		d=(unsigned char *)buf->data;
		*(d++)=SSL3_MT_CERTIFICATE_REQUEST;
		l2n3(n,d);

		/* we should now have things packed up, so lets send
		 * it off */

		s->init_num=n+4;
		s->init_off=0;
#ifdef NETSCAPE_HANG_BUG
		p=(unsigned char *)s->init_buf->data + s->init_num;

		/* do the header */
		*(p++)=SSL3_MT_SERVER_DONE;
		*(p++)=0;
		*(p++)=0;
		*(p++)=0;
		s->init_num += 4;
#endif

		s->state = SSL3_ST_SW_CERT_REQ_B;
		}

	/* SSL3_ST_SW_CERT_REQ_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
err:
	return(-1);
	}

int ssl3_get_client_key_exchange(SSL *s)
	{
	int i,al,ok;
	long n;
	unsigned long alg_k;
	unsigned char *p;
#ifndef OPENSSL_NO_RSA
	RSA *rsa=NULL;
	EVP_PKEY *pkey=NULL;
#endif
#ifndef OPENSSL_NO_DH
	BIGNUM *pub=NULL;
	DH *dh_srvr;
#endif
#ifndef OPENSSL_NO_KRB5
	KSSL_ERR kssl_err;
#endif /* OPENSSL_NO_KRB5 */

#ifndef OPENSSL_NO_ECDH
	EC_KEY *srvr_ecdh = NULL;
	EVP_PKEY *clnt_pub_pkey = NULL;
	EC_POINT *clnt_ecpoint = NULL;
	BN_CTX *bn_ctx = NULL; 
#endif

	n=s->method->ssl_get_message(s,
		SSL3_ST_SR_KEY_EXCH_A,
		SSL3_ST_SR_KEY_EXCH_B,
		SSL3_MT_CLIENT_KEY_EXCHANGE,
		2048, /* ??? */
		&ok);

	if (!ok) return((int)n);
	p=(unsigned char *)s->init_msg;

	alg_k=s->s3->tmp.new_cipher->algorithm_mkey;

#ifndef OPENSSL_NO_RSA
	if (alg_k & SSL_kRSA)
		{
		/* FIX THIS UP EAY EAY EAY EAY */
		if (s->s3->tmp.use_rsa_tmp)
			{
			if ((s->cert != NULL) && (s->cert->rsa_tmp != NULL))
				rsa=s->cert->rsa_tmp;
			/* Don't do a callback because rsa_tmp should
			 * be sent already */
			if (rsa == NULL)
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_MISSING_TMP_RSA_PKEY);
				goto f_err;

				}
			}
		else
			{
			pkey=s->cert->pkeys[SSL_PKEY_RSA_ENC].privatekey;
			if (	(pkey == NULL) ||
				(pkey->type != EVP_PKEY_RSA) ||
				(pkey->pkey.rsa == NULL))
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_MISSING_RSA_CERTIFICATE);
				goto f_err;
				}
			rsa=pkey->pkey.rsa;
			}

		/* TLS and [incidentally] DTLS{0xFEFF} */
		if (s->version > SSL3_VERSION && s->version != DTLS1_BAD_VER)
			{
			n2s(p,i);
			if (n != i+2)
				{
				if (!(s->options & SSL_OP_TLS_D5_BUG))
					{
					SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_TLS_RSA_ENCRYPTED_VALUE_LENGTH_IS_WRONG);
					goto err;
					}
				else
					p-=2;
				}
			else
				n=i;
			}

		i=RSA_private_decrypt((int)n,p,p,rsa,RSA_PKCS1_PADDING);

		al = -1;
		
		if (i != SSL_MAX_MASTER_KEY_LENGTH)
			{
			al=SSL_AD_DECODE_ERROR;
			/* SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_BAD_RSA_DECRYPT); */
			}

		if ((al == -1) && !((p[0] == (s->client_version>>8)) && (p[1] == (s->client_version & 0xff))))
			{
			/* The premaster secret must contain the same version number as the
			 * ClientHello to detect version rollback attacks (strangely, the
			 * protocol does not offer such protection for DH ciphersuites).
			 * However, buggy clients exist that send the negotiated protocol
			 * version instead if the server does not support the requested
			 * protocol version.
			 * If SSL_OP_TLS_ROLLBACK_BUG is set, tolerate such clients. */
			if (!((s->options & SSL_OP_TLS_ROLLBACK_BUG) &&
				(p[0] == (s->version>>8)) && (p[1] == (s->version & 0xff))))
				{
				al=SSL_AD_DECODE_ERROR;
				/* SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_BAD_PROTOCOL_VERSION_NUMBER); */

				/* The Klima-Pokorny-Rosa extension of Bleichenbacher's attack
				 * (http://eprint.iacr.org/2003/052/) exploits the version
				 * number check as a "bad version oracle" -- an alert would
				 * reveal that the plaintext corresponding to some ciphertext
				 * made up by the adversary is properly formatted except
				 * that the version number is wrong.  To avoid such attacks,
				 * we should treat this just like any other decryption error. */
				}
			}

		if (al != -1)
			{
			/* Some decryption failure -- use random value instead as countermeasure
			 * against Bleichenbacher's attack on PKCS #1 v1.5 RSA padding
			 * (see RFC 2246, section 7.4.7.1). */
			ERR_clear_error();
			i = SSL_MAX_MASTER_KEY_LENGTH;
			p[0] = s->client_version >> 8;
			p[1] = s->client_version & 0xff;
			if (RAND_pseudo_bytes(p+2, i-2) <= 0) /* should be RAND_bytes, but we cannot work around a failure */
				goto err;
			}
	
		s->session->master_key_length=
			s->method->ssl3_enc->generate_master_secret(s,
				s->session->master_key,
				p,i);
		OPENSSL_cleanse(p,i);
		}
	else
#endif
#ifndef OPENSSL_NO_DH
		if (alg_k & (SSL_kEDH|SSL_kDHr|SSL_kDHd))
		{
		n2s(p,i);
		if (n != i+2)
			{
			if (!(s->options & SSL_OP_SSLEAY_080_CLIENT_DH_BUG))
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_DH_PUBLIC_VALUE_LENGTH_IS_WRONG);
				goto err;
				}
			else
				{
				p-=2;
				i=(int)n;
				}
			}

		if (n == 0L) /* the parameters are in the cert */
			{
			al=SSL_AD_HANDSHAKE_FAILURE;
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_UNABLE_TO_DECODE_DH_CERTS);
			goto f_err;
			}
		else
			{
			if (s->s3->tmp.dh == NULL)
				{
				al=SSL_AD_HANDSHAKE_FAILURE;
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_MISSING_TMP_DH_KEY);
				goto f_err;
				}
			else
				dh_srvr=s->s3->tmp.dh;
			}

		pub=BN_bin2bn(p,i,NULL);
		if (pub == NULL)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_BN_LIB);
			goto err;
			}

		i=DH_compute_key(p,pub,dh_srvr);

		if (i <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,ERR_R_DH_LIB);
			BN_clear_free(pub);
			goto err;
			}

		DH_free(s->s3->tmp.dh);
		s->s3->tmp.dh=NULL;

		BN_clear_free(pub);
		pub=NULL;
		s->session->master_key_length=
			s->method->ssl3_enc->generate_master_secret(s,
				s->session->master_key,p,i);
		OPENSSL_cleanse(p,i);
		}
	else
#endif
#ifndef OPENSSL_NO_KRB5
	if (alg_k & SSL_kKRB5)
		{
		krb5_error_code		krb5rc;
		krb5_data		enc_ticket;
		krb5_data		authenticator;
		krb5_data		enc_pms;
		KSSL_CTX		*kssl_ctx = s->kssl_ctx;
		EVP_CIPHER_CTX		ciph_ctx;
		const EVP_CIPHER	*enc = NULL;
		unsigned char		iv[EVP_MAX_IV_LENGTH];
		unsigned char		pms[SSL_MAX_MASTER_KEY_LENGTH
					       + EVP_MAX_BLOCK_LENGTH];
		int		     padl, outl;
		krb5_timestamp		authtime = 0;
		krb5_ticket_times	ttimes;

		EVP_CIPHER_CTX_init(&ciph_ctx);

		if (!kssl_ctx)  kssl_ctx = kssl_ctx_new();

		n2s(p,i);
		enc_ticket.length = i;

		if (n < (long)(enc_ticket.length + 6))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}

		enc_ticket.data = (char *)p;
		p+=enc_ticket.length;

		n2s(p,i);
		authenticator.length = i;

		if (n < (long)(enc_ticket.length + authenticator.length + 6))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}

		authenticator.data = (char *)p;
		p+=authenticator.length;

		n2s(p,i);
		enc_pms.length = i;
		enc_pms.data = (char *)p;
		p+=enc_pms.length;

		/* Note that the length is checked again below,
		** after decryption
		*/
		if(enc_pms.length > sizeof pms)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			       SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}

		if (n != (long)(enc_ticket.length + authenticator.length +
						enc_pms.length + 6))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}

		if ((krb5rc = kssl_sget_tkt(kssl_ctx, &enc_ticket, &ttimes,
					&kssl_err)) != 0)
			{
#ifdef KSSL_DEBUG
			printf("kssl_sget_tkt rtn %d [%d]\n",
				krb5rc, kssl_err.reason);
			if (kssl_err.text)
				printf("kssl_err text= %s\n", kssl_err.text);
#endif	/* KSSL_DEBUG */
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				kssl_err.reason);
			goto err;
			}

		/*  Note: no authenticator is not considered an error,
		**  but will return authtime == 0.
		*/
		if ((krb5rc = kssl_check_authent(kssl_ctx, &authenticator,
					&authtime, &kssl_err)) != 0)
			{
#ifdef KSSL_DEBUG
			printf("kssl_check_authent rtn %d [%d]\n",
				krb5rc, kssl_err.reason);
			if (kssl_err.text)
				printf("kssl_err text= %s\n", kssl_err.text);
#endif	/* KSSL_DEBUG */
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				kssl_err.reason);
			goto err;
			}

		if ((krb5rc = kssl_validate_times(authtime, &ttimes)) != 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE, krb5rc);
			goto err;
			}

#ifdef KSSL_DEBUG
		kssl_ctx_show(kssl_ctx);
#endif	/* KSSL_DEBUG */

		enc = kssl_map_enc(kssl_ctx->enctype);
		if (enc == NULL)
		    goto err;

		memset(iv, 0, sizeof iv);	/* per RFC 1510 */

		if (!EVP_DecryptInit_ex(&ciph_ctx,enc,NULL,kssl_ctx->key,iv))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DECRYPTION_FAILED);
			goto err;
			}
		if (!EVP_DecryptUpdate(&ciph_ctx, pms,&outl,
					(unsigned char *)enc_pms.data, enc_pms.length))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DECRYPTION_FAILED);
			goto err;
			}
		if (outl > SSL_MAX_MASTER_KEY_LENGTH)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}
		if (!EVP_DecryptFinal_ex(&ciph_ctx,&(pms[outl]),&padl))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DECRYPTION_FAILED);
			goto err;
			}
		outl += padl;
		if (outl > SSL_MAX_MASTER_KEY_LENGTH)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_DATA_LENGTH_TOO_LONG);
			goto err;
			}
		if (!((pms[0] == (s->client_version>>8)) && (pms[1] == (s->client_version & 0xff))))
		    {
		    /* The premaster secret must contain the same version number as the
		     * ClientHello to detect version rollback attacks (strangely, the
		     * protocol does not offer such protection for DH ciphersuites).
		     * However, buggy clients exist that send random bytes instead of
		     * the protocol version.
		     * If SSL_OP_TLS_ROLLBACK_BUG is set, tolerate such clients. 
		     * (Perhaps we should have a separate BUG value for the Kerberos cipher)
		     */
		    if (!(s->options & SSL_OP_TLS_ROLLBACK_BUG))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			       SSL_AD_DECODE_ERROR);
			goto err;
			}
		    }

		EVP_CIPHER_CTX_cleanup(&ciph_ctx);

		s->session->master_key_length=
			s->method->ssl3_enc->generate_master_secret(s,
				s->session->master_key, pms, outl);

		if (kssl_ctx->client_princ)
			{
			size_t len = strlen(kssl_ctx->client_princ);
			if ( len < SSL_MAX_KRB5_PRINCIPAL_LENGTH ) 
				{
				s->session->krb5_client_princ_len = len;
				memcpy(s->session->krb5_client_princ,kssl_ctx->client_princ,len);
				}
			}


		/*  Was doing kssl_ctx_free() here,
		**  but it caused problems for apache.
		**  kssl_ctx = kssl_ctx_free(kssl_ctx);
		**  if (s->kssl_ctx)  s->kssl_ctx = NULL;
		*/
		}
	else
#endif	/* OPENSSL_NO_KRB5 */

#ifndef OPENSSL_NO_ECDH
		if (alg_k & (SSL_kEECDH|SSL_kECDHr|SSL_kECDHe))
		{
		int ret = 1;
		int field_size = 0;
		const EC_KEY   *tkey;
		const EC_GROUP *group;
		const BIGNUM *priv_key;

		/* initialize structures for server's ECDH key pair */
		if ((srvr_ecdh = EC_KEY_new()) == NULL) 
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			    ERR_R_MALLOC_FAILURE);
			goto err;
			}

		/* Let's get server private key and group information */
		if (alg_k & (SSL_kECDHr|SSL_kECDHe))
			{ 
			/* use the certificate */
			tkey = s->cert->pkeys[SSL_PKEY_ECC].privatekey->pkey.ec;
			}
		else
			{
			/* use the ephermeral values we saved when
			 * generating the ServerKeyExchange msg.
			 */
			tkey = s->s3->tmp.ecdh;
			}

		group    = EC_KEY_get0_group(tkey);
		priv_key = EC_KEY_get0_private_key(tkey);

		if (!EC_KEY_set_group(srvr_ecdh, group) ||
		    !EC_KEY_set_private_key(srvr_ecdh, priv_key))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			       ERR_R_EC_LIB);
			goto err;
			}

		/* Let's get client's public key */
		if ((clnt_ecpoint = EC_POINT_new(group)) == NULL)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			    ERR_R_MALLOC_FAILURE);
			goto err;
			}

		if (n == 0L) 
			{
			/* Client Publickey was in Client Certificate */

			 if (alg_k & SSL_kEECDH)
				 {
				 al=SSL_AD_HANDSHAKE_FAILURE;
				 SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_MISSING_TMP_ECDH_KEY);
				 goto f_err;
				 }
			if (((clnt_pub_pkey=X509_get_pubkey(s->session->peer))
			    == NULL) || 
			    (clnt_pub_pkey->type != EVP_PKEY_EC))
				{
				/* XXX: For now, we do not support client
				 * authentication using ECDH certificates
				 * so this branch (n == 0L) of the code is
				 * never executed. When that support is
				 * added, we ought to ensure the key 
				 * received in the certificate is 
				 * authorized for key agreement.
				 * ECDH_compute_key implicitly checks that
				 * the two ECDH shares are for the same
				 * group.
				 */
			   	al=SSL_AD_HANDSHAKE_FAILURE;
			   	SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				    SSL_R_UNABLE_TO_DECODE_ECDH_CERTS);
			   	goto f_err;
			   	}

			if (EC_POINT_copy(clnt_ecpoint,
			    EC_KEY_get0_public_key(clnt_pub_pkey->pkey.ec)) == 0)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_EC_LIB);
				goto err;
				}
			ret = 2; /* Skip certificate verify processing */
			}
		else
			{
			/* Get client's public key from encoded point
			 * in the ClientKeyExchange message.
			 */
			if ((bn_ctx = BN_CTX_new()) == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				    ERR_R_MALLOC_FAILURE);
				goto err;
				}

			/* Get encoded point length */
			i = *p; 
			p += 1;
			if (n != 1 + i)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				    ERR_R_EC_LIB);
				goto err;
				}
			if (EC_POINT_oct2point(group, 
			    clnt_ecpoint, p, i, bn_ctx) == 0)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				    ERR_R_EC_LIB);
				goto err;
				}
			/* p is pointing to somewhere in the buffer
			 * currently, so set it to the start 
			 */ 
			p=(unsigned char *)s->init_buf->data;
			}

		/* Compute the shared pre-master secret */
		field_size = EC_GROUP_get_degree(group);
		if (field_size <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE, 
			       ERR_R_ECDH_LIB);
			goto err;
			}
		i = ECDH_compute_key(p, (field_size+7)/8, clnt_ecpoint, srvr_ecdh, NULL);
		if (i <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
			    ERR_R_ECDH_LIB);
			goto err;
			}

		EVP_PKEY_free(clnt_pub_pkey);
		EC_POINT_free(clnt_ecpoint);
		EC_KEY_free(srvr_ecdh);
		BN_CTX_free(bn_ctx);
		EC_KEY_free(s->s3->tmp.ecdh);
		s->s3->tmp.ecdh = NULL; 

		/* Compute the master secret */
		s->session->master_key_length = s->method->ssl3_enc-> \
		    generate_master_secret(s, s->session->master_key, p, i);
		
		OPENSSL_cleanse(p, i);
		return (ret);
		}
	else
#endif
#ifndef OPENSSL_NO_PSK
		if (alg_k & SSL_kPSK)
			{
			unsigned char *t = NULL;
			unsigned char psk_or_pre_ms[PSK_MAX_PSK_LEN*2+4];
			unsigned int pre_ms_len = 0, psk_len = 0;
			int psk_err = 1;
			char tmp_id[PSK_MAX_IDENTITY_LEN+1];

			al=SSL_AD_HANDSHAKE_FAILURE;

			n2s(p,i);
			if (n != i+2)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					SSL_R_LENGTH_MISMATCH);
				goto psk_err;
				}
			if (i > PSK_MAX_IDENTITY_LEN)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					SSL_R_DATA_LENGTH_TOO_LONG);
				goto psk_err;
				}
			if (s->psk_server_callback == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				       SSL_R_PSK_NO_SERVER_CB);
				goto psk_err;
				}

			/* Create guaranteed NULL-terminated identity
			 * string for the callback */
			memcpy(tmp_id, p, i);
			memset(tmp_id+i, 0, PSK_MAX_IDENTITY_LEN+1-i);
			psk_len = s->psk_server_callback(s, tmp_id,
				psk_or_pre_ms, sizeof(psk_or_pre_ms));
			OPENSSL_cleanse(tmp_id, PSK_MAX_IDENTITY_LEN+1);

			if (psk_len > PSK_MAX_PSK_LEN)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_INTERNAL_ERROR);
				goto psk_err;
				}
			else if (psk_len == 0)
				{
				/* PSK related to the given identity not found */
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				       SSL_R_PSK_IDENTITY_NOT_FOUND);
				al=SSL_AD_UNKNOWN_PSK_IDENTITY;
				goto psk_err;
				}

			/* create PSK pre_master_secret */
			pre_ms_len=2+psk_len+2+psk_len;
			t = psk_or_pre_ms;
			memmove(psk_or_pre_ms+psk_len+4, psk_or_pre_ms, psk_len);
			s2n(psk_len, t);
			memset(t, 0, psk_len);
			t+=psk_len;
			s2n(psk_len, t);

			if (s->session->psk_identity != NULL)
				OPENSSL_free(s->session->psk_identity);
			s->session->psk_identity = BUF_strdup((char *)p);
			if (s->session->psk_identity == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_MALLOC_FAILURE);
				goto psk_err;
				}

			if (s->session->psk_identity_hint != NULL)
				OPENSSL_free(s->session->psk_identity_hint);
			s->session->psk_identity_hint = BUF_strdup(s->ctx->psk_identity_hint);
			if (s->ctx->psk_identity_hint != NULL &&
				s->session->psk_identity_hint == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_MALLOC_FAILURE);
				goto psk_err;
				}

			s->session->master_key_length=
				s->method->ssl3_enc->generate_master_secret(s,
					s->session->master_key, psk_or_pre_ms, pre_ms_len);
			psk_err = 0;
		psk_err:
			OPENSSL_cleanse(psk_or_pre_ms, sizeof(psk_or_pre_ms));
			if (psk_err != 0)
				goto f_err;
			}
		else
#endif
#ifndef OPENSSL_NO_SRP
		if (alg_k & SSL_kSRP)
			{
			int param_len;

			n2s(p,i);
			param_len=i+2;
			if (param_len > n)
				{
				al=SSL_AD_DECODE_ERROR;
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_BAD_SRP_A_LENGTH);
				goto f_err;
				}
			if (!(s->srp_ctx.A=BN_bin2bn(p,i,NULL)))
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,ERR_R_BN_LIB);
				goto err;
				}
			if (s->session->srp_username != NULL)
				OPENSSL_free(s->session->srp_username);
			s->session->srp_username = BUF_strdup(s->srp_ctx.login);
			if (s->session->srp_username == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
					ERR_R_MALLOC_FAILURE);
				goto err;
				}

			if ((s->session->master_key_length = SRP_generate_server_master_secret(s,s->session->master_key))<0)
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,ERR_R_INTERNAL_ERROR);
				goto err;
				}

			p+=i;
			}
		else
#endif	/* OPENSSL_NO_SRP */
		if (alg_k & SSL_kGOST) 
			{
			int ret = 0;
			EVP_PKEY_CTX *pkey_ctx;
			EVP_PKEY *client_pub_pkey = NULL, *pk = NULL;
			unsigned char premaster_secret[32], *start;
			size_t outlen=32, inlen;
			unsigned long alg_a;

			/* Get our certificate private key*/
			alg_a = s->s3->tmp.new_cipher->algorithm_auth;
			if (alg_a & SSL_aGOST94)
				pk = s->cert->pkeys[SSL_PKEY_GOST94].privatekey;
			else if (alg_a & SSL_aGOST01)
				pk = s->cert->pkeys[SSL_PKEY_GOST01].privatekey;

			pkey_ctx = EVP_PKEY_CTX_new(pk,NULL);
			EVP_PKEY_decrypt_init(pkey_ctx);
			/* If client certificate is present and is of the same type, maybe
			 * use it for key exchange.  Don't mind errors from
			 * EVP_PKEY_derive_set_peer, because it is completely valid to use
			 * a client certificate for authorization only. */
			client_pub_pkey = X509_get_pubkey(s->session->peer);
			if (client_pub_pkey)
				{
				if (EVP_PKEY_derive_set_peer(pkey_ctx, client_pub_pkey) <= 0)
					ERR_clear_error();
				}
			/* Decrypt session key */
			if ((*p!=( V_ASN1_SEQUENCE| V_ASN1_CONSTRUCTED))) 
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_DECRYPTION_FAILED);
				goto gerr;
				}
			if (p[1] == 0x81)
				{
				start = p+3;
				inlen = p[2];
				}
			else if (p[1] < 0x80)
				{
				start = p+2;
				inlen = p[1];
				}
			else
				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_DECRYPTION_FAILED);
				goto gerr;
				}
			if (EVP_PKEY_decrypt(pkey_ctx,premaster_secret,&outlen,start,inlen) <=0) 

				{
				SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,SSL_R_DECRYPTION_FAILED);
				goto gerr;
				}
			/* Generate master secret */
			s->session->master_key_length=
				s->method->ssl3_enc->generate_master_secret(s,
					s->session->master_key,premaster_secret,32);
			/* Check if pubkey from client certificate was used */
			if (EVP_PKEY_CTX_ctrl(pkey_ctx, -1, -1, EVP_PKEY_CTRL_PEER_KEY, 2, NULL) > 0)
				ret = 2;
			else
				ret = 1;
		gerr:
			EVP_PKEY_free(client_pub_pkey);
			EVP_PKEY_CTX_free(pkey_ctx);
			if (ret)
				return ret;
			else
				goto err;
			}
		else
		{
		al=SSL_AD_HANDSHAKE_FAILURE;
		SSLerr(SSL_F_SSL3_GET_CLIENT_KEY_EXCHANGE,
				SSL_R_UNKNOWN_CIPHER_TYPE);
		goto f_err;
		}

	return(1);
f_err:
	ssl3_send_alert(s,SSL3_AL_FATAL,al);
#if !defined(OPENSSL_NO_DH) || !defined(OPENSSL_NO_RSA) || !defined(OPENSSL_NO_ECDH) || defined(OPENSSL_NO_SRP)
err:
#endif
#ifndef OPENSSL_NO_ECDH
	EVP_PKEY_free(clnt_pub_pkey);
	EC_POINT_free(clnt_ecpoint);
	if (srvr_ecdh != NULL) 
		EC_KEY_free(srvr_ecdh);
	BN_CTX_free(bn_ctx);
#endif
	return(-1);
	}

int ssl3_get_cert_verify(SSL *s)
	{
	EVP_PKEY *pkey=NULL;
	unsigned char *p;
	int al,ok,ret=0;
	long n;
	int type=0,i,j;
	X509 *peer;
	const EVP_MD *md = NULL;
	EVP_MD_CTX mctx;
	EVP_MD_CTX_init(&mctx);

	n=s->method->ssl_get_message(s,
		SSL3_ST_SR_CERT_VRFY_A,
		SSL3_ST_SR_CERT_VRFY_B,
		-1,
		516, /* Enough for 4096 bit RSA key with TLS v1.2 */
		&ok);

	if (!ok) return((int)n);

	if (s->session->peer != NULL)
		{
		peer=s->session->peer;
		pkey=X509_get_pubkey(peer);
		type=X509_certificate_type(peer,pkey);
		}
	else
		{
		peer=NULL;
		pkey=NULL;
		}

	if (s->s3->tmp.message_type != SSL3_MT_CERTIFICATE_VERIFY)
		{
		s->s3->tmp.reuse_message=1;
		if ((peer != NULL) && (type & EVP_PKT_SIGN))
			{
			al=SSL_AD_UNEXPECTED_MESSAGE;
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_MISSING_VERIFY_MESSAGE);
			goto f_err;
			}
		ret=1;
		goto end;
		}

	if (peer == NULL)
		{
		SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_NO_CLIENT_CERT_RECEIVED);
		al=SSL_AD_UNEXPECTED_MESSAGE;
		goto f_err;
		}

	if (!(type & EVP_PKT_SIGN))
		{
		SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_SIGNATURE_FOR_NON_SIGNING_CERTIFICATE);
		al=SSL_AD_ILLEGAL_PARAMETER;
		goto f_err;
		}

	if (s->s3->change_cipher_spec)
		{
		SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_CCS_RECEIVED_EARLY);
		al=SSL_AD_UNEXPECTED_MESSAGE;
		goto f_err;
		}

	/* we now have a signature that we need to verify */
	p=(unsigned char *)s->init_msg;
	/* Check for broken implementations of GOST ciphersuites */
	/* If key is GOST and n is exactly 64, it is bare
	 * signature without length field */
	if (n==64 && (pkey->type==NID_id_GostR3410_94 ||
		pkey->type == NID_id_GostR3410_2001) )
		{
		i=64;
		} 
	else 
		{	
		if (TLS1_get_version(s) >= TLS1_2_VERSION)
			{
			int sigalg = tls12_get_sigid(pkey);
			/* Should never happen */
			if (sigalg == -1)
				{
				SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,ERR_R_INTERNAL_ERROR);
				al=SSL_AD_INTERNAL_ERROR;
				goto f_err;
				}
			/* Check key type is consistent with signature */
			if (sigalg != (int)p[1])
				{
				SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_WRONG_SIGNATURE_TYPE);
				al=SSL_AD_DECODE_ERROR;
				goto f_err;
				}
			md = tls12_get_hash(p[0]);
			if (md == NULL)
				{
				SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_UNKNOWN_DIGEST);
				al=SSL_AD_DECODE_ERROR;
				goto f_err;
				}
#ifdef SSL_DEBUG
fprintf(stderr, "USING TLSv1.2 HASH %s\n", EVP_MD_name(md));
#endif
			p += 2;
			n -= 2;
			}
		n2s(p,i);
		n-=2;
		if (i > n)
			{
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_LENGTH_MISMATCH);
			al=SSL_AD_DECODE_ERROR;
			goto f_err;
			}
    	}
	j=EVP_PKEY_size(pkey);
	if ((i > j) || (n > j) || (n <= 0))
		{
		SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_WRONG_SIGNATURE_SIZE);
		al=SSL_AD_DECODE_ERROR;
		goto f_err;
		}

	if (TLS1_get_version(s) >= TLS1_2_VERSION)
		{
		long hdatalen = 0;
		void *hdata;
		hdatalen = BIO_get_mem_data(s->s3->handshake_buffer, &hdata);
		if (hdatalen <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY, ERR_R_INTERNAL_ERROR);
			al=SSL_AD_INTERNAL_ERROR;
			goto f_err;
			}
#ifdef SSL_DEBUG
		fprintf(stderr, "Using TLS 1.2 with client verify alg %s\n",
							EVP_MD_name(md));
#endif
		if (!EVP_VerifyInit_ex(&mctx, md, NULL)
			|| !EVP_VerifyUpdate(&mctx, hdata, hdatalen))
			{
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY, ERR_R_EVP_LIB);
			al=SSL_AD_INTERNAL_ERROR;
			goto f_err;
			}

		if (EVP_VerifyFinal(&mctx, p , i, pkey) <= 0)
			{
			al=SSL_AD_DECRYPT_ERROR;
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_BAD_SIGNATURE);
			goto f_err;
			}
		}
	else
#ifndef OPENSSL_NO_RSA 
	if (pkey->type == EVP_PKEY_RSA)
		{
		i=RSA_verify(NID_md5_sha1, s->s3->tmp.cert_verify_md,
			MD5_DIGEST_LENGTH+SHA_DIGEST_LENGTH, p, i, 
							pkey->pkey.rsa);
		if (i < 0)
			{
			al=SSL_AD_DECRYPT_ERROR;
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_BAD_RSA_DECRYPT);
			goto f_err;
			}
		if (i == 0)
			{
			al=SSL_AD_DECRYPT_ERROR;
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_BAD_RSA_SIGNATURE);
			goto f_err;
			}
		}
	else
#endif
#ifndef OPENSSL_NO_DSA
		if (pkey->type == EVP_PKEY_DSA)
		{
		j=DSA_verify(pkey->save_type,
			&(s->s3->tmp.cert_verify_md[MD5_DIGEST_LENGTH]),
			SHA_DIGEST_LENGTH,p,i,pkey->pkey.dsa);
		if (j <= 0)
			{
			/* bad signature */
			al=SSL_AD_DECRYPT_ERROR;
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,SSL_R_BAD_DSA_SIGNATURE);
			goto f_err;
			}
		}
	else
#endif
#ifndef OPENSSL_NO_ECDSA
		if (pkey->type == EVP_PKEY_EC)
		{
		j=ECDSA_verify(pkey->save_type,
			&(s->s3->tmp.cert_verify_md[MD5_DIGEST_LENGTH]),
			SHA_DIGEST_LENGTH,p,i,pkey->pkey.ec);
		if (j <= 0)
			{
			/* bad signature */
			al=SSL_AD_DECRYPT_ERROR;
			SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,
			    SSL_R_BAD_ECDSA_SIGNATURE);
			goto f_err;
			}
		}
	else
#endif
	if (pkey->type == NID_id_GostR3410_94 || pkey->type == NID_id_GostR3410_2001)
		{   unsigned char signature[64];
			int idx;
			EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new(pkey,NULL);
			EVP_PKEY_verify_init(pctx);
			if (i!=64) {
				fprintf(stderr,"GOST signature length is %d",i);
			}	
			for (idx=0;idx<64;idx++) {
				signature[63-idx]=p[idx];
			}	
			j=EVP_PKEY_verify(pctx,signature,64,s->s3->tmp.cert_verify_md,32);
			EVP_PKEY_CTX_free(pctx);
			if (j<=0) 
				{
				al=SSL_AD_DECRYPT_ERROR;
				SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,
					SSL_R_BAD_ECDSA_SIGNATURE);
				goto f_err;
				}	
		}
	else	
		{
		SSLerr(SSL_F_SSL3_GET_CERT_VERIFY,ERR_R_INTERNAL_ERROR);
		al=SSL_AD_UNSUPPORTED_CERTIFICATE;
		goto f_err;
		}


	ret=1;
	if (0)
		{
f_err:
		ssl3_send_alert(s,SSL3_AL_FATAL,al);
		}
end:
	if (s->s3->handshake_buffer)
		{
		BIO_free(s->s3->handshake_buffer);
		s->s3->handshake_buffer = NULL;
		s->s3->flags &= ~TLS1_FLAGS_KEEP_HANDSHAKE;
		}
	EVP_MD_CTX_cleanup(&mctx);
	EVP_PKEY_free(pkey);
	return(ret);
	}

int ssl3_get_client_certificate(SSL *s)
	{
	int i,ok,al,ret= -1;
	X509 *x=NULL;
	unsigned long l,nc,llen,n;
	const unsigned char *p,*q;
	unsigned char *d;
	STACK_OF(X509) *sk=NULL;

	n=s->method->ssl_get_message(s,
		SSL3_ST_SR_CERT_A,
		SSL3_ST_SR_CERT_B,
		-1,
		s->max_cert_list,
		&ok);

	if (!ok) return((int)n);

	if	(s->s3->tmp.message_type == SSL3_MT_CLIENT_KEY_EXCHANGE)
		{
		if (	(s->verify_mode & SSL_VERIFY_PEER) &&
			(s->verify_mode & SSL_VERIFY_FAIL_IF_NO_PEER_CERT))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE);
			al=SSL_AD_HANDSHAKE_FAILURE;
			goto f_err;
			}
		/* If tls asked for a client cert, the client must return a 0 list */
		if ((s->version > SSL3_VERSION) && s->s3->tmp.cert_request)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_TLS_PEER_DID_NOT_RESPOND_WITH_CERTIFICATE_LIST);
			al=SSL_AD_UNEXPECTED_MESSAGE;
			goto f_err;
			}
		s->s3->tmp.reuse_message=1;
		return(1);
		}

	if (s->s3->tmp.message_type != SSL3_MT_CERTIFICATE)
		{
		al=SSL_AD_UNEXPECTED_MESSAGE;
		SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_WRONG_MESSAGE_TYPE);
		goto f_err;
		}
	p=d=(unsigned char *)s->init_msg;

	if ((sk=sk_X509_new_null()) == NULL)
		{
		SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,ERR_R_MALLOC_FAILURE);
		goto err;
		}

	n2l3(p,llen);
	if (llen+3 != n)
		{
		al=SSL_AD_DECODE_ERROR;
		SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_LENGTH_MISMATCH);
		goto f_err;
		}
	for (nc=0; nc<llen; )
		{
		n2l3(p,l);
		if ((l+nc+3) > llen)
			{
			al=SSL_AD_DECODE_ERROR;
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_CERT_LENGTH_MISMATCH);
			goto f_err;
			}

		q=p;
		x=d2i_X509(NULL,&p,l);
		if (x == NULL)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,ERR_R_ASN1_LIB);
			goto err;
			}
		if (p != (q+l))
			{
			al=SSL_AD_DECODE_ERROR;
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_CERT_LENGTH_MISMATCH);
			goto f_err;
			}
		if (!sk_X509_push(sk,x))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,ERR_R_MALLOC_FAILURE);
			goto err;
			}
		x=NULL;
		nc+=l+3;
		}

	if (sk_X509_num(sk) <= 0)
		{
		/* TLS does not mind 0 certs returned */
		if (s->version == SSL3_VERSION)
			{
			al=SSL_AD_HANDSHAKE_FAILURE;
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_NO_CERTIFICATES_RETURNED);
			goto f_err;
			}
		/* Fail for TLS only if we required a certificate */
		else if ((s->verify_mode & SSL_VERIFY_PEER) &&
			 (s->verify_mode & SSL_VERIFY_FAIL_IF_NO_PEER_CERT))
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE);
			al=SSL_AD_HANDSHAKE_FAILURE;
			goto f_err;
			}
		/* No client certificate so digest cached records */
		if (s->s3->handshake_buffer && !ssl3_digest_cached_records(s))
			{
			al=SSL_AD_INTERNAL_ERROR;
			goto f_err;
			}
		}
	else
		{
		i=ssl_verify_cert_chain(s,sk);
		if (i <= 0)
			{
			al=ssl_verify_alarm_type(s->verify_result);
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE,SSL_R_NO_CERTIFICATE_RETURNED);
			goto f_err;
			}
		}

	if (s->session->peer != NULL) /* This should not be needed */
		X509_free(s->session->peer);
	s->session->peer=sk_X509_shift(sk);
	s->session->verify_result = s->verify_result;

	/* With the current implementation, sess_cert will always be NULL
	 * when we arrive here. */
	if (s->session->sess_cert == NULL)
		{
		s->session->sess_cert = ssl_sess_cert_new();
		if (s->session->sess_cert == NULL)
			{
			SSLerr(SSL_F_SSL3_GET_CLIENT_CERTIFICATE, ERR_R_MALLOC_FAILURE);
			goto err;
			}
		}
	if (s->session->sess_cert->cert_chain != NULL)
		sk_X509_pop_free(s->session->sess_cert->cert_chain, X509_free);
	s->session->sess_cert->cert_chain=sk;
	/* Inconsistency alert: cert_chain does *not* include the
	 * peer's own certificate, while we do include it in s3_clnt.c */

	sk=NULL;

	ret=1;
	if (0)
		{
f_err:
		ssl3_send_alert(s,SSL3_AL_FATAL,al);
		}
err:
	if (x != NULL) X509_free(x);
	if (sk != NULL) sk_X509_pop_free(sk,X509_free);
	return(ret);
	}

int ssl3_send_server_certificate(SSL *s)
	{
	unsigned long l;
	X509 *x;

	if (s->state == SSL3_ST_SW_CERT_A)
		{
		x=ssl_get_server_send_cert(s);
		if (x == NULL)
			{
			/* VRS: allow null cert if auth == KRB5 */
			if ((s->s3->tmp.new_cipher->algorithm_auth != SSL_aKRB5) ||
			    (s->s3->tmp.new_cipher->algorithm_mkey & SSL_kKRB5))
				{
				SSLerr(SSL_F_SSL3_SEND_SERVER_CERTIFICATE,ERR_R_INTERNAL_ERROR);
				return(0);
				}
			}

		l=ssl3_output_cert_chain(s,x);
		s->state=SSL3_ST_SW_CERT_B;
		s->init_num=(int)l;
		s->init_off=0;
		}

	/* SSL3_ST_SW_CERT_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
	}

#ifndef OPENSSL_NO_TLSEXT
/* send a new session ticket (not necessarily for a new session) */
int ssl3_send_newsession_ticket(SSL *s)
	{
	if (s->state == SSL3_ST_SW_SESSION_TICKET_A)
		{
		unsigned char *p, *senc, *macstart;
		const unsigned char *const_p;
		int len, slen_full, slen;
		SSL_SESSION *sess;
		unsigned int hlen;
		EVP_CIPHER_CTX ctx;
		HMAC_CTX hctx;
		SSL_CTX *tctx = s->initial_ctx;
		unsigned char iv[EVP_MAX_IV_LENGTH];
		unsigned char key_name[16];

		/* get session encoding length */
		slen_full = i2d_SSL_SESSION(s->session, NULL);
		/* Some length values are 16 bits, so forget it if session is
 		 * too long
 		 */
		if (slen_full > 0xFF00)
			return -1;
		senc = OPENSSL_malloc(slen_full);
		if (!senc)
			return -1;
		p = senc;
		i2d_SSL_SESSION(s->session, &p);

		/* create a fresh copy (not shared with other threads) to clean up */
		const_p = senc;
		sess = d2i_SSL_SESSION(NULL, &const_p, slen_full);
		if (sess == NULL)
			{
			OPENSSL_free(senc);
			return -1;
			}
		sess->session_id_length = 0; /* ID is irrelevant for the ticket */

		slen = i2d_SSL_SESSION(sess, NULL);
		if (slen > slen_full) /* shouldn't ever happen */
			{
			OPENSSL_free(senc);
			return -1;
			}
		p = senc;
		i2d_SSL_SESSION(sess, &p);
		SSL_SESSION_free(sess);

		/* Grow buffer if need be: the length calculation is as
 		 * follows 1 (size of message name) + 3 (message length
 		 * bytes) + 4 (ticket lifetime hint) + 2 (ticket length) +
 		 * 16 (key name) + max_iv_len (iv length) +
 		 * session_length + max_enc_block_size (max encrypted session
 		 * length) + max_md_size (HMAC).
 		 */
		if (!BUF_MEM_grow(s->init_buf,
			26 + EVP_MAX_IV_LENGTH + EVP_MAX_BLOCK_LENGTH +
			EVP_MAX_MD_SIZE + slen))
			return -1;

		p=(unsigned char *)s->init_buf->data;
		/* do the header */
		*(p++)=SSL3_MT_NEWSESSION_TICKET;
		/* Skip message length for now */
		p += 3;
		EVP_CIPHER_CTX_init(&ctx);
		HMAC_CTX_init(&hctx);
		/* Initialize HMAC and cipher contexts. If callback present
		 * it does all the work otherwise use generated values
		 * from parent ctx.
		 */
		if (tctx->tlsext_ticket_key_cb)
			{
			if (tctx->tlsext_ticket_key_cb(s, key_name, iv, &ctx,
							 &hctx, 1) < 0)
				{
				OPENSSL_free(senc);
				return -1;
				}
			}
		else
			{
			RAND_pseudo_bytes(iv, 16);
			EVP_EncryptInit_ex(&ctx, EVP_aes_128_cbc(), NULL,
					tctx->tlsext_tick_aes_key, iv);
			HMAC_Init_ex(&hctx, tctx->tlsext_tick_hmac_key, 16,
					tlsext_tick_md(), NULL);
			memcpy(key_name, tctx->tlsext_tick_key_name, 16);
			}

		/* Ticket lifetime hint (advisory only):
		 * We leave this unspecified for resumed session (for simplicity),
		 * and guess that tickets for new sessions will live as long
		 * as their sessions. */
		l2n(s->hit ? 0 : s->session->timeout, p);

		/* Skip ticket length for now */
		p += 2;
		/* Output key name */
		macstart = p;
		memcpy(p, key_name, 16);
		p += 16;
		/* output IV */
		memcpy(p, iv, EVP_CIPHER_CTX_iv_length(&ctx));
		p += EVP_CIPHER_CTX_iv_length(&ctx);
		/* Encrypt session data */
		EVP_EncryptUpdate(&ctx, p, &len, senc, slen);
		p += len;
		EVP_EncryptFinal(&ctx, p, &len);
		p += len;
		EVP_CIPHER_CTX_cleanup(&ctx);

		HMAC_Update(&hctx, macstart, p - macstart);
		HMAC_Final(&hctx, p, &hlen);
		HMAC_CTX_cleanup(&hctx);

		p += hlen;
		/* Now write out lengths: p points to end of data written */
		/* Total length */
		len = p - (unsigned char *)s->init_buf->data;
		p=(unsigned char *)s->init_buf->data + 1;
		l2n3(len - 4, p); /* Message length */
		p += 4;
		s2n(len - 10, p);  /* Ticket length */

		/* number of bytes to write */
		s->init_num= len;
		s->state=SSL3_ST_SW_SESSION_TICKET_B;
		s->init_off=0;
		OPENSSL_free(senc);
		}

	/* SSL3_ST_SW_SESSION_TICKET_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
	}

int ssl3_send_cert_status(SSL *s)
	{
	if (s->state == SSL3_ST_SW_CERT_STATUS_A)
		{
		unsigned char *p;
		/* Grow buffer if need be: the length calculation is as
 		 * follows 1 (message type) + 3 (message length) +
 		 * 1 (ocsp response type) + 3 (ocsp response length)
 		 * + (ocsp response)
 		 */
		if (!BUF_MEM_grow(s->init_buf, 8 + s->tlsext_ocsp_resplen))
			return -1;

		p=(unsigned char *)s->init_buf->data;

		/* do the header */
		*(p++)=SSL3_MT_CERTIFICATE_STATUS;
		/* message length */
		l2n3(s->tlsext_ocsp_resplen + 4, p);
		/* status type */
		*(p++)= s->tlsext_status_type;
		/* length of OCSP response */
		l2n3(s->tlsext_ocsp_resplen, p);
		/* actual response */
		memcpy(p, s->tlsext_ocsp_resp, s->tlsext_ocsp_resplen);
		/* number of bytes to write */
		s->init_num = 8 + s->tlsext_ocsp_resplen;
		s->state=SSL3_ST_SW_CERT_STATUS_B;
		s->init_off = 0;
		}

	/* SSL3_ST_SW_CERT_STATUS_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
	}

# ifndef OPENSSL_NO_NEXTPROTONEG
/* ssl3_get_next_proto reads a Next Protocol Negotiation handshake message. It
 * sets the next_proto member in s if found */
int ssl3_get_next_proto(SSL *s)
	{
	int ok;
	int proto_len, padding_len;
	long n;
	const unsigned char *p;

	/* Clients cannot send a NextProtocol message if we didn't see the
	 * extension in their ClientHello */
	if (!s->s3->next_proto_neg_seen)
		{
		SSLerr(SSL_F_SSL3_GET_NEXT_PROTO,SSL_R_GOT_NEXT_PROTO_WITHOUT_EXTENSION);
		return -1;
		}

	n=s->method->ssl_get_message(s,
		SSL3_ST_SR_NEXT_PROTO_A,
		SSL3_ST_SR_NEXT_PROTO_B,
		SSL3_MT_NEXT_PROTO,
		514,  /* See the payload format below */
		&ok);

	if (!ok)
		return((int)n);

	/* s->state doesn't reflect whether ChangeCipherSpec has been received
	 * in this handshake, but s->s3->change_cipher_spec does (will be reset
	 * by ssl3_get_finished). */
	if (!s->s3->change_cipher_spec)
		{
		SSLerr(SSL_F_SSL3_GET_NEXT_PROTO,SSL_R_GOT_NEXT_PROTO_BEFORE_A_CCS);
		return -1;
		}

	if (n < 2)
		return 0;  /* The body must be > 1 bytes long */

	p=(unsigned char *)s->init_msg;

	/* The payload looks like:
	 *   uint8 proto_len;
	 *   uint8 proto[proto_len];
	 *   uint8 padding_len;
	 *   uint8 padding[padding_len];
	 */
	proto_len = p[0];
	if (proto_len + 2 > s->init_num)
		return 0;
	padding_len = p[proto_len + 1];
	if (proto_len + padding_len + 2 != s->init_num)
		return 0;

	s->next_proto_negotiated = OPENSSL_malloc(proto_len);
	if (!s->next_proto_negotiated)
		{
		SSLerr(SSL_F_SSL3_GET_NEXT_PROTO,ERR_R_MALLOC_FAILURE);
		return 0;
		}
	memcpy(s->next_proto_negotiated, p + 1, proto_len);
	s->next_proto_negotiated_len = proto_len;

	return 1;
	}
# endif
#endif
