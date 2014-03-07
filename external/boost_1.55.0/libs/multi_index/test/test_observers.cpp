/* Boost.MultiIndex test for observer memfuns.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_observers.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <vector>
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace boost::multi_index;

void test_observers()
{
  employee_set                es;
  const employee_set_by_name& i1=get<by_name>(es);
  const employee_set_by_age&  i2=get<age>(es);

  es.insert(employee(0,"Joe",31,1123));
  es.insert(employee(1,"Robert",27,5601));
  es.insert(employee(2,"John",40,7889));
  es.insert(employee(3,"Albert",20,9012));
  es.insert(employee(4,"John",57,1002));

  {
    employee_set_by_name::key_from_value k=i1.key_extractor();
    employee_set_by_name::hasher         h=i1.hash_function();
    employee_set_by_name::key_equal      eq=i1.key_eq();

    employee_set_by_name::const_iterator it0=i1.equal_range("John").first;
    employee_set_by_name::const_iterator it1=it0;++it1;
    BOOST_TEST(k(*it0)=="John"&&k(*it1)=="John");
    BOOST_TEST(h(k(*it0))==h(k(*it1)));
    BOOST_TEST(eq(k(*it0),k(*it1))==true);
  }
  {
    employee_set_by_age::key_from_value k=i2.key_extractor();
    employee_set_by_age::key_compare    c=i2.key_comp();
    employee_set_by_age::value_compare  vc=i2.value_comp();

    employee_set_by_age::const_iterator it0=i2.find(31);
    employee_set_by_age::const_iterator it1=i2.find(40);
    BOOST_TEST(k(*it0)==31&&k(*it1)==40);
    BOOST_TEST(c(k(*it0),k(*it1))==true);
    BOOST_TEST(vc(*it0,*it1)==true);
  }
}
