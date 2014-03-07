//  (C) Copyright Gennadiy Rozental 2002-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : tests an ability of the Program Execution Monitor to catch 
//  user fatal exceptions. Should fail during run. But not crash.
// ***************************************************************************

#include <cassert>

int cpp_main( int, char *[] )  // note the name
{
    int div = 0;

    assert( div != 0 );

#if NDEBUG 

#   pragma message("assert check turned off under NDEBUG")

    return 1; 

#endif 

    return 0;
}

//____________________________________________________________________________//

// EOF
