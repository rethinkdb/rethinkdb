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

/** Example custom_interval.cpp \file custom_interval.cpp 
    \brief Shows how to use interval containers with own interval classes. 

    There may be instances, where we want to use interval container with our
    own user defined interval classes. Boost interval containers can be adapted
    to your interval class by partial template specialisation. Only a few lines
    of code are needed to achieve this.

    \include custom_interval_/custom_interval.cpp
*/
//[example_custom_interval
#include <iostream>
#include <boost/icl/interval_set.hpp>

using namespace std;
using namespace boost::icl;

// Here is a typical class that may model intervals in your application.
class MyInterval
{
public:
    MyInterval(): _first(), _past(){}
    MyInterval(int lo, int up): _first(lo), _past(up){}
    int first()const{ return _first; }
    int past ()const{ return _past; }
private:
    int _first, _past;
};

namespace boost{ namespace icl 
{
// Class template interval_traits serves as adapter to register and customize your interval class
template<>
struct interval_traits< MyInterval >       //1.  Partially specialize interval_traits for 
{                                          //    your class MyInterval
                                           //2.  Define associated types
    typedef MyInterval     interval_type;  //2.1 MyInterval will be the interval_type
    typedef int            domain_type;    //2.2 The elements of the domain are ints 
    typedef std::less<int> domain_compare; //2.3 This is the way our element shall be ordered.
                                           //3.  Next we define the essential functions 
                                           //    of the specialisation
                                           //3.1 Construction of intervals
    static interval_type construct(const domain_type& lo, const domain_type& up) 
    { return interval_type(lo, up); }        
                                           //3.2 Selection of values 
    static domain_type lower(const interval_type& inter_val){ return inter_val.first(); };
    static domain_type upper(const interval_type& inter_val){ return inter_val.past(); };
};

template<>
struct interval_bound_type<MyInterval>     //4.  Finally we define the interval borders.
{                                          //    Choose between static_open         (lo..up)
    typedef interval_bound_type type;      //                   static_left_open    (lo..up]
    BOOST_STATIC_CONSTANT(bound_type, value = interval_bounds::static_right_open);//[lo..up)
};                                         //               and static_closed       [lo..up] 

}} // namespace boost icl

void custom_interval()
{
    // Now we can use class MyInterval with interval containers:
    typedef interval_set<int, std::less, MyInterval> MyIntervalSet;
    MyIntervalSet mySet;
    mySet += MyInterval(1,9);
    cout << mySet << endl;
    mySet.subtract(3) -= 6;
    cout << mySet << "            subtracted 3 and 6\n";
    mySet ^= MyInterval(2,8);
    cout << mySet <<      "  flipped between 2 and 7\n";
}


int main()
{
    cout << ">>Interval Container Library: Sample custom_interval.cpp <<\n";
    cout << "-----------------------------------------------------------\n";
    cout << "This program uses a user defined interval class:\n";
    custom_interval();
    return 0;
}

// Program output:
/*-----------------------------------------------------------------------------
>>Interval Container Library: Sample custom_interval.cpp <<
-----------------------------------------------------------
This program uses a user defined interval class:
{[1,                      9)}
{[1,  3)   [4,  6)   [7,  9)}       subtracted 3 and 6
{[1,2) [3,4)     [6,7) [8,9)}  flipped between 2 and 7
-----------------------------------------------------------------------------*/
//]

