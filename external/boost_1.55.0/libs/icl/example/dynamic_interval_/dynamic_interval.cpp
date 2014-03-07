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
/** Example dynamic_interval.cpp \file dynamic_interval.cpp
    \brief Intervals with dynamic interval bounds that can be changed at runtime.

    Intervals types with dynamic interval bounds can represent closed and
    open interval borders. Interval borders are not static or fixed for
    the type but may change due to computations in interval containers.
    Dynamically bounded intervals are the library default for interval
    parameters in interval containers.

    \include dynamic_interval_/dynamic_interval.cpp
*/
//[example_dynamic_interval
#include <iostream>
#include <string>
#include <math.h>
#include <boost/type_traits/is_same.hpp>

#include <boost/icl/interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>
// Dynamically bounded intervals 'discrete_interval' and 'continuous_interval'
// are indirectly included via interval containers as library defaults.
#include "../toytime.hpp"
#include <boost/icl/rational.hpp>

using namespace std;
using namespace boost;
using namespace boost::icl;

int main()
{
    cout << ">>Interval Container Library: Sample interval.cpp <<\n";
    cout << "----------------------------------------------------\n";

    // Dynamically bounded intervals are the library default for 
    // interval parameters in interval containers.
    BOOST_STATIC_ASSERT((
        boost::is_same< interval_set<int>::interval_type
                      , discrete_interval<int> >::value
                      )); 


    BOOST_STATIC_ASSERT((
        boost::is_same< interval_set<float>::interval_type
                      , continuous_interval<float> >::value
                      )); 

    // As we can see the library default chooses the appropriate
    // class template instance discrete_interval<T> or continuous_interval<T>
    // dependent on the domain_type T. The library default for intervals
    // is also available via the template 'interval':
    BOOST_STATIC_ASSERT((
        boost::is_same< interval<int>::type
                      , discrete_interval<int> >::value
                      )); 

    BOOST_STATIC_ASSERT((
        boost::is_same< interval<float>::type
                      , continuous_interval<float> >::value
                      )); 

    // template interval also provides static functions for the four border types

    interval<int>::type    int_interval  = interval<int>::closed(3, 7);
    interval<double>::type sqrt_interval = interval<double>::right_open(1/sqrt(2.0), sqrt(2.0));
    interval<string>::type city_interval = interval<string>::left_open("Barcelona", "Boston");
    interval<Time>::type   time_interval = interval<Time>::open(Time(monday,8,30), Time(monday,17,20));

    cout << "----- Dynamically bounded intervals ----------------------------------------\n";
    cout << "  discrete_interval<int>   : " << int_interval  << endl;
    cout << "continuous_interval<double>: " << sqrt_interval << " does " 
                                            << string(contains(sqrt_interval, sqrt(2.0))?"":"NOT") 
                                            << " contain sqrt(2)" << endl;
    cout << "continuous_interval<string>: " << city_interval << " does "  
                                            << string(contains(city_interval,"Barcelona")?"":"NOT") 
                                            << " contain 'Barcelona'" << endl;
    cout << "continuous_interval<string>: " << city_interval << " does "  
                                            << string(contains(city_interval, "Berlin")?"":"NOT") 
                                            << " contain 'Berlin'" << endl;
    cout << "  discrete_interval<Time>  : " << time_interval << "\n\n";

    // Using dynamically bounded intervals allows to apply operations
    // with intervals and also with elements on all interval containers 
    // including interval containers of continuous domain types:

    interval<rational<int> >::type unit_interval 
        = interval<rational<int> >::right_open(rational<int>(0), rational<int>(1));
    interval_set<rational<int> > unit_set(unit_interval);
    interval_set<rational<int> > ratio_set(unit_set);
    ratio_set -= rational<int>(1,3); // Subtract 1/3 from the set

    cout << "----- Manipulation of single values in continuous sets ---------------------\n";
    cout << "1/3 subtracted from [0..1) : " << ratio_set << endl;
    cout << "The set does " << string(contains(ratio_set, rational<int>(1,3))?"":"NOT") 
                                            << " contain '1/3'" << endl;
    ratio_set ^= unit_set;
    cout << "Flipping the holey set     : " << ratio_set << endl;
    cout << "yields the subtracted      :     1/3\n\n";

    // Of course we can use interval types that are different from the
    // library default by explicit instantiation:
    split_interval_set<int, std::less, closed_interval<Time> > intuitive_times;
    // Interval set 'intuitive_times' uses statically bounded closed intervals
    intuitive_times += closed_interval<Time>(Time(monday,  9,00), Time(monday, 10,59));
    intuitive_times += closed_interval<Time>(Time(monday, 10,00), Time(monday, 11,59));
    cout << "----- Here we are NOT using the library default for intervals --------------\n";
    cout << intuitive_times << endl;

    return 0;
}

// Program output:
//>>Interval Container Library: Sample interval.cpp <<
//----------------------------------------------------
//----- Dynamically bounded intervals ----------------------------------------
//  discrete_interval<int>   : [3,7]
//continuous_interval<double>: [0.707107,1.41421) does NOT contain sqrt(2)
//continuous_interval<string>: (Barcelona,Boston] does NOT contain 'Barcelona'
//continuous_interval<string>: (Barcelona,Boston] does  contain 'Berlin'
//  discrete_interval<Time>  : (mon:08:30,mon:17:20)
//
//----- Manipulation of single values in continuous sets ---------------------
//1/3 subtracted from [0..1) : {[0/1,1/3)(1/3,1/1)}
//The set does NOT contain '1/3'
//Flipping the holey set     : {[1/3,1/3]}
//yields the subtracted      :     1/3
//
//----- Here we are NOT using the library default for intervals --------------
//{[mon:09:00,mon:09:59][mon:10:00,mon:10:59][mon:11:00,mon:11:59]}
//]

