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
/** Example interval_container.cpp \file interval_container.cpp

    \brief Demonstrates basic characteristics of interval container objects.

    \include interval_container_/interval_container.cpp
*/
//[example_interval_container
#include <iostream>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/split_interval_map.hpp>
#include "../toytime.hpp"

using namespace std;
using namespace boost::icl;

void interval_container_basics()
{
    interval<Time>::type night_and_day(Time(monday,   20,00), Time(tuesday,  20,00));
    interval<Time>::type day_and_night(Time(tuesday,   7,00), Time(wednesday, 7,00));
    interval<Time>::type  next_morning(Time(wednesday, 7,00), Time(wednesday,10,00));
    interval<Time>::type  next_evening(Time(wednesday,18,00), Time(wednesday,21,00));

    // An interval set of type interval_set joins intervals that that overlap or touch each other.
    interval_set<Time> joinedTimes;
    joinedTimes.insert(night_and_day);
    joinedTimes.insert(day_and_night); //overlapping in 'day' [07:00, 20.00)
    joinedTimes.insert(next_morning);  //touching
    joinedTimes.insert(next_evening);  //disjoint

    cout << "Joined times  :" << joinedTimes << endl;

    // A separate interval set of type separate_interval_set joins intervals that that 
    // overlap but it preserves interval borders that just touch each other. You may 
    // represent time grids like the months of a year as a split_interval_set.
    separate_interval_set<Time> separateTimes;
    separateTimes.insert(night_and_day);
    separateTimes.insert(day_and_night); //overlapping in 'day' [07:00, 20.00)
    separateTimes.insert(next_morning);  //touching
    separateTimes.insert(next_evening);  //disjoint

    cout << "Separate times:" << separateTimes << endl;

    // A split interval set of type split_interval_set preserves all interval
    // borders. On insertion of overlapping intervals the intervals in the
    // set are split up at the interval borders of the inserted interval.
    split_interval_set<Time> splitTimes;
    splitTimes += night_and_day;
    splitTimes += day_and_night; //overlapping in 'day' [07:00, 20:00)
    splitTimes += next_morning;  //touching
    splitTimes += next_evening;  //disjoint

    cout << "Split times   :\n" << splitTimes << endl;

    // A split interval map splits up inserted intervals on overlap and aggregates the
    // associated quantities via the operator +=
    split_interval_map<Time, int> overlapCounter;
    overlapCounter += make_pair(night_and_day,1);
    overlapCounter += make_pair(day_and_night,1); //overlapping in 'day' [07:00, 20.00)
    overlapCounter += make_pair(next_morning, 1); //touching
    overlapCounter += make_pair(next_evening, 1); //disjoint

    cout << "Split times overlap counted:\n" << overlapCounter << endl;

    // An interval map joins touching intervals, if associated values are equal
    interval_map<Time, int> joiningOverlapCounter;
    joiningOverlapCounter = overlapCounter;
    cout << "Times overlap counted:\n" << joiningOverlapCounter << endl;
}

int main()
{
    cout << ">>Interval Container Library: Sample interval_container.cpp <<\n";
    cout << "--------------------------------------------------------------\n";
    interval_container_basics();
    return 0;
}


// Program output:
/* ----------------------------------------------------------------------------
>>Interval Container Library: Sample interval_container.cpp <<
--------------------------------------------------------------
Joined times  :[mon:20:00,wed:10:00)[wed:18:00,wed:21:00)
Separate times:[mon:20:00,wed:07:00)[wed:07:00,wed:10:00)[wed:18:00,wed:21:00)
Split times   :
[mon:20:00,tue:07:00)[tue:07:00,tue:20:00)[tue:20:00,wed:07:00)
[wed:07:00,wed:10:00)[wed:18:00,wed:21:00)
Split times overlap counted:
{([mon:20:00,tue:07:00)->1)([tue:07:00,tue:20:00)->2)([tue:20:00,wed:07:00)->1)
([wed:07:00,wed:10:00)->1)([wed:18:00,wed:21:00)->1)}
Times overlap counted:
{([mon:20:00,tue:07:00)->1)([tue:07:00,tue:20:00)->2)([tue:20:00,wed:10:00)->1)
([wed:18:00,wed:21:00)->1)}
-----------------------------------------------------------------------------*/
//]
