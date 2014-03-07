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
/** Example overlap_counter.cpp \file overlap_counter.cpp

    \brief The most simple application of an interval map: 
           Counting the overlaps of added intervals.

    The most basic application of an interval_map is a counter counting
    the number of overlaps of intervals inserted into it.

    On could call an interval_map an aggregate on overlap machine. A very basic
    aggregation is summation of an integer. A interval_map<int,int> maps
    intervals of int to ints. 

    If we insert a value pair (discrete_interval<int>(2,6), 1) into the interval_map, it
    increases the content of all value pairs in the map by 1, if their interval
    part overlaps with discrete_interval<int>(2,6).

    \include overlap_counter_/overlap_counter.cpp
*/
//[example_overlap_counter
#include <iostream>
#include <boost/icl/split_interval_map.hpp>

using namespace std;
using namespace boost::icl;


/* The most simple example of an interval_map is an overlap counter.
   If intervals are added that are associated with the value 1,
   all overlaps of added intervals are counted as a result in the
   associated values. 
*/
typedef interval_map<int, int> OverlapCounterT;

void print_overlaps(const OverlapCounterT& counter)
{
    for(OverlapCounterT::const_iterator it = counter.begin(); it != counter.end(); it++)
    {
        discrete_interval<int> itv  = (*it).first;
        int overlaps_count = (*it).second;
        if(overlaps_count == 1)
            cout << "in interval " << itv << " intervals do not overlap" << endl;
        else
            cout << "in interval " << itv << ": "<< overlaps_count << " intervals overlap" << endl;
    }
}

void overlap_counter()
{
    OverlapCounterT overlap_counter;
    discrete_interval<int> inter_val;

    inter_val = discrete_interval<int>::right_open(4,8);
    cout << "-- adding   " << inter_val << " -----------------------------------------" << endl;
    overlap_counter += make_pair(inter_val, 1);
    print_overlaps(overlap_counter);
    cout << "-----------------------------------------------------------" << endl;

    inter_val = discrete_interval<int>::right_open(6,9);
    cout << "-- adding   " << inter_val << " -----------------------------------------" << endl;
    overlap_counter += make_pair(inter_val, 1);
    print_overlaps(overlap_counter);
    cout << "-----------------------------------------------------------" << endl;

    inter_val = discrete_interval<int>::right_open(1,9);
    cout << "-- adding   " << inter_val << " -----------------------------------------" << endl;
    overlap_counter += make_pair(inter_val, 1);
    print_overlaps(overlap_counter);
    cout << "-----------------------------------------------------------" << endl;
    
}

int main()
{
    cout << ">>Interval Container Library: Sample overlap_counter.cpp <<\n";
    cout << "-----------------------------------------------------------\n";
    overlap_counter();
    return 0;
}

// Program output:

// >>Interval Container Library: Sample overlap_counter.cpp <<
// -----------------------------------------------------------
// -- adding   [4,8) -----------------------------------------
// in interval [4,8) intervals do not overlap
// -----------------------------------------------------------
// -- adding   [6,9) -----------------------------------------
// in interval [4,6) intervals do not overlap
// in interval [6,8): 2 intervals overlap
// in interval [8,9) intervals do not overlap
// -----------------------------------------------------------
// -- adding   [1,9) -----------------------------------------
// in interval [1,4) intervals do not overlap
// in interval [4,6): 2 intervals overlap
// in interval [6,8): 3 intervals overlap
// in interval [8,9): 2 intervals overlap
// -----------------------------------------------------------
//]
