/* apps/dh.c */
/* obsoleted by dhparam.c */
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

#include <openssl/opensslconf.h>	/* for OPENSSL_NO_DH */
#ifndef OPENSSL_NO_DH
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "apps.h"
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#undef PROG
#define PROG	dh_main

/* -inform arg	- input format - default PEM (DER or PEM)
 * -outform arg - output format - default PEM
 * -in arg	- input file - default stdin
 * -out arg	- output file - default stdout
 * -check	- check the parameters are ok
 * -noout
 * -text
 * -C
 */

int MAIN(int, char **);

int MAIN(int argc, char **argv)
	{
	DH *dh=NULL;
	int i,badops=0,text=0;
	BIO *in=NULL,*out=NULL;
	int informat,outformat,check=0,noout=0,C=0,ret=1;
	char *infile,*outfile,*prog;
#ifndef OPENSSL_NO_ENGINE
	char *engine;
#endif

	apps_startup();

	if (bio_err == NULL)
		if ((bio_err=BIO_new(BIO_s_file())) != NULL)
			BIO_set_fp(bio_err,stderr,BIO_NOCLOSE|BIO_FP_TEXT);

	if (!load_config(bio_err, NULL))
		goto end;

#ifndef OPENSSL_NO_ENGINE
	engine=NULL;
#endif
	infile=NULL;
	outfile=NULL;
	informat=FORMAT_PEM;
	outformat=FORMAT_PEM;

	prog=argv[0];
	argc--;
	argv++;
	while (argc >= 1)
		{
		if 	(strcmp(*argv,"-inform") == 0)
			{
			if (--argc < 1) goto bad;
			informat=str2fmt(*(++argv));
			}
		else if (strcmp(*argv,"-outform") == 0)
			{
			if (--argc < 1) goto bad;
			outformat=str2fmt(*(++argv));
			}
		else if (strcmp(*argv,"-in") == 0)
			{
			if (--argc < 1) goto bad;
			infile= *(++argv);
			}
		else if (strcmp(*argv,"-out") == 0)
			{
			if (--argc < 1) goto bad;
			outfile= *(++argv);
			}
#ifndef OPENSSL_NO_ENGINE
		else if (strcmp(*argv,"-engine") == 0)
			{
			if (--argc < 1) goto bad;
			engine= *(++argv);
			}
#endif
		else if (strcmp(*argv,"-check") == 0)
			check=1;
		else if (strcmp(*argv,"-text") == 0)
			text=1;
		else if (strcmp(*argv,"-C") == 0)
			C=1;
		else if (strcmp(*argv,"-noout") == 0)
			noout=1;
		else
			{
			BIO_printf(bio_err,"unknown option %s\n",*argv);
			badops=1;
			break;
			}
		argc--;
		argv++;
		}

	if (badops)
		{
bad:
		BIO_printf(bio_err,"%s [options] <infile >outfile\n",prog);
		BIO_printf(bio_err,"where options are\n");
		BIO_printf(bio_err," -inform arg   input format - one of DER PEM\n");
		BIO_printf(bio_err," -outform arg  output format - one of DER PEM\n");
		BIO_printf(bio_err," -in arg       input file\n");
		BIO_printf(bio_err," -out arg      output file\n");
		BIO_printf(bio_err," -check        check the DH parameters\n");
		BIO_printf(bio_err," -text         print a text form of the DH parameters\n");
		BIO_printf(bio_err," -C            Output C code\n");
		BIO_printf(bio_err," -noout        no output\n");
#ifndef OPENSSL_NO_ENGINE
		BIO_printf(bio_err," -engine e     use engine e, possibly a hardware device.\n");
#endif
		goto end;
		}

	ERR_load_crypto_strings();

#ifndef OPENSSL_NO_ENGINE
        setup_engine(bio_err, engine, 0);
#endif

	in=BIO_new(BIO_s_file());
	out=BIO_new(BIO_s_file());
	if ((in == NULL) || (out == NULL))
		{
		ERR_print_errors(bio_err);
		goto end;
		}

	if (infile == NULL)
		BIO_set_fp(in,stdin,BIO_NOCLOSE);
	else
		{
		if (BIO_read_filename(in,infile) <= 0)
			{
			perror(infile);
			goto end;
			}
		}
	if (outfile == NULL)
		{
		BIO_set_fp(out,stdout,BIO_NOCLOSE);
#ifdef OPENSSL_SYS_VMS
		{
		BIO *tmpbio = BIO_new(BIO_f_linebuffer());
		out = BIO_push(tmpbio, out);
		}
#endif
		}
	else
		{
		if (BIO_write_filename(out,outfile) <= 0)
			{
			perror(outfile);
			goto end;
			}
		}

	if	(informat == FORMAT_ASN1)
		dh=d2i_DHparams_bio(in,NULL);
	else if (informat == FORMAT_PEM)
		dh=PEM_read_bio_DHparams(in,NULL,NULL,NULL);
	else
		{
		BIO_printf(bio_err,"bad input format specified\n");
		goto end;
		}
	if (dh == NULL)
		{
		BIO_printf(bio_err,"unable to load DH parameters\n");
		ERR_print_errors(bio_err);
		goto end;
		}

	

	if (text)
		{
		DHparams_print(out,dh);
#ifdef undef
		printf("p=");
		BN_print(stdout,dh->p);
		printf("\ng=");
		BN_print(stdout,dh->g);
		printf("\n");
		if (dh->length != 0)
			printf("recommended private length=%ld\n",dh->length);
#endif
		}
	
	if (check)
		{
		if (!DH_check(dh,&i))
			{
			ERR_print_errors(bio_err);
			goto end;
			}
		if (i & DH_CHECK_P_NOT_PRIME)
			printf("p value is not prime\n");
		if (i & DH_CHECK_P_NOT_SAFE_PRIME)
			printf("p value is not a safe prime\n");
		if (i & DH_UNABLE_TO_CHECK_GENERATOR)
			printf("unable to check the generator value\n");
		if (i & DH_NOT_SUITABLE_GENERATOR)
			printf("the g value is not a generator\n");
		if (i == 0)
			printf("DH parameters appear to be ok.\n");
		}
	if (C)
		{
		unsigned char *data;
		int len,l,bits;

		len=BN_num_bytes(dh->p);
		bits=BN_num_bits(dh->p);
		data=(unsigned char *)OPENSSL_malloc(len);
		if (data == NULL)
			{
			perror("OPENSSL_malloc");
			goto end;
			}
		l=BN_bn2bin(dh->p,data);
		printf("static unsigned char dh%d_p[]={",bits);
		for (i=0; i<l; i++)
			{
			if ((i%12) == 0) printf("\n\t");
			printf("0x%02X,",data[i]);
			}
		printf("\n\t};\n");

		l=BN_bn2bin(dh->g,data);
		printf("static unsigned char dh%d_g[]={",bits);
		for (i=0; i<l; i++)
			{
			if ((i%12) == 0) printf("\n\t");
			printf("0x%02X,",data[i]);
			}
		printf("\n\t};\n\n");

		printf("DH *get_dh%d()\n\t{\n",bits);
		printf("\tDH *dh;\n\n");
		printf("\tif ((dh=DH_new()) == NULL) return(NULL);\n");
		printf("\tdh->p=BN_bin2bn(dh%d_p,sizeof(dh%d_p),NULL);\n",
			bits,bits);
		printf("\tdh->g=BN_bin2bn(dh%d_g,sizeof(dh%d_g),NULL);\n",
			bits,bits);
		printf("\tif ((dh->p == NULL) || (dh->g == NULL))\n");
		printf("\t\treturn(NULL);\n");
		printf("\treturn(dh);\n\t}\n");
		OPENSSL_free(data);
		}


	if (!noout)
		{
		if 	(outformat == FORMAT_ASN1)
			i=i2d_DHparams_bio(out,dh);
		else if (outformat == FORMAT_PEM)
			i=PEM_write_bio_DHparams(out,dh);
		else	{
			BIO_printf(bio_err,"bad output format specified for outfile\n");
			goto end;
			}
		if (!i)
			{
			BIO_printf(bio_err,"unable to write DH parameters\n");
			ERR_print_errors(bio_err);
			goto end;
			}
		}
	ret=0;
end:
	if (in != NULL) BIO_free(in);
	if (out != NULL) BIO_free_all(out);
	if (dh != NULL) DH_free(dh);
	apps_shutdown();
	OPENSSL_EXIT(ret);
	}
#else /* !OPENSSL_NO_DH */

# if PEDANTIC
static void *dummy=&dummy;
# endif

#endif
