/* crypto/engine/eng_dyn.c */
/* Written by Geoff Thorpe (geoff@geoffthorpe.net) for the OpenSSL
 * project 2001.
 */
/* ====================================================================
 * Copyright (c) 1999-2001 The OpenSSL Project.  All rights reserved.
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


#include "eng_int.h"
#include <openssl/dso.h>

/* Shared libraries implementing ENGINEs for use by the "dynamic" ENGINE loader
 * should implement the hook-up functions with the following prototypes. */

/* Our ENGINE handlers */
static int dynamic_init(ENGINE *e);
static int dynamic_finish(ENGINE *e);
static int dynamic_ctrl(ENGINE *e, int cmd, long i, void *p, void (*f)(void));
/* Predeclare our context type */
typedef struct st_dynamic_data_ctx dynamic_data_ctx;
/* The implementation for the important control command */
static int dynamic_load(ENGINE *e, dynamic_data_ctx *ctx);

#define DYNAMIC_CMD_SO_PATH		ENGINE_CMD_BASE
#define DYNAMIC_CMD_NO_VCHECK		(ENGINE_CMD_BASE + 1)
#define DYNAMIC_CMD_ID			(ENGINE_CMD_BASE + 2)
#define DYNAMIC_CMD_LIST_ADD		(ENGINE_CMD_BASE + 3)
#define DYNAMIC_CMD_DIR_LOAD		(ENGINE_CMD_BASE + 4)
#define DYNAMIC_CMD_DIR_ADD		(ENGINE_CMD_BASE + 5)
#define DYNAMIC_CMD_LOAD		(ENGINE_CMD_BASE + 6)

/* The constants used when creating the ENGINE */
static const char *engine_dynamic_id = "dynamic";
static const char *engine_dynamic_name = "Dynamic engine loading support";
static const ENGINE_CMD_DEFN dynamic_cmd_defns[] = {
	{DYNAMIC_CMD_SO_PATH,
		"SO_PATH",
		"Specifies the path to the new ENGINE shared library",
		ENGINE_CMD_FLAG_STRING},
	{DYNAMIC_CMD_NO_VCHECK,
		"NO_VCHECK",
		"Specifies to continue even if version checking fails (boolean)",
		ENGINE_CMD_FLAG_NUMERIC},
	{DYNAMIC_CMD_ID,
		"ID",
		"Specifies an ENGINE id name for loading",
		ENGINE_CMD_FLAG_STRING},
	{DYNAMIC_CMD_LIST_ADD,
		"LIST_ADD",
		"Whether to add a loaded ENGINE to the internal list (0=no,1=yes,2=mandatory)",
		ENGINE_CMD_FLAG_NUMERIC},
	{DYNAMIC_CMD_DIR_LOAD,
		"DIR_LOAD",
		"Specifies whether to load from 'DIR_ADD' directories (0=no,1=yes,2=mandatory)",
		ENGINE_CMD_FLAG_NUMERIC},
	{DYNAMIC_CMD_DIR_ADD,
		"DIR_ADD",
		"Adds a directory from which ENGINEs can be loaded",
		ENGINE_CMD_FLAG_STRING},
	{DYNAMIC_CMD_LOAD,
		"LOAD",
		"Load up the ENGINE specified by other settings",
		ENGINE_CMD_FLAG_NO_INPUT},
	{0, NULL, NULL, 0}
	};
static const ENGINE_CMD_DEFN dynamic_cmd_defns_empty[] = {
	{0, NULL, NULL, 0}
	};

/* Loading code stores state inside the ENGINE structure via the "ex_data"
 * element. We load all our state into a single structure and use that as a
 * single context in the "ex_data" stack. */
struct st_dynamic_data_ctx
	{
	/* The DSO object we load that supplies the ENGINE code */
	DSO *dynamic_dso;
	/* The function pointer to the version checking shared library function */
	dynamic_v_check_fn v_check;
	/* The function pointer to the engine-binding shared library function */
	dynamic_bind_engine bind_engine;
	/* The default name/path for loading the shared library */
	const char *DYNAMIC_LIBNAME;
	/* Whether to continue loading on a version check failure */
	int no_vcheck;
	/* If non-NULL, stipulates the 'id' of the ENGINE to be loaded */
	const char *engine_id;
	/* If non-zero, a successfully loaded ENGINE should be added to the internal
	 * ENGINE list. If 2, the add must succeed or the entire load should fail. */
	int list_add_value;
	/* The symbol name for the version checking function */
	const char *DYNAMIC_F1;
	/* The symbol name for the "initialise ENGINE structure" function */
	const char *DYNAMIC_F2;
	/* Whether to never use 'dirs', use 'dirs' as a fallback, or only use
	 * 'dirs' for loading. Default is to use 'dirs' as a fallback. */
	int dir_load;
	/* A stack of directories from which ENGINEs could be loaded */
	STACK_OF(OPENSSL_STRING) *dirs;
	};

