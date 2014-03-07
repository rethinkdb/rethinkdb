/****************************************************************************
Copyright (c) 1994 by Xerox Corporation.  All rights reserved.
 
THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 
Permission is hereby granted to use or copy this program for any
purpose, provided the above notices are retained on all copies.
Permission to modify the code and to distribute modified code is
granted, provided the above notices are retained, and a notice that
the code was modified is included with the above copyright notice.
****************************************************************************
Last modified on Mon Jul 10 21:06:03 PDT 1995 by ellis
     modified on December 20, 1994 7:27 pm PST by boehm

usage: test_cpp number-of-iterations

This program tries to test the specific C++ functionality provided by
gc_c++.h that isn't tested by the more general test routines of the
collector.

A recommended value for number-of-iterations is 10, which will take a
few minutes to complete.

***************************************************************************/

#include "gc_cpp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define USE_STD_ALLOCATOR
#ifdef USE_STD_ALLOCATOR
#   include "gc_allocator.h"
#elif __GNUC__
#   include "new_gc_alloc.h"
#else
#   include "gc_alloc.h"
#endif
extern "C" {
# include "private/gcconfig.h"
  GC_API void GC_printf(const char *format, ...);
  /* Use GC private output to reach the same log file.  */
  /* Don't include gc_priv.h, since that may include Windows system	*/
  /* header files that don't take kindly to this context.		*/
}
#ifdef MSWIN32
#   include <windows.h>
#endif
#ifdef GC_NAME_CONFLICT
#   define USE_GC UseGC
    struct foo * GC;
#else
#   define USE_GC GC
#endif


#define my_assert( e ) \
    if (! (e)) { \
        GC_printf( "Assertion failure in " __FILE__ ", line %d: " #e "\n", \
                    __LINE__ ); \
        exit( 1 ); }


class A {public:
    /* An uncollectable class. */

    A( int iArg ): i( iArg ) {}
    void Test( int iArg ) {
        my_assert( i == iArg );} 
    int i;};


class B: public gc, public A {public:
    /* A collectable class. */

    B( int j ): A( j ) {}
    ~B() {
        my_assert( deleting );}
    static void Deleting( int on ) {
        deleting = on;}
    static int deleting;};

int B::deleting = 0;


class C: public gc_cleanup, public A {public:
    /* A collectable class with cleanup and virtual multiple inheritance. */

    C( int levelArg ): A( levelArg ), level( levelArg ) {
        nAllocated++;
        if (level > 0) {
            left = new C( level - 1 );
            right = new C( level - 1 );}
        else {
            left = right = 0;}}
    ~C() {
        this->A::Test( level );
        nFreed++;
        my_assert( level == 0 ? 
                   left == 0 && right == 0 :
                   level == left->level + 1 && level == right->level + 1 );
        left = right = 0;
        level = -123456;}
    static void Test() {
        my_assert( nFreed <= nAllocated && nFreed >= .8 * nAllocated );}

    static int nFreed;
    static int nAllocated;
    int level;
    C* left;
    C* right;};

int C::nFreed = 0;
int C::nAllocated = 0;


class D: public gc {public:
    /* A collectable class with a static member function to be used as
    an explicit clean-up function supplied to ::new. */

    D( int iArg ): i( iArg ) {
        nAllocated++;}
    static void CleanUp( void* obj, void* data ) {
        D* self = (D*) obj;
        nFreed++;
        my_assert( self->i == (int) (GC_word) data );}
    static void Test() {
        my_assert( nFreed >= .8 * nAllocated );}
       
    int i;
    static int nFreed;
    static int nAllocated;};

int D::nFreed = 0;
int D::nAllocated = 0;


class E: public gc_cleanup {public:
    /* A collectable class with clean-up for use by F. */

    E() {
        nAllocated++;}
    ~E() {
        nFreed++;}

    static int nFreed;
    static int nAllocated;};
    
int E::nFreed = 0;
int E::nAllocated = 0;
   

class F: public E {public:
    /* A collectable class with clean-up, a base with clean-up, and a
    member with clean-up. */

    F() {
        nAllocated++;}
    ~F() {
        nFreed++;}
    static void Test() {
        my_assert( nFreed >= .8 * nAllocated );
        my_assert( 2 * nFreed == E::nFreed );}
       
