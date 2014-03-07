/*-----------------------------------------------------------------------------+
Interval Container Library
Author: Joachim Faulhaber
Copyright (c) 2007-2009: Joachim Faulhaber
Copyright (c) 1999-2006: Cortex Software GmbH, Kantstrasse 57, Berlin
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/

/** Example partys_tallest_guests.cpp \file partys_tallest_guests.cpp
    \brief Using <i>aggregate on overlap</i> the heights of the party's tallest 
           guests are computed. 

    In partys_tallest_guests.cpp we use a different instantiation of
    interval map templates to compute maxima of guest heights.

    Instead of aggregating groups of people attending the party in time
    we aggregate the maximum of guest height for the time intervals.

    Using a joining interval_map results in a smaller map: All interval
    value pairs are joined if the maximum does not change in time. Using
    a split_interval_map results in a larger map: All splits of intervals
    that occur due to entering and leaving of guests are preserved in 
    the split_interval_map.

    \include partys_tallest_guests_/partys_tallest_guests.cpp
*/
//[example_partys_tallest_guests
// The next line includes <boost/date_time/posix_time/posix_time.hpp>
// and a few lines of adapter code.
#include <boost/icl/ptime.hpp> 
#include <iostream>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>

using namespace std;
using namespace boost::posix_time;
using namespace boost::icl;


// A party's height shall be defined as the maximum height of all guests ;-)
// The last parameter 'inplace_max' is a functor template that calls a max 
// aggregation on overlap.
typedef interval_map<ptime, int, partial_absorber, less, inplace_max> 
    PartyHeightHistoryT;

// Using a split_interval_map we preserve interval splittings that occurred via insertion.
typedef split_interval_map<ptime, int, partial_absorber, less, inplace_max> 
    PartyHeightSplitHistoryT;

void partys_height()
{
    PartyHeightHistoryT tallest_guest;

    tallest_guest +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 19:30"), 
          time_from_string("2008-05-20 23:00")), 
        180); // Mary & Harry: Harry is 1,80 m tall.

    tallest_guest +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 20:10"), 
          time_from_string("2008-05-21 00:00")), 
        170); // Diana & Susan: Diana is 1,70 m tall.

    tallest_guest +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 22:15"), 
          time_from_string("2008-05-21 00:30")), 
        200); // Peters height is 2,00 m

    PartyHeightHistoryT::iterator height_ = tallest_guest.begin();
    cout << "-------------- History of maximum guest height -------------------\n";
    while(height_ != tallest_guest.end())
    {
        discrete_interval<ptime> when = height_->first;
        // Of what height are the tallest guests within the time interval 'when' ?
        int height = (*height_++).second;
        cout << "[" << first(when) << " - " << upper(when) << ")"
             << ": " << height <<" cm = " << height/30.48 << " ft" << endl;
    }

}

// Next we are using a split_interval_map instead of a joining interval_map
void partys_split_height()
{
    PartyHeightSplitHistoryT tallest_guest;

    // adding an element can be done wrt. simple aggregate functions
    // like e.g. min, max etc. in their 'inplace' or op= incarnation
    tallest_guest +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 19:30"), 
          time_from_string("2008-05-20 23:00")), 
        180); // Mary & Harry: Harry is 1,80 m tall.

    tallest_guest +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 20:10"), 
          time_from_string("2008-05-21 00:00")), 
        170); // Diana & Susan: Diana is 1,70 m tall.

    tallest_guest +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 22:15"), 
          time_from_string("2008-05-21 00:30")), 
        200); // Peters height is 2,00 m

    PartyHeightSplitHistoryT::iterator height_ = tallest_guest.begin();
    cout << "\n";
    cout << "-------- Split History of maximum guest height -------------------\n";
    cout << "--- Same map as above but split for every interval insertion.  ---\n";
    while(height_ != tallest_guest.end())
    {
        discrete_interval<ptime> when = height_->first;
        // Of what height are the tallest guests within the time interval 'when' ?
        int height = (*height_++).second;
        cout << "[" << first(when) << " - " << upper(when) << ")"
             << ": " << height <<" cm = " << height/30.48 << " ft" << endl;
    }

}


int main()
{
    cout << ">>Interval Container Library: Sample partys_tallest_guests.cpp  <<\n";
    cout << "------------------------------------------------------------------\n";
    partys_height();
    partys_split_height();
    return 0;
}

// Program output:
/*-----------------------------------------------------------------------------
>>Interval Container Library: Sample partys_tallest_guests.cpp  <<
------------------------------------------------------------------
-------------- History of maximum guest height -------------------
[2008-May-20 19:30:00 - 2008-May-20 22:15:00): 180 cm = 5.90551 ft
[2008-May-20 22:15:00 - 2008-May-21 00:30:00): 200 cm = 6.56168 ft

-------- Split History of maximum guest height -------------------
--- Same map as above but split for every interval insertion.  ---
[2008-May-20 19:30:00 - 2008-May-20 20:10:00): 180 cm = 5.90551 ft
[2008-May-20 20:10:00 - 2008-May-20 22:15:00): 180 cm = 5.90551 ft
[2008-May-20 22:15:00 - 2008-May-20 23:00:00): 200 cm = 6.56168 ft
[2008-May-20 23:00:00 - 2008-May-21 00:00:00): 200 cm = 6.56168 ft
[2008-May-21 00:00:00 - 2008-May-21 00:30:00): 200 cm = 6.56168 ft
-----------------------------------------------------------------------------*/
//]