/* This is the "ex_data" index we obtain and reserve for use with our context
 * structure. */
static int dynamic_ex_data_idx = -1;

static void int_free_str(char *s) { OPENSSL_free(s); }
/* Because our ex_data element may or may not get allocated depending on whether
 * a "first-use" occurs before the ENGINE is freed, we have a memory leak
 * problem to solve. We can't declare a "new" handler for the ex_data as we
 * don't want a dynamic_data_ctx in *all* ENGINE structures of all types (this
 * is a bug in the design of CRYPTO_EX_DATA). As such, we just declare a "free"
 * handler and that will get called if an ENGINE is being destroyed and there
 * was an ex_data element corresponding to our context type. */
static void dynamic_data_ctx_free_func(void *parent, void *ptr,
			CRYPTO_EX_DATA *ad, int idx, long argl, void *argp)
	{
	if(ptr)
		{
		dynamic_data_ctx *ctx = (dynamic_data_ctx *)ptr;
		if(ctx->dynamic_dso)
			DSO_free(ctx->dynamic_dso);
		if(ctx->DYNAMIC_LIBNAME)
			OPENSSL_free((void*)ctx->DYNAMIC_LIBNAME);
		if(ctx->engine_id)
			OPENSSL_free((void*)ctx->engine_id);
		if(ctx->dirs)
			sk_OPENSSL_STRING_pop_free(ctx->dirs, int_free_str);
		OPENSSL_free(ctx);
		}
	}

/* Construct the per-ENGINE context. We create it blindly and then use a lock to
 * check for a race - if so, all but one of the threads "racing" will have
 * wasted their time. The alternative involves creating everything inside the
 * lock which is far worse. */
static int dynamic_set_data_ctx(ENGINE *e, dynamic_data_ctx **ctx)
	{
	dynamic_data_ctx *c;
	c = OPENSSL_malloc(sizeof(dynamic_data_ctx));
	if(!c)
		{
		ENGINEerr(ENGINE_F_DYNAMIC_SET_DATA_CTX,ERR_R_MALLOC_FAILURE);
		return 0;
		}
	memset(c, 0, sizeof(dynamic_data_ctx));
	c->dynamic_dso = NULL;
	c->v_check = NULL;
	c->bind_engine = NULL;
	c->DYNAMIC_LIBNAME = NULL;
	c->no_vcheck = 0;
	c->engine_id = NULL;
	c->list_add_value = 0;
	c->DYNAMIC_F1 = "v_check";
	c->DYNAMIC_F2 = "bind_engine";
	c->dir_load = 1;
	c->dirs = sk_OPENSSL_STRING_new_null();
	if(!c->dirs)
		{
		ENGINEerr(ENGINE_F_DYNAMIC_SET_DATA_CTX,ERR_R_MALLOC_FAILURE);
		OPENSSL_free(c);
		return 0;
		}
	CRYPTO_w_lock(CRYPTO_LOCK_ENGINE);
	if((*ctx = (dynamic_data_ctx *)ENGINE_get_ex_data(e,
				dynamic_ex_data_idx)) == NULL)
		{
		/* Good, we're the first */
		ENGINE_set_ex_data(e, dynamic_ex_data_idx, c);
		*ctx = c;
		c = NULL;
		}
	CRYPTO_w_unlock(CRYPTO_LOCK_ENGINE);
	/* If we lost the race to set the context, c is non-NULL and *ctx is the
	 * context of the thread that won. */
	if(c)
		OPENSSL_free(c);
	return 1;
	}

/* This function retrieves the context structure from an ENGINE's "ex_data", or
 * if it doesn't exist yet, sets it up. */
