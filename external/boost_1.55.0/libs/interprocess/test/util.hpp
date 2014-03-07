//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_INTERPROCESS_TEST_UTIL_HEADER
#define BOOST_INTERPROCESS_TEST_UTIL_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <algorithm>
#include <iostream>
#include <boost/version.hpp>

namespace boost {
namespace interprocess {
namespace test {

inline void sleep(const boost::posix_time::ptime &xt)
{
   boost::interprocess::ipcdetail::thread_sleep
      ((xt - microsec_clock::universal_time()).total_milliseconds());
}

inline boost::posix_time::ptime delay(int secs, int msecs=0, int nsecs = 0)
{
   (void)msecs;
   using namespace boost::posix_time;
   int count = static_cast<int>(double(nsecs)*
               (double(time_duration::ticks_per_second())/double(1000000000.0)));
   count += static_cast<int>(double(msecs)*
               (double(time_duration::ticks_per_second())/double(1000.0)));
   boost::posix_time::ptime cur = microsec_clock::universal_time();
   return cur +=  boost::posix_time::time_duration(0, 0, secs, count);
}

inline bool in_range(const boost::posix_time::ptime& xt, int secs=1)
{
    boost::posix_time::ptime min = delay(-secs);
    boost::posix_time::ptime max = delay(0);
    return (xt > min) && (max > xt);
}

template <typename P>
class thread_adapter
{
   public:
   thread_adapter(void (*func)(void*, P &), void* param1, P &param2)
      : func_(func), param1_(param1) ,param2_(param2){ }
   void operator()() const { func_(param1_, param2_); }

   private:
   void (*func_)(void*, P &);
   void* param1_;
   P& param2_;
};

template <typename P>
struct data
{
   data(int id, int secs=0)
      : m_id(id), m_value(-1), m_secs(secs), m_error(no_error)
   {}
   int            m_id;
   int            m_value;
   int            m_secs;
   error_code_t   m_error;
};

static int shared_val = 0;
static const int BaseSeconds = 1;

}  //namespace test {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_TEST_UTIL_HEADER
