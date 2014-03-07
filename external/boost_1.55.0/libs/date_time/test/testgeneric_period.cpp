/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Bart Garst
 */

#include <iostream>
#include "boost/date_time/period.hpp"
#include "testfrmwk.hpp"

/*! duration_rep parameter for period requires a func unit() that
 * returns the smallest unit of measure for this type. This minimal
 * class fulfills that, and other, requirements */
template<class int_type>
class duration_type {
  public:
    duration_type(int_type a = 0) : _val(a) {}
    static int_type unit() { return 1; }
    int_type get_rep() { return _val; }
    bool operator==(duration_type<int_type> rhs) { return _val == rhs._val; }
    bool operator<(duration_type<int_type> rhs) { return _val < rhs._val; }
    bool operator>(duration_type<int_type> rhs) { return _val > rhs._val; }
  private:
    int_type _val;
};
//! To enable things like "cout << period.length()"
template<class int_type>
inline
std::ostream& operator<<(std::ostream& os, duration_type<int_type> dt){
  os << dt.get_rep();
  return os;
}
//! this operator is needed because period adds a duration_rep to a point_rep
template<class int_type>
inline
int_type operator+(int i, duration_type<int_type> dt){
  return i + dt.get_rep();
}

//! this operator is needed because period adds a duration_rep to a point_rep
template<class int_type>
inline
int_type operator-(int i, duration_type<int_type> dt){
  return i - dt.get_rep();
}


