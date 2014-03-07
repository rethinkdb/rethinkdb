/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
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
/* Boehm, July 31, 1995 5:02 pm PDT */


#include "private/gc_priv.h"

#if defined(MANUAL_VDB)
/* Stubborn object (hard to change, nearly immutable) allocation. */
/* This interface is deprecated.  We mostly emulate it using	  */
/* MANUAL_VDB.  But that imposes the additional constraint that	  */
/* written, but not yet GC_dirty()ed objects must be referenced	  */
/* by a stack.							  */
void * GC_malloc_stubborn(size_t lb)
{
    return(GC_malloc(lb));
}

/*ARGSUSED*/
void GC_end_stubborn_change(void *p)
{
    GC_dirty(p);
}

/*ARGSUSED*/
void GC_change_stubborn(void *p)
{
}

#else /* !MANUAL_VDB */

void * GC_malloc_stubborn(size_t lb)
{
    return(GC_malloc(lb));
}

/*ARGSUSED*/
void GC_end_stubborn_change(void *p)
{
}

/*ARGSUSED*/
void GC_change_stubborn(void *p)
{
}

#endif /* !MANUAL_VDB */
