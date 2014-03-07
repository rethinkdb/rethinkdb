//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_BETHREADS
//  TITLE:         BeOS Threads
//  DESCRIPTION:   The platform supports BeOS style threads.

#include <OS.h>


namespace boost_has_bethreads{

int test()
{
   sem_id mut = create_sem(1, "test");
   if(mut > 0)
   {
      acquire_sem(mut);
      release_sem(mut);
      delete_sem(mut);
   }
   return 0;
}

}





