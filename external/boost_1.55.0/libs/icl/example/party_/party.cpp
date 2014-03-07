/*-----------------------------------------------------------------------------+    
Interval Container Library
Author: Joachim Faulhaber
Copyright (c) 2007-2010: Joachim Faulhaber
Copyright (c) 1999-2006: Cortex Software GmbH, Kantstrasse 57, Berlin
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#include <iostream>
#include <boost/icl/interval_map.hpp>
#include "../toytime.hpp"

using namespace std;
using namespace boost::icl;

/** 
    Party.cpp demonstrates the possibilities of an interval map (interval_map or
    split_interval_map). An interval_map maps intervals to a given content. In
    this case the content is a set of party guests represented by their name
    strings.

    As time goes by, groups of people join the party and leave later in the 
    evening. So we add a time interval and a name set to the interval_map for 
    the attendance of each group of people, that come together and leave 
    together.

    On every overlap of intervals, the corresponding name sets are accumulated. 
    At the points of overlap the intervals are split. The accumulation of content
    on overlap of intervals is always done via an operator += that has to be 
    implemented for the content parameter of the interval_map.

    Finally the interval_map contains the history of attendance and all points
    in time, where the group of party guests changed.

    Party.cpp demonstrates a principle that we call aggregate on overlap 
    (aggovering;) On insertion a value associated to the interval is aggregated
    (added) to those values in the interval_map that overlap with the inserted 
    value.

    There are two behavioral aspects to aggovering: a decompositional behavior
    and a accumulative behavior.

    The decompositional behavior splits up intervals on the time dimension of
    the interval_map so that the intervals change whenever associated values
    change.

    The accumulative behavior accumulates associated values on every overlap of
    an insertion for the associated values.
*/

// Type set<string> collects the names of party guests. Since std::set is
// a model of the itl's set concept, the concept provides an operator += 
// that performs a set union on overlap of intervals.
typedef std::set<string> GuestSetT;

// 'Time' is the domain type the interval_map. It's key values are therefore
// time intervals. The content is the set of names: GuestSetT.


void party()
{
    GuestSetT mary_harry; 
    mary_harry.insert("Mary");
    mary_harry.insert("Harry");

    GuestSetT diana_susan; 
    diana_susan.insert("Diana");
    diana_susan.insert("Susan");

    GuestSetT peter; 
    peter.insert("Peter");

    interval_map<Time, GuestSetT> party;

    party += make_pair( interval<Time>::right_open(Time(19,30), Time(23,00)), mary_harry);
    party += make_pair( interval<Time>::right_open(Time(20,10), Time(monday,0,0)), diana_susan);
    party += make_pair( interval<Time>::right_open(Time(22,15), Time(monday,0,30)), peter);

    interval_map<Time, GuestSetT>::iterator it = party.begin();
    while(it != party.end())
    {
        discrete_interval<Time> when = it->first;
        // Who is at the party within the time interval 'when' ?
        GuestSetT who = (*it++).second;
        cout << when << ": " << who << endl;
    }
}

int main()
{
    cout << ">>Interval Container Library: Sample party.cpp       <<\n";
    cout << "-------------------------------------------------------\n";
    party();
    return 0;
}

// Program output:

// >>Interval Container Library: Sample party.cpp <<
// -------------------------------------------------
// [sun:19:30,sun:20:10): Harry Mary
// [sun:20:10,sun:22:15): Diana Harry Mary Susan
// [sun:22:15,sun:23:00): Diana Harry Mary Peter Susan
// [sun:23:00,mon:00:00): Diana Peter Susan
// [mon:00:00,mon:00:30): Peter


