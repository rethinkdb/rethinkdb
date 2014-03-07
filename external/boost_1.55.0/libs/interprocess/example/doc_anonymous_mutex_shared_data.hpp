//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
//[doc_anonymous_mutex_shared_data
#include <boost/interprocess/sync/interprocess_mutex.hpp>

struct shared_memory_log
{
   enum { NumItems = 100 };
   enum { LineSize = 100 };

   shared_memory_log()
      :  current_line(0)
      ,  end_a(false)
      ,  end_b(false)
   {}

   //Mutex to protect access to the queue
   boost::interprocess::interprocess_mutex mutex;

   //Items to fill
   char   items[NumItems][LineSize];
   int    current_line;
   bool   end_a;
   bool   end_b;
};
//]
#include <boost/interprocess/detail/config_end.hpp>
