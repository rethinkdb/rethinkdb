//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

#include <boost/test/prg_exec_monitor.hpp>

int add( int i, int j ) { return i+j; }

int cpp_main( int, char *[] )  // note the name!
{ 
    // two ways to detect and report the same error:
    if ( add(2,2) != 4 ) throw "Oops..."; // #1 throws on error

    return add(2,2) == 4 ? 15 : 1;         // #2 returns error directly
}

// EOF
