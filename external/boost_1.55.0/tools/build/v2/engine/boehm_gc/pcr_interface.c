/* 
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */
# include "private/gc_priv.h"

# ifdef PCR
/*
 * Note that POSIX PCR requires an ANSI C compiler.  Hence we are allowed
 * to make the same assumption here.
 * We wrap all of the allocator functions to avoid questions of
 * compatibility between the prototyped and nonprototyped versions of the f
 */
# include "config/PCR_StdTypes.h"
# include "mm/PCR_MM.h"
# include <errno.h>

# define MY_MAGIC 17L
# define MY_DEBUGMAGIC 42L

void * GC_AllocProc(size_t size, PCR_Bool ptrFree, PCR_Bool clear )
{
    if (ptrFree) {
        void * result = (void *)GC_malloc_atomic(size);
        if (clear && result != 0) BZERO(result, size);
        return(result);
    } else {
        return((void *)GC_malloc(size));
    }
}

void * GC_DebugAllocProc(size_t size, PCR_Bool ptrFree, PCR_Bool clear )
{
    if (ptrFree) {
        void * result = (void *)GC_debug_malloc_atomic(size, __FILE__,
        						     __LINE__);
        if (clear && result != 0) BZERO(result, size);
        return(result);
    } else {
        return((void *)GC_debug_malloc(size, __FILE__, __LINE__));
    }
}

# define GC_ReallocProc GC_realloc
void * GC_DebugReallocProc(void * old_object, size_t new_size_in_bytes)
{
    return(GC_debug_realloc(old_object, new_size_in_bytes, __FILE__, __LINE__));
}

# define GC_FreeProc GC_free
# define GC_DebugFreeProc GC_debug_free

typedef struct {
  PCR_ERes (*ed_proc)(void *p, size_t size, PCR_Any data);
  GC_bool ed_pointerfree;
  PCR_ERes ed_fail_code;
  PCR_Any ed_client_data;
} enumerate_data;

void GC_enumerate_block(struct hblk *h; enumerate_data * ed)
{
    register hdr * hhdr;
    register int sz;
    ptr_t p;
    ptr_t lim;
    word descr;
#   error This code was updated without testing.
#   error and its precursor was clearly broken.
    
    hhdr = HDR(h);
    descr = hhdr -> hb_descr;
    sz = hhdr -> hb_sz;
    if (descr != 0 && ed -> ed_pointerfree
    	|| descr == 0 && !(ed -> ed_pointerfree)) return;
    lim = (ptr_t)(h+1) - sz;
    p = (ptr_t)h;
    do {
        if (PCR_ERes_IsErr(ed -> ed_fail_code)) return;
        ed -> ed_fail_code =
            (*(ed -> ed_proc))(p, sz, ed -> ed_client_data);
        p+= sz;
    } while (p <= lim);
}

struct PCR_MM_ProcsRep * GC_old_allocator = 0;

PCR_ERes GC_EnumerateProc(
    PCR_Bool ptrFree,
    PCR_ERes (*proc)(void *p, size_t size, PCR_Any data),
    PCR_Any data
)
{
    enumerate_data ed;
    
    ed.ed_proc = proc;
    ed.ed_pointerfree = ptrFree;
    ed.ed_fail_code = PCR_ERes_okay;
    ed.ed_client_data = data;
    GC_apply_to_all_blocks(GC_enumerate_block, &ed);
    if (ed.ed_fail_code != PCR_ERes_okay) {
        return(ed.ed_fail_code);
    } else {
    	/* Also enumerate objects allocated by my predecessors */
    	return((*(GC_old_allocator->mmp_enumerate))(ptrFree, proc, data));
    }
}

void GC_DummyFreeProc(void *p) {}

void GC_DummyShutdownProc(void) {}

struct PCR_MM_ProcsRep GC_Rep = {
	MY_MAGIC,
	GC_AllocProc,
	GC_ReallocProc,
	GC_DummyFreeProc,  	/* mmp_free */
	GC_FreeProc,  		/* mmp_unsafeFree */
	GC_EnumerateProc,
	GC_DummyShutdownProc	/* mmp_shutdown */
};

struct PCR_MM_ProcsRep GC_DebugRep = {
	MY_DEBUGMAGIC,
	GC_DebugAllocProc,
	GC_DebugReallocProc,
	GC_DummyFreeProc,  	/* mmp_free */
	GC_DebugFreeProc,  		/* mmp_unsafeFree */
	GC_EnumerateProc,
	GC_DummyShutdownProc	/* mmp_shutdown */
};

GC_bool GC_use_debug = 0;

void GC_pcr_install()
{
    PCR_MM_Install((GC_use_debug? &GC_DebugRep : &GC_Rep), &GC_old_allocator);
}

PCR_ERes
PCR_GC_Setup(void)
{
    return PCR_ERes_okay;
}

PCR_ERes
PCR_GC_Run(void)
{

    if( !PCR_Base_TestPCRArg("-nogc") ) {
        GC_quiet = ( PCR_Base_TestPCRArg("-gctrace") ? 0 : 1 );
        GC_use_debug = (GC_bool)PCR_Base_TestPCRArg("-debug_alloc");
        GC_init();
        if( !PCR_Base_TestPCRArg("-nogc_incremental") ) {
            /*
             * awful hack to test whether VD is implemented ...
             */
            if( PCR_VD_Start( 0, NIL, 0) != PCR_ERes_FromErr(ENOSYS) ) {
	        GC_enable_incremental();
	    }
	}
    }
    return PCR_ERes_okay;
}

void GC_push_thread_structures(void)
{
    /* PCR doesn't work unless static roots are pushed.  Can't get here. */
    ABORT("In GC_push_thread_structures()");
}

# endif
