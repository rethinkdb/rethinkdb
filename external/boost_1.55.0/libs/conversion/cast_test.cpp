//  boost utility cast test program  -----------------------------------------//

//  (C) Copyright Beman Dawes, Dave Abrahams 1999. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

//  Revision History
//   28 Set 04  factored out numeric_cast<> test (Fernando Cacciola)
//   20 Jan 01  removed use of <limits> for portability to raw GCC (David Abrahams)
//   28 Jun 00  implicit_cast removed (Beman Dawes)
//   30 Aug 99  value_cast replaced by numeric_cast
//    3 Aug 99  Initial Version

#include <iostream>
#include <climits>
#include <cfloat>   // for DBL_MAX (Peter Schmid)
#include <boost/cast.hpp>

#  if SCHAR_MAX == LONG_MAX
#      error "This test program doesn't work if SCHAR_MAX == LONG_MAX"
#  endif

using namespace boost;
using std::cout;

namespace
{
    struct Base
    {
        virtual char kind() { return 'B'; }
    };

    struct Base2
    {
        virtual char kind2() { return '2'; }
    };

    struct Derived : public Base, Base2
    {
        virtual char kind() { return 'D'; }
    };
}


int main( int argc, char * argv[] )
{
#   ifdef NDEBUG
        cout << "NDEBUG is defined\n";
#   else
        cout << "NDEBUG is not defined\n";
#   endif

    cout << "\nBeginning tests...\n";

//  test polymorphic_cast  ---------------------------------------------------//

    //  tests which should succeed
    Base *    base = new Derived;
    Base2 *   base2 = 0;
    Derived * derived = 0;
    derived = polymorphic_downcast<Derived*>( base );  // downcast
    assert( derived->kind() == 'D' );

    derived = 0;
    derived = polymorphic_cast<Derived*>( base ); // downcast, throw on error
    assert( derived->kind() == 'D' );

    base2 = polymorphic_cast<Base2*>( base ); // crosscast
    assert( base2->kind2() == '2' );

     //  tests which should result in errors being detected
    int err_count = 0;
    base = new Base;

    if ( argc > 1 && *argv[1] == '1' )
        { derived = polymorphic_downcast<Derived*>( base ); } // #1 assert failure

    bool caught_exception = false;
    try { derived = polymorphic_cast<Derived*>( base ); }
    catch (std::bad_cast)
        { cout<<"caught bad_cast\n"; caught_exception = true; }
    if ( !caught_exception ) ++err_count;
    //  the following is just so generated code can be inspected
    if ( derived->kind() == 'B' ) ++err_count;

    cout << err_count << " errors detected\nTest "
         << (err_count==0 ? "passed\n" : "failed\n");
    return err_count;
}   // main