int main(){
  using namespace boost::date_time;
  typedef period<int, duration_type<int> > a_period;

  /*** check all functions - normal periods ***/

  a_period p1(1, duration_type<int>(9));
  check("Different constructors", p1 == a_period(1, 10));
  check("First", p1.begin() == 1); 
  check("Last", p1.last() == 9);
  check("End", p1.end() == 10);
  check("Length", p1.length() == 9);
  check("is_null (not)", !p1.is_null());
  {
    a_period p1(1, 10);
    check("First", p1.begin() == 1); 
    check("Last", p1.last() == 9);
    check("End", p1.end() == 10);
    check("Length", p1.length() == 9);
    check("is_null (not)", !p1.is_null());
  }

  a_period p2(5, 30);
  check("First", p2.begin() == 5); 
  check("Last", p2.last() == 29);
  check("End", p2.end() == 30);
  check("Length", p2.length() == 25);
  check("is_null (not)", !p2.is_null());

  a_period p3(35, 81);
  check("Operator ==", p1 == a_period(1,10));
  check("Operator !=", p1 != p2);
  check("Operator <", p1 < p3);
  check("Operator >", p3 > p2);

  {
    a_period p(1,10);
    p.shift(5);
    check("Shift (right)", p == a_period(6,15));
    p.shift(-15);
    check("Shift (left)", p == a_period(-9,0));
  }

  check("Contains rep", p2.contains(20));
  check("Contains rep (not)", !p2.contains(2));
  check("Contains period", p1.contains(a_period(2,8)));
  check("Contains period (not)", !p1.contains(p3));

  check("Intersects", p1.intersects(p2));
  check("Intersects", p2.intersects(p1));

  check("Adjacent", p1.is_adjacent(a_period(-5,1)));
  check("Adjacent", p1.is_adjacent(a_period(10,20)));
  check("Adjacent (not)", !p1.is_adjacent(p3));

  check("Is before", p1.is_before(15));
  check("Is after", p3.is_after(15));

  check("Intersection", (p1.intersection(p2) == a_period(5,10)));
  check("Intersection", (p1.intersection(p3).is_null()));

  check("Merge", p1.merge(p2) == a_period(1,30) );
  check("Merge", p1.merge(p3).is_null());
  
  check("Span", p3.span(p1) == a_period(1, 81));

  /*** zero length period ***/
  
  // treat a zero length period as a point
  a_period zero_len(3,duration_type<int>(0));
  check("Same beg & end == zero_length", 
      a_period(1,1) == a_period(1, duration_type<int>(0)));
  check("2 point (zero length) == 1 point zero duration",
      a_period(3,3) == zero_len);
 
  // zero_length period always returns false for is_before & is_after
  check("Is Before zero period", !zero_len.is_before(5));
  check("Is After zero period (not)", !zero_len.is_after(5));
  check("Is Before zero period (not)", !zero_len.is_before(-5));
  check("Is After zero period", !zero_len.is_after(-5));
 
  check("is_null", zero_len.is_null());
  check("Contains rep (not)", !zero_len.contains(20));
  // a null_period cannot contain any points
  check("Contains rep", !zero_len.contains(3));
  check("Contains period (not)", !zero_len.contains(a_period(5,8)));
  check("Contains period", p1.contains(zero_len));
  check("Intersects", zero_len.intersects(p1));
  check("Intersects", p1.intersects(zero_len));
  check("Adjacent", zero_len.is_adjacent(a_period(-10,3)));
  check("Adjacent", a_period(-10,3).is_adjacent(zero_len));
  check("Intersection", (zero_len.intersection(p1) == zero_len));
  check("Span", zero_len.span(p2) == a_period(3,30));

  /*** invalid period ***/

  a_period null_per(5,1);

  check("Is Before invalid period (always false)", !null_per.is_before(7));
  check("Is After invalid period (always false)", !null_per.is_after(7));
  check("Is Before invalid period (always false)", !null_per.is_before(-5));
  check("Is After invalid period (always false)", !null_per.is_after(-5));
 
  check("is_null", null_per.is_null());
  check("Contains rep larger (always false)", !null_per.contains(20));
  check("Contains rep in-between (always false)", !null_per.contains(3));
  check("Contains period (not)", !null_per.contains(a_period(7,9)));
  check("Contains period", p1.contains(null_per));
  check("Intersects", null_per.intersects(p1));
  check("Intersects", p1.intersects(null_per));
  check("Adjacent", null_per.is_adjacent(a_period(-10,5)));
  check("Adjacent", null_per.is_adjacent(a_period(1,10)));

  // what should this next one do?
  //check("Intersection", (null_per.intersection(p1) == zero_len));
  check("Span", null_per.span(p3) == a_period(5,81));

  {
    std::cout << std::endl;
    a_period p1(0, -2);
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == -3);
    check("End", p1.end() == -2);
    check("Length", p1.length() == -2);
    check("is_null", p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, -1);
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == -2);
    check("End", p1.end() == -1);
    check("Length", p1.length() == -1);
    check("is_null", p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, 0);
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == -1);
    check("End", p1.end() == 0);
    check("Length", p1.length() == 0);
    check("is_null", p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, 1);
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == 0);
    check("End", p1.end() == 1);
    check("Length", p1.length() == 1);
    check("is_null", !p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, 2);
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == 1);
    check("End", p1.end() == 2);
    check("Length", p1.length() == 2);
    check("is_null", !p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, duration_type<int>(-1));
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == -2);
    check("End", p1.end() == -1);
    check("Length", p1.length() == -1);
    check("is_null", p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, duration_type<int>(-2));
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == -3);
    check("End", p1.end() == -2);
    check("Length", p1.length() == -2);
    check("is_null", p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, duration_type<int>(0));
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == -1);
    check("End", p1.end() == 0);
    check("Length", p1.length() == 0);
    check("is_null", p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, duration_type<int>(1));
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == 0);
    check("End", p1.end() == 1);
    check("Length", p1.length() == 1);
    check("is_null", !p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(0, duration_type<int>(2));
    check("First", p1.begin() == 0); 
    check("Last", p1.last() == 1);
    check("End", p1.end() == 2);
    check("Length", p1.length() == 2);
    check("is_null", !p1.is_null());
  }
  {
    std::cout << std::endl;
    a_period p1(1,1); // length should be 0
    a_period p2(1,2); // length should be 1
    a_period p3(1,3); // length should be 2
    check("Length p1", p1.length() == 0);
    check("Length p2", p2.length() == 1);
    check("Length p3", p3.length() == 2);
    check("is_null p1 (not)", p1.is_null());
    check("is_null p2 (not)", !p2.is_null());
  }

  {
    a_period p1(1,2); // length should be 1
    p1.shift(duration_type<int>(1));
    a_period p2(2,3); // shifted result
    check("shift", p1 == p2);
  }
  {
    a_period p1(5,duration_type<int>(3)); 
    a_period p2(3,10); // expanded result
    p1.expand(duration_type<int>(2)); //from 2000-Jan-01--2000-Jan-04
    check("expand", p1 == p2);
  } 
  return printTestStats();
}
