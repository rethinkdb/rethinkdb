//  (C) Copyright Gennadiy Rozental 2001-2008.
//  (C) Copyright Beman Dawes 2001. 
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : tests an ability of Program Execution Monitor to catch 
//  uncatched exceptions. Should fail during run.
// ***************************************************************************

#ifdef __MWERKS__
//  Metrowerks doesn't build knowledge of what runtime libraries to link with
//  into their compiler. Instead they depend on pragmas in their standard
//  library headers. That creates the odd situation that certain programs
//  won't link unless at least one standard header is included. Note that
//  this problem is highly dependent on enviroment variables and command
//  line options, so just because the problem doesn't show up on one
//  system doesn't mean it has been fixed. Remove this workaround only
//  when told by Metrowerks that it is safe to do so.
#include <cstddef> //Metrowerks linker needs at least one standard library
#endif

#include <boost/test/included/prg_exec_monitor.hpp>

int
cpp_main( int argc, char *[] )  // note the name
{
    if( argc > 0 ) // to prevent the unreachable return warning
        throw "Test error by throwing C-style string exception";

    return 0;
}

//____________________________________________________________________________//

// EOF