    E e;
    static int nFreed;
    static int nAllocated;};
    
int F::nFreed = 0;
int F::nAllocated = 0;
   

GC_word Disguise( void* p ) {
    return ~ (GC_word) p;}

void* Undisguise( GC_word i ) {
    return (void*) ~ i;}


#ifdef MSWIN32
int APIENTRY WinMain(
    HINSTANCE instance, HINSTANCE prev, LPSTR cmd, int cmdShow ) 
{
    int argc;
    char* argv[ 3 ];

    for (argc = 1; argc < sizeof( argv ) / sizeof( argv[ 0 ] ); argc++) {
        argv[ argc ] = strtok( argc == 1 ? cmd : 0, " \t" );
        if (0 == argv[ argc ]) break;}

#else
# ifdef MACOS
    int main() {
# else
    int main( int argc, char* argv[] ) {
# endif
#endif

   GC_INIT();

#  if defined(MACOS)                        // MacOS
    char* argv_[] = {"test_cpp", "10"};     //   doesn't
    argv = argv_;                           //     have a
    argc = sizeof(argv_)/sizeof(argv_[0]);  //       commandline
#  endif 
    int i, iters, n;
#   ifdef USE_STD_ALLOCATOR
      int *x = gc_allocator<int>().allocate(1);
      int **xptr = traceable_allocator<int *>().allocate(1);
#   else 
#     ifdef __GNUC__
          int *x = (int *)gc_alloc::allocate(sizeof(int));
#     else
          int *x = (int *)alloc::allocate(sizeof(int));
#     endif
#   endif
    *x = 29;
#   ifdef USE_STD_ALLOCATOR
      *xptr = x;
      x = 0;
#   endif
    if (argc != 2 || (0 >= (n = atoi( argv[ 1 ] )))) {
        GC_printf( "usage: test_cpp number-of-iterations\nAssuming 10 iters\n" );
        n = 10;}
        
    for (iters = 1; iters <= n; iters++) {
        GC_printf( "Starting iteration %d\n", iters );

            /* Allocate some uncollectable As and disguise their pointers.
            Later we'll check to see if the objects are still there.  We're
            checking to make sure these objects really are uncollectable. */
        GC_word as[ 1000 ];
        GC_word bs[ 1000 ];
        for (i = 0; i < 1000; i++) {
            as[ i ] = Disguise( new (NoGC) A( i ) );
            bs[ i ] = Disguise( new (NoGC) B( i ) );}

            /* Allocate a fair number of finalizable Cs, Ds, and Fs.
            Later we'll check to make sure they've gone away. */
        for (i = 0; i < 1000; i++) {
            C* c = new C( 2 );
            C c1( 2 );           /* stack allocation should work too */
            D* d = ::new (USE_GC, D::CleanUp, (void*)(GC_word)i) D( i );
            F* f = new F;
            if (0 == i % 10) delete c;}

            /* Allocate a very large number of collectable As and Bs and
            drop the references to them immediately, forcing many
            collections. */
        for (i = 0; i < 1000000; i++) {
            A* a = new (USE_GC) A( i );
            B* b = new B( i );
            b = new (USE_GC) B( i );
            if (0 == i % 10) {
                B::Deleting( 1 );
                delete b;
                B::Deleting( 0 );}
#	    ifdef FINALIZE_ON_DEMAND
	      GC_invoke_finalizers();
#	    endif
	    }

            /* Make sure the uncollectable As and Bs are still there. */
        for (i = 0; i < 1000; i++) {
            A* a = (A*) Undisguise( as[ i ] );
            B* b = (B*) Undisguise( bs[ i ] );
            a->Test( i );
            delete a;
            b->Test( i );
            B::Deleting( 1 );
            delete b;
            B::Deleting( 0 );
#	    ifdef FINALIZE_ON_DEMAND
	   	 GC_invoke_finalizers();
#	    endif

	    }

            /* Make sure most of the finalizable Cs, Ds, and Fs have
            gone away. */
        C::Test();
        D::Test();
        F::Test();}

#   ifdef USE_STD_ALLOCATOR
      x = *xptr;
#   endif
    my_assert (29 == x[0]);
    GC_printf( "The test appears to have succeeded.\n" );
    return( 0 );}
    

