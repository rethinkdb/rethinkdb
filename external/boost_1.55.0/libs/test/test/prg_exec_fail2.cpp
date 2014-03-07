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
//  system exceptions. Should fail during run.
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


int cpp_main( int, char *[] )  // note the name
{
#if defined(APPLE) && defined(ppc)
    printf("The ppc doesn't throw on divice-by-zero. No check.\n"); 
    return 1;
#else 
    int div = 0;
    return 10 / div;
#endif
}

//____________________________________________________________________________//

// EOF
