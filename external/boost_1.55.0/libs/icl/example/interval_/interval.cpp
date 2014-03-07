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
/** Example interval.cpp \file interval.cpp
    \brief Intervals for integral and continuous instance types. 
           Closed and open interval borders.

    Much of the library code deals with intervals which are implemented
    by interval class templates. This program gives a very short samlpe of 
    different interval instances.

    \include interval_/interval.cpp
*/
//[example_interval
#include <iostream>
#include <string>
#include <math.h>

// Dynamically bounded intervals
#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/continuous_interval.hpp>

// Statically bounded intervals
#include <boost/icl/right_open_interval.hpp>
#include <boost/icl/left_open_interval.hpp>
#include <boost/icl/closed_interval.hpp>
#include <boost/icl/open_interval.hpp>

#include "../toytime.hpp"
#include <boost/icl/rational.hpp>

using namespace std;
using namespace boost;
using namespace boost::icl;

int main()
{
    cout << ">>Interval Container Library: Sample interval.cpp <<\n";
    cout << "----------------------------------------------------\n";

    // Class template discrete_interval can be used for discrete data types
    // like integers, date and time and other types that have a least steppable
    // unit.
    discrete_interval<int>      int_interval  
        = construct<discrete_interval<int> >(3, 7, interval_bounds::closed());

    // Class template continuous_interval can be used for continuous data types
    // like double, boost::rational or strings.
    continuous_interval<double> sqrt_interval 
        = construct<continuous_interval<double> >(1/sqrt(2.0), sqrt(2.0));
                                                 //interval_bounds::right_open() is default
    continuous_interval<string> city_interval 
        = construct<continuous_interval<string> >("Barcelona", "Boston", interval_bounds::left_open());

    discrete_interval<Time>     time_interval 
        = construct<discrete_interval<Time> >(Time(monday,8,30), Time(monday,17,20), 
                                              interval_bounds::open());

    cout << "Dynamically bounded intervals:\n";
    cout << "  discrete_interval<int>:    " << int_interval  << endl;
    cout << "continuous_interval<double>: " << sqrt_interval << " does " 
                                            << string(contains(sqrt_interval, sqrt(2.0))?"":"NOT") 
                                            << " contain sqrt(2)" << endl;
    cout << "continuous_interval<string>: " << city_interval << " does "  
                                            << string(contains(city_interval,"Barcelona")?"":"NOT") 
                                            << " contain 'Barcelona'" << endl;
    cout << "continuous_interval<string>: " << city_interval << " does "  
                                            << string(contains(city_interval, "Berlin")?"":"NOT") 
                                            << " contain 'Berlin'" << endl;
    cout << "  discrete_interval<Time>:   " << time_interval << "\n\n";

    // There are statically bounded interval types with fixed interval borders
    right_open_interval<string>   fix_interval1; // You will probably use one kind of static intervals
                                                 // right_open_intervals are recommended.
    closed_interval<unsigned int> fix_interval2; // ... static closed, left_open and open intervals
    left_open_interval<float>     fix_interval3; // are implemented for sake of completeness but
    open_interval<short>          fix_interval4; // are of minor practical importance.

    right_open_interval<rational<int> > range1(rational<int>(0,1),  rational<int>(2,3));
    right_open_interval<rational<int> > range2(rational<int>(1,3),  rational<int>(1,1));

    // This middle third of the unit interval [0,1)
    cout << "Statically bounded interval:\n";
    cout << "right_open_interval<rational<int>>: " << (range1 & range2) << endl;

    return 0;
}

// Program output:

//>>Interval Container Library: Sample interval.cpp <<
//----------------------------------------------------
//Dynamically bounded intervals
//  discrete_interval<int>:    [3,7]
//continuous_interval<double>: [0.707107,1.41421) does NOT contain sqrt(2)
//continuous_interval<string>: (Barcelona,Boston] does NOT contain 'Barcelona'
//continuous_interval<string>: (Barcelona,Boston] does  contain 'Berlin'
//  discrete_interval<Time>:   (mon:08:30,mon:17:20)
//
//Statically bounded interval
//right_open_interval<rational<int>>: [1/3,2/3)

//]

