/*************************************************************************
Copyright (c) 1994 by Xerox Corporation.  All rights reserved.
 
THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 
    Last modified on Sat Nov 19 19:31:14 PST 1994 by ellis
                  on Sat Jun  8 15:10:00 PST 1994 by boehm

Permission is hereby granted to copy this code for any purpose,
provided the above notices are retained on all copies.

This implementation module for gc_c++.h provides an implementation of
the global operators "new" and "delete" that calls the Boehm
allocator.  All objects allocated by this implementation will be
non-collectable but part of the root set of the collector.

You should ensure (using implementation-dependent techniques) that the
linker finds this module before the library that defines the default
built-in "new" and "delete".

Authors: John R. Ellis and Jesse Hull

**************************************************************************/
/* Boehm, December 20, 1994 7:26 pm PST */

#include "gc_cpp.h"

void* operator new( size_t size ) {
    return GC_MALLOC_UNCOLLECTABLE( size );}
  
void operator delete( void* obj ) {
    GC_FREE( obj );}
  
#ifdef GC_OPERATOR_NEW_ARRAY

void* operator new[]( size_t size ) {
    return GC_MALLOC_UNCOLLECTABLE( size );}
  
void operator delete[]( void* obj ) {
    GC_FREE( obj );}

#endif /* GC_OPERATOR_NEW_ARRAY */

#ifdef _MSC_VER

// This new operator is used by VC++ in case of Debug builds !
void* operator new( size_t size,
			  int ,//nBlockUse,
			  const char * szFileName,
			  int nLine )
{
#ifndef GC_DEBUG
	return GC_malloc_uncollectable( size );
#else
	return GC_debug_malloc_uncollectable(size, szFileName, nLine);
#endif
}

// This new operator is used by VC++ 7.0 and later in Debug builds.
void* operator new[](size_t size, int nBlockUse, const char* szFileName, int nLine)
{
    return operator new(size, nBlockUse, szFileName, nLine);
}

#endif /* _MSC_VER */

