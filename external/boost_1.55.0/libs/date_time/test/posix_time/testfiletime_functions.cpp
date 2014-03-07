/* Copyright (c) 2002,2003, 2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include "../testfrmwk.hpp"
#include "boost/date_time/filetime_functions.hpp"
#include <cmath>

#if defined(BOOST_HAS_FTIME)
#include <windows.h>
#endif

int main() 
{
#if defined(BOOST_HAS_FTIME) // skip tests if no FILETIME

  using namespace boost::posix_time;

  // adjustor is used to truncate ptime's fractional seconds for 
  // comparison with SYSTEMTIME's milliseconds
  const int adjustor = time_duration::ticks_per_second() / 1000;
  
  for(int i = 0; i < 5; ++i){

    FILETIME ft;
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st,&ft); 

    ptime pt = from_ftime<ptime>(ft);

    check_equal("ptime year matches systemtime year", 
        st.wYear, pt.date().year());
    check_equal("ptime month matches systemtime month", 
        st.wMonth, pt.date().month());
    check_equal("ptime day matches systemtime day", 
        st.wDay, pt.date().day());
    check_equal("ptime hour matches systemtime hour", 
        st.wHour, pt.time_of_day().hours());
    check_equal("ptime minute matches systemtime minute", 
        st.wMinute, pt.time_of_day().minutes());
    check_equal("ptime second matches systemtime second", 
        st.wSecond, pt.time_of_day().seconds());
    check_equal("truncated ptime fractional second matches systemtime millisecond", 
        st.wMilliseconds, (pt.time_of_day().fractional_seconds() / adjustor));

    // burn up a little time
    for (int j=0; j<100000; j++)
    {
      SYSTEMTIME tmp;
      GetSystemTime(&tmp);
    }

  } // for loop

  // check that time_from_ftime works for pre-1970-Jan-01 dates, too
  // zero FILETIME should represent 1601-Jan-01 00:00:00.000
  FILETIME big_bang_by_ms;
  big_bang_by_ms.dwLowDateTime = big_bang_by_ms.dwHighDateTime = 0;
  ptime pt = from_ftime<ptime>(big_bang_by_ms);

  check_equal("big bang ptime year matches 1601", 
      1601, pt.date().year());
  check_equal("big bang ptime month matches Jan", 
      1, pt.date().month());
  check_equal("big bang ptime day matches 1", 
      1, pt.date().day());
  check_equal("big bang ptime hour matches 0", 
      0, pt.time_of_day().hours());
  check_equal("big bang ptime minute matches 0", 
      0, pt.time_of_day().minutes());
  check_equal("big bang ptime second matches 0", 
      0, pt.time_of_day().seconds());
  check_equal("big bang truncated ptime fractional second matches 0", 
      0, (pt.time_of_day().fractional_seconds() / adjustor));


#else // BOOST_HAS_FTIME
  // we don't want a forced failure here, not a shortcoming
  check("FILETIME not available for this compiler/platform", true);
#endif // BOOST_HAS_FTIME
  
  return printTestStats();

}

