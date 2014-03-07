//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//[doc_anonymous_semaphore_shared_data
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

struct shared_memory_buffer
{
   enum { NumItems = 10 };

   shared_memory_buffer()
      : mutex(1), nempty(NumItems), nstored(0)
   {}

   //Semaphores to protect and synchronize access
   boost::interprocess::interprocess_semaphore
      mutex, nempty, nstored;

   //Items to fill
   int items[NumItems];
};
//]