static dynamic_data_ctx *dynamic_get_data_ctx(ENGINE *e)
	{
	dynamic_data_ctx *ctx;
	if(dynamic_ex_data_idx < 0)
		{
		/* Create and register the ENGINE ex_data, and associate our
		 * "free" function with it to ensure any allocated contexts get
		 * freed when an ENGINE goes underground. */
		int new_idx = ENGINE_get_ex_new_index(0, NULL, NULL, NULL,
					dynamic_data_ctx_free_func);
		if(new_idx == -1)
			{
			ENGINEerr(ENGINE_F_DYNAMIC_GET_DATA_CTX,ENGINE_R_NO_INDEX);
			return NULL;
			}
		CRYPTO_w_lock(CRYPTO_LOCK_ENGINE);
		/* Avoid a race by checking again inside this lock */
		if(dynamic_ex_data_idx < 0)
			{
			/* Good, someone didn't beat us to it */
			dynamic_ex_data_idx = new_idx;
			new_idx = -1;
			}
		CRYPTO_w_unlock(CRYPTO_LOCK_ENGINE);
		/* In theory we could "give back" the index here if
		 * (new_idx>-1), but it's not possible and wouldn't gain us much
		 * if it were. */
		}
	ctx = (dynamic_data_ctx *)ENGINE_get_ex_data(e, dynamic_ex_data_idx);
	/* Check if the context needs to be created */
	if((ctx == NULL) && !dynamic_set_data_ctx(e, &ctx))
		/* "set_data" will set errors if necessary */
		return NULL;
	return ctx;
	}

static ENGINE *engine_dynamic(void)
	{
	ENGINE *ret = ENGINE_new();
	if(!ret)
		return NULL;
	if(!ENGINE_set_id(ret, engine_dynamic_id) ||
			!ENGINE_set_name(ret, engine_dynamic_name) ||
			!ENGINE_set_init_function(ret, dynamic_init) ||
			!ENGINE_set_finish_function(ret, dynamic_finish) ||
			!ENGINE_set_ctrl_function(ret, dynamic_ctrl) ||
			!ENGINE_set_flags(ret, ENGINE_FLAGS_BY_ID_COPY) ||
			!ENGINE_set_cmd_defns(ret, dynamic_cmd_defns))
		{
		ENGINE_free(ret);
		return NULL;
		}
	return ret;
	}

void ENGINE_load_dynamic(void)
	{
	ENGINE *toadd = engine_dynamic();
	if(!toadd) return;
	ENGINE_add(toadd);
	/* If the "add" worked, it gets a structural reference. So either way,
	 * we release our just-created reference. */
	ENGINE_free(toadd);
	/* If the "add" didn't work, it was probably a conflict because it was
	 * already added (eg. someone calling ENGINE_load_blah then calling
	 * ENGINE_load_builtin_engines() perhaps). */
	ERR_clear_error();
	}

static int dynamic_init(ENGINE *e)
	{
	/* We always return failure - the "dyanamic" engine itself can't be used
	 * for anything. */
	return 0;
	}

static int dynamic_finish(ENGINE *e)
	{
	/* This should never be called on account of "dynamic_init" always
	 * failing. */
	return 0;
	}

