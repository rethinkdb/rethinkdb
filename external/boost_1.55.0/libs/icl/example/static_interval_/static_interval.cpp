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
/** Example static_interval.cpp \file static_interval.cpp
    \brief Intervals with static interval bounds.

    Intervals types with static or fixed interval bounds. Statically 
    bounded intervals use up to 33% less memory than dynamically
    bounded ones. Of the four possible statically bounded intervals types
    right_open_intervals are the most important ones. We can switch the
    library default to statically bounded intervals by defining
    BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS.

    \include static_interval_/static_interval.cpp
*/
//[example_static_interval
#include <iostream>
#include <string>
#include <math.h>
#include <boost/type_traits/is_same.hpp>

// We can change the library default for the interval types by defining 
#define BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
// prior to other inluces from the icl.
// The interval type that is automatically used with interval
// containers then is the statically bounded right_open_interval.

#include <boost/icl/interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>
// The statically bounded interval type 'right_open_interval'
// is indirectly included via interval containers.


#include "../toytime.hpp"
#include <boost/icl/rational.hpp>

using namespace std;
using namespace boost;
using namespace boost::icl;

int main()
{
    cout << ">> Interval Container Library: Sample static_interval.cpp <<\n";
    cout << "------------------------------------------------------------\n";

    // Statically bounded intervals are the user defined library default for 
    // interval parameters in interval containers now.
    BOOST_STATIC_ASSERT((
        boost::is_same< interval_set<int>::interval_type
                      , right_open_interval<int> >::value
                      )); 

    BOOST_STATIC_ASSERT((
        boost::is_same< interval_set<float>::interval_type
                      , right_open_interval<float> >::value
                      )); 

    // As we can see the library default both for discrete and continuous
    // domain_types T is 'right_open_interval<T>'.
    // The user defined library default for intervals is also available via 
    // the template 'interval':
    BOOST_STATIC_ASSERT((
        boost::is_same< interval<int>::type
                      , right_open_interval<int> >::value
                      )); 

    // Again we are declaring and initializing the four test intervals that have been used
    // in the example 'interval' and 'dynamic_interval'
    interval<int>::type    int_interval  = interval<int>::right_open(3, 8); // shifted the upper bound
    interval<double>::type sqrt_interval = interval<double>::right_open(1/sqrt(2.0), sqrt(2.0));

    // Interval ("Barcelona", "Boston"] can not be represented because there is no 'steppable next' on
    // lower bound "Barcelona". Ok. this is a different interval:
    interval<string>::type city_interval = interval<string>::right_open("Barcelona", "Boston");

    // Toy Time is discrete again so we can transfrom open(Time(monday,8,30), Time(monday,17,20))
    //                                       to right_open(Time(monday,8,31), Time(monday,17,20))
    interval<Time>::type   time_interval = interval<Time>::right_open(Time(monday,8,31), Time(monday,17,20));

    cout << "----- Statically bounded intervals ----------------------------------------\n";
    cout << "right_open_interval<int>   : " << int_interval  << endl;
    cout << "right_open_interval<double>: " << sqrt_interval << " does " 
                                            << string(contains(sqrt_interval, sqrt(2.0))?"":"NOT") 
                                            << " contain sqrt(2)" << endl;
    cout << "right_open_interval<string>: " << city_interval << " does "  
                                            << string(contains(city_interval,"Barcelona")?"":"NOT") 
                                            << " contain 'Barcelona'" << endl;
    cout << "right_open_interval<string>: " << city_interval << " does "  
                                            << string(contains(city_interval, "Boston")?"":"NOT") 
                                            << " contain 'Boston'" << endl;
    cout << "right_open_interval<Time>  : " << time_interval << "\n\n";

    // Using statically bounded intervals does not allows to apply operations
    // with elements on all interval containers, if their domain_type is continuous. 
    // The code that follows is identical to example 'dynamic_interval'. Only 'internally'
    // the library default for the interval template now is 'right_open_interval' 
    interval<rational<int> >::type unit_interval 
        = interval<rational<int> >::right_open(rational<int>(0), rational<int>(1));
    interval_set<rational<int> > unit_set(unit_interval);
    interval_set<rational<int> > ratio_set(unit_set);
    // ratio_set -= rational<int>(1,3); // This line will not compile, because we can not
                                        // represent a singleton interval as right_open_interval.
    return 0;
}

// Program output:
//>> Interval Container Library: Sample static_interval.cpp <<
//------------------------------------------------------------
//----- Statically bounded intervals ----------------------------------------
//right_open_interval<int>   : [3,8)
//right_open_interval<double>: [0.707107,1.41421) does NOT contain sqrt(2)
//right_open_interval<string>: [Barcelona,Boston) does  contain 'Barcelona'
//right_open_interval<string>: [Barcelona,Boston) does NOT contain 'Boston'
//right_open_interval<Time>  : [mon:08:31,mon:17:20)
//]

