

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/gregorian/greg_serialize.hpp"
#include "boost/serialization/set.hpp"
#include "boost/serialization/list.hpp"
#include "boost/archive/text_oarchive.hpp"
#include "boost/archive/text_iarchive.hpp"
#include <iostream>
#include <fstream>

using namespace boost::gregorian;
typedef std::set<date> date_set;
typedef std::list<date> date_list;

void print(std::ostream& os, const date_set& ds) 
{
  os << "******** Date Set *********" << std::endl;
  date_set::const_iterator itr = ds.begin();
  while (itr != ds.end()) 
  {
    os << (*itr) << " ";
    itr++;
  }
  os << "\n***************************" << std::endl;
}

class foo {
 public:
  foo(date d = date(not_a_date_time), 
      int i = 0) :
    my_date(d),
    my_int(i)
  {}
  void insert_date(date d)
  {
    my_dates.push_back(d);
  }
  void print(std::ostream& os) const
  {
    os << "foo= my_date is: " << my_date 
       << " my_int is: " << my_int;
    date_list::const_iterator i = my_dates.begin();
    os << " Important dates: ";
    while (i != my_dates.end()) {
      os << (*i) << " ";
      i++;
    }
    os << std::endl;
  }
 private:
  friend class boost::serialization::access;
 
  // is a type of input archive the & operator is defined similar to >>.
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & my_date;
    ar & my_int;
    ar & my_dates;
  }

  date my_date;
  int my_int;
  date_list my_dates;
};


int
main()
{
  try {
    date d(2004, Apr, 5);
    std::cout << "Date: " << to_iso_string(d) << std::endl;
    std::cout << "Date: " << d << std::endl;
    std::ofstream ofs("date_demo.txt");
    boost::archive::text_oarchive oa(ofs);
    oa << d;
    
    std::cout << "Construct a foo" << std::endl;
    foo f(d, 1);
    f.insert_date(d+days(1));
    f.insert_date(d+days(2));
    f.insert_date(d+days(3));
    f.print(std::cout);
    oa << f;
    
    date_set dates;
    dates.insert(date(2004, Apr,1));
    dates.insert(date(2004, Apr,10));
    dates.insert(date(2004, Apr,15));
    print(std::cout, dates);
    
    oa << dates;
    ofs.close();

    std::cout << "Now do the input streaming" << std::endl;
    date d2(not_a_date_time);
    std::ifstream ifs("date_demo.txt");
    boost::archive::text_iarchive ia(ifs);
    ia >> d2;
    
    std::cout << "New date is: " << d2 << std::endl;
    
    foo f2;
    ia >> f2;
    f2.print(std::cout);
    
    date_set dates2;
    ia >> dates2; //exception here
    print(std::cout, dates2);

  }
  catch(std::exception& e) {
    std::cout << "Caught Exception: " << e.what() << std::endl;
  }

}


/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

