/* Boost.MultiIndex test for special set operations.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_special_set_ops.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <sstream>
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace boost::multi_index;

static int string_to_int(const std::string& str)
{
  std::istringstream iss(str);
  int                res;
  iss>>res;
  return res;
}

struct comp_int_string
{
  bool operator()(int x,const std::string& y)const
  {
    return x<string_to_int(y);
  }

  bool operator()(const std::string& x,int y)const
  {
    return string_to_int(x)<y;
  }
};

struct hash_string_as_int
{
  int operator()(const std::string& x)const
  {
    return boost::hash<int>()(string_to_int(x));
  }
};

struct eq_string_int
{
  bool operator()(int x,const std::string& y)const
  {
    return x==string_to_int(y);
  }

  bool operator()(const std::string& x,int y)const
  {
    return operator()(y,x);
  }
};

void test_special_set_ops()
{
  employee_set es;

  es.insert(employee(0,"Joe",31,1123));
  es.insert(employee(1,"Robert",27,5601));
  es.insert(employee(2,"John",40,7889));
  es.insert(employee(3,"Albert",20,9012));
  es.insert(employee(4,"John",57,1002));

  std::pair<employee_set_by_ssn::iterator,employee_set_by_ssn::iterator> p=
    get<ssn>(es).equal_range(
      "7889",hash_string_as_int(),eq_string_int());

  BOOST_TEST(std::distance(p.first,p.second)==1&&(p.first)->id==2);

  BOOST_TEST(
    get<ssn>(es).count(
      "5601",hash_string_as_int(),eq_string_int())==1);

  BOOST_TEST(
    get<ssn>(es).find(
      "1123",hash_string_as_int(),eq_string_int())->name=="Joe");

  BOOST_TEST(
    std::distance(
      get<age>(es).lower_bound("27",comp_int_string()),
      get<age>(es).upper_bound("40",comp_int_string()))==3);

  BOOST_TEST(es.count(2,employee::comp_id())==1);
  BOOST_TEST(es.count(5,employee::comp_id())==0);
}
