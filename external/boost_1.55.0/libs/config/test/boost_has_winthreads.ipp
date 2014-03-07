//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_WINTHREADS
//  TITLE:         MS Windows threads
//  DESCRIPTION:   The platform supports MS Windows style threads.

#include <windows.h>


namespace boost_has_winthreads{

int test()
{
   HANDLE h = CreateMutex(0, 0, 0);
   if(h != NULL)
   {
      WaitForSingleObject(h, INFINITE);
      ReleaseMutex(h);
      CloseHandle(h);
   }
   return 0;
}

}





