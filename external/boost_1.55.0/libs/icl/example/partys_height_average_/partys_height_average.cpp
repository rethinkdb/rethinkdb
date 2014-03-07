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

/** Example partys_height_average.cpp \file partys_height_average.cpp
    \brief Using <i>aggregate on overlap</i> a history of height averages of 
           party guests is computed.

    In partys_height_average.cpp we compute yet another aggregation:
    The average height of guests as it changes over time. This is done by 
    defining a class counted_sum that sums up heights and counts the number 
    of guests via an operator +=.
    
    Based on the operator += we can aggregate counted sums on addition
    of interval value pairs into an interval_map.

    \include partys_height_average_/partys_height_average.cpp
*/
//[example_partys_height_average
// The next line includes <boost/date_time/posix_time/posix_time.hpp>
// and a few lines of adapter code.
#include <boost/icl/ptime.hpp> 
#include <iostream>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>

using namespace std;
using namespace boost::posix_time;
using namespace boost::icl;


class counted_sum
{
public:
    counted_sum():_sum(0),_count(0){}
    counted_sum(int sum):_sum(sum),_count(1){}

    int sum()const  {return _sum;}
    int count()const{return _count;}
    double average()const{ return _count==0 ? 0.0 : _sum/static_cast<double>(_count); }

    counted_sum& operator += (const counted_sum& right)
    { _sum += right.sum(); _count += right.count(); return *this; }

private:
    int _sum;
    int _count;
};

bool operator == (const counted_sum& left, const counted_sum& right)
{ return left.sum()==right.sum() && left.count()==right.count(); } 


void partys_height_average()
{
    interval_map<ptime, counted_sum> height_sums;

    height_sums +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 19:30"), 
          time_from_string("2008-05-20 23:00")), 
        counted_sum(165)); // Mary is 1,65 m tall.

    height_sums +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 19:30"), 
          time_from_string("2008-05-20 23:00")), 
        counted_sum(180)); // Harry is 1,80 m tall.

    height_sums +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 20:10"), 
          time_from_string("2008-05-21 00:00")), 
        counted_sum(170)); // Diana is 1,70 m tall.

    height_sums +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 20:10"), 
          time_from_string("2008-05-21 00:00")), 
        counted_sum(165)); // Susan is 1,65 m tall.

    height_sums +=
      make_pair( 
        discrete_interval<ptime>::right_open(
          time_from_string("2008-05-20 22:15"), 
          time_from_string("2008-05-21 00:30")), 
        counted_sum(200)); // Peters height is 2,00 m

    interval_map<ptime, counted_sum>::iterator height_sum_ = height_sums.begin();
    cout << "-------------- History of average guest height -------------------\n";
    while(height_sum_ != height_sums.end())
    {
        discrete_interval<ptime> when = height_sum_->first;

        double height_average = (*height_sum_++).second.average();
        cout << setprecision(3)
             << "[" << first(when) << " - " << upper(when) << ")"
             << ": " << height_average <<" cm = " << height_average/30.48 << " ft" << endl;
    }
}


int main()
{
    cout << ">>Interval Container Library: Sample partys_height_average.cpp  <<\n";
    cout << "------------------------------------------------------------------\n";
    partys_height_average();
    return 0;
}

// Program output:
/*-----------------------------------------------------------------------------
>>Interval Container Library: Sample partys_height_average.cpp  <<
------------------------------------------------------------------
-------------- History of average guest height -------------------
[2008-May-20 19:30:00 - 2008-May-20 20:10:00): 173 cm = 5.66 ft
[2008-May-20 20:10:00 - 2008-May-20 22:15:00): 170 cm = 5.58 ft
[2008-May-20 22:15:00 - 2008-May-20 23:00:00): 176 cm = 5.77 ft
[2008-May-20 23:00:00 - 2008-May-21 00:00:00): 178 cm = 5.85 ft
[2008-May-21 00:00:00 - 2008-May-21 00:30:00): 200 cm = 6.56 ft
-----------------------------------------------------------------------------*/
//]

