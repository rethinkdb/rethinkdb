/* Boost.MultiIndex test for standard set operations.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_set_ops.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <vector>
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace boost::multi_index;

void test_set_ops()
{
  employee_set               es;
  employee_set_by_name&      i1=get<by_name>(es);
  const employee_set_by_age& i2=get<age>(es);
  employee_set_by_ssn&       i4=get<ssn>(es);

  es.insert(employee(0,"Joe",31,1123));
  es.insert(employee(1,"Robert",27,5601));
  es.insert(employee(2,"John",40,7889));
  es.insert(employee(3,"Albert",20,9012));
  es.insert(employee(4,"John",57,1002));

  BOOST_TEST(i1.find("John")->name=="John");
  BOOST_TEST(i2.find(41)==i2.end());
  BOOST_TEST(i4.find(5601)->name=="Robert");

  BOOST_TEST(i1.count("John")==2);
  BOOST_TEST(es.count(employee(10,"",-1,0))==0);
  BOOST_TEST(i4.count(7881)==0);

  BOOST_TEST(
    std::distance(
      i2.lower_bound(31),
      i2.upper_bound(60))==3);

  std::pair<employee_set_by_name::iterator,employee_set_by_name::iterator> p=
    i1.equal_range("John");
  BOOST_TEST(std::distance(p.first,p.second)==2);

  p=i1.equal_range("Serena");
  BOOST_TEST(p.first==i1.end()&&p.second==i1.end());

  std::pair<employee_set_by_age::iterator,employee_set_by_age::iterator> p2=
    i2.equal_range(30);
  BOOST_TEST(p2.first==p2.second&&p2.first->age==31);
}