static int dynamic_ctrl(ENGINE *e, int cmd, long i, void *p, void (*f)(void))
	{
	dynamic_data_ctx *ctx = dynamic_get_data_ctx(e);
	int initialised;
	
	if(!ctx)
		{
		ENGINEerr(ENGINE_F_DYNAMIC_CTRL,ENGINE_R_NOT_LOADED);
		return 0;
		}
	initialised = ((ctx->dynamic_dso == NULL) ? 0 : 1);
	/* All our control commands require the ENGINE to be uninitialised */
	if(initialised)
		{
		ENGINEerr(ENGINE_F_DYNAMIC_CTRL,
			ENGINE_R_ALREADY_LOADED);
		return 0;
		}
	switch(cmd)
		{
	case DYNAMIC_CMD_SO_PATH:
		/* a NULL 'p' or a string of zero-length is the same thing */
		if(p && (strlen((const char *)p) < 1))
			p = NULL;
		if(ctx->DYNAMIC_LIBNAME)
			OPENSSL_free((void*)ctx->DYNAMIC_LIBNAME);
		if(p)
			ctx->DYNAMIC_LIBNAME = BUF_strdup(p);
		else
			ctx->DYNAMIC_LIBNAME = NULL;
		return (ctx->DYNAMIC_LIBNAME ? 1 : 0);
	case DYNAMIC_CMD_NO_VCHECK:
		ctx->no_vcheck = ((i == 0) ? 0 : 1);
		return 1;
	case DYNAMIC_CMD_ID:
		/* a NULL 'p' or a string of zero-length is the same thing */
		if(p && (strlen((const char *)p) < 1))
			p = NULL;
		if(ctx->engine_id)
			OPENSSL_free((void*)ctx->engine_id);
		if(p)
			ctx->engine_id = BUF_strdup(p);
		else
			ctx->engine_id = NULL;
		return (ctx->engine_id ? 1 : 0);
	case DYNAMIC_CMD_LIST_ADD:
		if((i < 0) || (i > 2))
			{
			ENGINEerr(ENGINE_F_DYNAMIC_CTRL,
				ENGINE_R_INVALID_ARGUMENT);
			return 0;
			}
		ctx->list_add_value = (int)i;
		return 1;
	case DYNAMIC_CMD_LOAD:
		return dynamic_load(e, ctx);
	case DYNAMIC_CMD_DIR_LOAD:
		if((i < 0) || (i > 2))
			{
			ENGINEerr(ENGINE_F_DYNAMIC_CTRL,
				ENGINE_R_INVALID_ARGUMENT);
			return 0;
			}
		ctx->dir_load = (int)i;
		return 1;
	case DYNAMIC_CMD_DIR_ADD:
		/* a NULL 'p' or a string of zero-length is the same thing */
		if(!p || (strlen((const char *)p) < 1))
			{
			ENGINEerr(ENGINE_F_DYNAMIC_CTRL,
				ENGINE_R_INVALID_ARGUMENT);
			return 0;
			}
		{
		char *tmp_str = BUF_strdup(p);
		if(!tmp_str)
			{
			ENGINEerr(ENGINE_F_DYNAMIC_CTRL,
				ERR_R_MALLOC_FAILURE);
			return 0;
			}
		sk_OPENSSL_STRING_insert(ctx->dirs, tmp_str, -1);
		}
		return 1;
	default:
		break;
		}
	ENGINEerr(ENGINE_F_DYNAMIC_CTRL,ENGINE_R_CTRL_COMMAND_NOT_IMPLEMENTED);
	return 0;
	}

static int int_load(dynamic_data_ctx *ctx)
	{
	int num, loop;
	/* Unless told not to, try a direct load */
	if((ctx->dir_load != 2) && (DSO_load(ctx->dynamic_dso,
				ctx->DYNAMIC_LIBNAME, NULL, 0)) != NULL)
		return 1;
	/* If we're not allowed to use 'dirs' or we have none, fail */
	if(!ctx->dir_load || (num = sk_OPENSSL_STRING_num(ctx->dirs)) < 1)
		return 0;
	for(loop = 0; loop < num; loop++)
		{
		const char *s = sk_OPENSSL_STRING_value(ctx->dirs, loop);
		char *merge = DSO_merge(ctx->dynamic_dso, ctx->DYNAMIC_LIBNAME, s);
		if(!merge)
			return 0;
		if(DSO_load(ctx->dynamic_dso, merge, NULL, 0))
			{
			/* Found what we're looking for */
			OPENSSL_free(merge);
			return 1;
			}
		OPENSSL_free(merge);
		}
	return 0;
	}

