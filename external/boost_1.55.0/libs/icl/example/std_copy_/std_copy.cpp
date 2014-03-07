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
/** Example std_copy.cpp \file std_copy.cpp
    \brief Fill interval containers using std::copy. 

    Example std_copy shows how algorithm std::copy can be used to
    fill interval containers from other std::containers and how copying
    to interval containers differs from other uses of std::copy.

    \include std_copy_/std_copy.cpp
*/
//[example_std_copy
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/icl/interval_map.hpp>

using namespace std;
using namespace boost;
using namespace boost::icl;

// 'make_segments' returns a vector of interval value pairs, which
// are not sorted. The values are taken from the minimal example
// in section 'interval combining styles'.
vector<pair<discrete_interval<int>, int> > make_segments()
{
    vector<pair<discrete_interval<int>, int> > segment_vec;
    segment_vec.push_back(make_pair(discrete_interval<int>::right_open(2,4), 1));
    segment_vec.push_back(make_pair(discrete_interval<int>::right_open(4,5), 1));
    segment_vec.push_back(make_pair(discrete_interval<int>::right_open(1,3), 1));
    return segment_vec;
}

// 'show_segments' displays the source segements.
void show_segments(const vector<pair<discrete_interval<int>, int> >& segments)
{
    vector<pair<discrete_interval<int>, int> >::const_iterator iter = segments.begin();
    while(iter != segments.end())
    {
        cout << "(" << iter->first << "," << iter->second << ")";
        ++iter;
    }
}

void std_copy()
{
    // So we have some segments stored in an std container.
    vector<pair<discrete_interval<int>, int> > segments = make_segments(); 
    // Display the input
    cout << "input sequence: "; show_segments(segments); cout << "\n\n";

    // We are going to 'std::copy' those segments into an interval_map:
    interval_map<int,int> segmap;

    // Use an 'icl::inserter' from <boost/icl/iterator.hpp> to call 
    // insertion on the interval container.
    std::copy(segments.begin(), segments.end(), 
              icl::inserter(segmap, segmap.end()));
    cout << "icl::inserting: " << segmap << endl;
    segmap.clear();

    // When we are feeding data into interval_maps, most of the time we are
    // intending to compute an aggregation result. So we are not interested
    // the std::insert semantincs but the aggregating icl::addition semantics.
    // To achieve this there is an icl::add_iterator and an icl::adder function 
    // provided in <boost/icl/iterator.hpp>.
    std::copy(segments.begin(), segments.end(), 
              icl::adder(segmap, segmap.end())); //Aggregating associated values
    cout << "icl::adding   : " << segmap << endl; 

    // In this last case, the semantics of 'std::copy' transforms to the 
    // generalized addition operation, that is implemented by operator
    // += or + on itl maps and sets.
}

int main()
{
    cout << ">>    Interval Container Library: Example std_copy.cpp    <<\n";
    cout << "-----------------------------------------------------------\n";
    cout << "Using std::copy to fill an interval_map:\n\n";

    std_copy();
    return 0;
}

// Program output:
/*---------------------------------------------------------
>>    Interval Container Library: Example std_copy.cpp    <<
-----------------------------------------------------------
Using std::copy to fill an interval_map:

input sequence: ([2,4),1)([4,5),1)([1,3),1)

icl::inserting: {([1,5)->1)}
icl::adding   : {([1,2)->1)([2,3)->2)([3,5)->1)}
---------------------------------------------------------*/
//]
