///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

unsigned allocation_count = 0;

void *(*alloc_func_ptr) (size_t);
void *(*realloc_func_ptr) (void *, size_t, size_t);
void (*free_func_ptr) (void *, size_t);

void *alloc_func(size_t n)
{
   ++allocation_count;
   return (*alloc_func_ptr)(n);
}

void free_func(void * p, size_t n)
{
   (*free_func_ptr)(p, n);
}

void * realloc_func(void * p, size_t old, size_t n)
{
   ++allocation_count;
   return (*realloc_func_ptr)(p, old, n);
}

int main()
{
   using namespace boost::multiprecision;

#if defined(TEST_MPFR) || defined(TEST_MPFR_CLASS) || defined(TEST_MPREAL) || defined(TEST_MPF)
   mp_get_memory_functions(&alloc_func_ptr, &realloc_func_ptr, &free_func_ptr);
   mp_set_memory_functions(&alloc_func, &realloc_func, &free_func);
#endif

   basic_tests();
   bessel_tests();
   poly_tests();
   nct_tests();
}