static int dynamic_load(ENGINE *e, dynamic_data_ctx *ctx)
	{
	ENGINE cpy;
	dynamic_fns fns;

	if(!ctx->dynamic_dso)
		ctx->dynamic_dso = DSO_new();
	if(!ctx->DYNAMIC_LIBNAME)
		{
		if(!ctx->engine_id)
			return 0;
		ctx->DYNAMIC_LIBNAME =
			DSO_convert_filename(ctx->dynamic_dso, ctx->engine_id);
		}
	if(!int_load(ctx))
		{
		ENGINEerr(ENGINE_F_DYNAMIC_LOAD,
			ENGINE_R_DSO_NOT_FOUND);
		DSO_free(ctx->dynamic_dso);
		ctx->dynamic_dso = NULL;
		return 0;
		}
	/* We have to find a bind function otherwise it'll always end badly */
	if(!(ctx->bind_engine = (dynamic_bind_engine)DSO_bind_func(
					ctx->dynamic_dso, ctx->DYNAMIC_F2)))
		{
		ctx->bind_engine = NULL;
		DSO_free(ctx->dynamic_dso);
		ctx->dynamic_dso = NULL;
		ENGINEerr(ENGINE_F_DYNAMIC_LOAD,
			ENGINE_R_DSO_FAILURE);
		return 0;
		}
	/* Do we perform version checking? */
	if(!ctx->no_vcheck)
		{
		unsigned long vcheck_res = 0;
		/* Now we try to find a version checking function and decide how
		 * to cope with failure if/when it fails. */
		ctx->v_check = (dynamic_v_check_fn)DSO_bind_func(
				ctx->dynamic_dso, ctx->DYNAMIC_F1);
		if(ctx->v_check)
			vcheck_res = ctx->v_check(OSSL_DYNAMIC_VERSION);
		/* We fail if the version checker veto'd the load *or* if it is
		 * deferring to us (by returning its version) and we think it is
		 * too old. */
		if(vcheck_res < OSSL_DYNAMIC_OLDEST)
			{
			/* Fail */
			ctx->bind_engine = NULL;
			ctx->v_check = NULL;
			DSO_free(ctx->dynamic_dso);
			ctx->dynamic_dso = NULL;
			ENGINEerr(ENGINE_F_DYNAMIC_LOAD,
				ENGINE_R_VERSION_INCOMPATIBILITY);
			return 0;
			}
		}
	/* First binary copy the ENGINE structure so that we can roll back if
	 * the hand-over fails */
	memcpy(&cpy, e, sizeof(ENGINE));
	/* Provide the ERR, "ex_data", memory, and locking callbacks so the
	 * loaded library uses our state rather than its own. FIXME: As noted in
	 * engine.h, much of this would be simplified if each area of code
	 * provided its own "summary" structure of all related callbacks. It
	 * would also increase opaqueness. */
	fns.static_state = ENGINE_get_static_state();
	fns.err_fns = ERR_get_implementation();
	fns.ex_data_fns = CRYPTO_get_ex_data_implementation();
	CRYPTO_get_mem_functions(&fns.mem_fns.malloc_cb,
				&fns.mem_fns.realloc_cb,
				&fns.mem_fns.free_cb);
	fns.lock_fns.lock_locking_cb = CRYPTO_get_locking_callback();
	fns.lock_fns.lock_add_lock_cb = CRYPTO_get_add_lock_callback();
	fns.lock_fns.dynlock_create_cb = CRYPTO_get_dynlock_create_callback();
	fns.lock_fns.dynlock_lock_cb = CRYPTO_get_dynlock_lock_callback();
	fns.lock_fns.dynlock_destroy_cb = CRYPTO_get_dynlock_destroy_callback();
	/* Now that we've loaded the dynamic engine, make sure no "dynamic"
	 * ENGINE elements will show through. */
	engine_set_all_null(e);

	/* Try to bind the ENGINE onto our own ENGINE structure */
	if(!ctx->bind_engine(e, ctx->engine_id, &fns))
		{
		ctx->bind_engine = NULL;
		ctx->v_check = NULL;
		DSO_free(ctx->dynamic_dso);
		ctx->dynamic_dso = NULL;
		ENGINEerr(ENGINE_F_DYNAMIC_LOAD,ENGINE_R_INIT_FAILED);
		/* Copy the original ENGINE structure back */
		memcpy(e, &cpy, sizeof(ENGINE));
		return 0;
		}
	/* Do we try to add this ENGINE to the internal list too? */
	if(ctx->list_add_value > 0)
		{
		if(!ENGINE_add(e))
			{
			/* Do we tolerate this or fail? */
			if(ctx->list_add_value > 1)
				{
				/* Fail - NB: By this time, it's too late to
				 * rollback, and trying to do so allows the
				 * bind_engine() code to have created leaks. We
				 * just have to fail where we are, after the
				 * ENGINE has changed. */
				ENGINEerr(ENGINE_F_DYNAMIC_LOAD,
					ENGINE_R_CONFLICTING_ENGINE_ID);
				return 0;
				}
			/* Tolerate */
			ERR_clear_error();
			}
		}
	return 1;
	}
