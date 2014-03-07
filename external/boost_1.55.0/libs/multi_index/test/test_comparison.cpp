/* Boost.MultiIndex test for comparison functions.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_comparison.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace boost::multi_index;

template<typename Value>
struct lookup_list{
  typedef multi_index_container<
    Value,
    indexed_by<
      sequenced<>,
      ordered_non_unique<identity<Value> >
    >
  > type;
};

template<typename Value>
struct lookup_vector{
  typedef multi_index_container<
    Value,
    indexed_by<
      random_access<>,
      ordered_non_unique<identity<Value> >
    >
  > type;
};

void test_comparison()
{
  employee_set              es;
  employee_set_by_age&      i2=get<2>(es);
  employee_set_as_inserted& i3=get<3>(es);
  employee_set_randomly&    i5=get<5>(es);
  es.insert(employee(0,"Joe",31,1123));
  es.insert(employee(1,"Robert",27,5601));
  es.insert(employee(2,"John",40,7889));
  es.insert(employee(3,"Albert",20,9012));
  es.insert(employee(4,"John",57,1002));

  employee_set              es2;
  employee_set_by_age&      i22=get<age>(es2);
  employee_set_as_inserted& i32=get<3>(es2);
  employee_set_randomly&    i52=get<5>(es2);
  es2.insert(employee(0,"Joe",31,1123));
  es2.insert(employee(1,"Robert",27,5601));
  es2.insert(employee(2,"John",40,7889));
  es2.insert(employee(3,"Albert",20,9012));

  BOOST_TEST(es==es&&es<=es&&es>=es&&
              i22==i22&&i22<=i22&&i22>=i22&&
              i32==i32&&i32<=i32&&i32>=i32&&
              i52==i52&&i52<=i52&&i52>=i52);
  BOOST_TEST(es!=es2&&es2<es&&es>es2&&!(es<=es2)&&!(es2>=es));
  BOOST_TEST(i2!=i22&&i22<i2&&i2>i22&&!(i2<=i22)&&!(i22>=i2));
  BOOST_TEST(i3!=i32&&i32<i3&&i3>i32&&!(i3<=i32)&&!(i32>=i3));
  BOOST_TEST(i5!=i52&&i52<i5&&i5>i52&&!(i5<=i52)&&!(i52>=i5));

  lookup_list<int>::type    l1;
  lookup_list<char>::type   l2;
  lookup_vector<char>::type l3;
  lookup_list<long>::type   l4;
  lookup_vector<long>::type l5;

  l1.push_back(3);
  l1.push_back(4);
  l1.push_back(5);
  l1.push_back(1);
  l1.push_back(2);

  l2.push_back(char(3));
  l2.push_back(char(4));
  l2.push_back(char(5));
  l2.push_back(char(1));
  l2.push_back(char(2));

  l3.push_back(char(3));
  l3.push_back(char(4));
  l3.push_back(char(5));
  l3.push_back(char(1));
  l3.push_back(char(2));

  l4.push_back(long(3));
  l4.push_back(long(4));
  l4.push_back(long(5));
  l4.push_back(long(1));

  l5.push_back(long(3));
  l5.push_back(long(4));
  l5.push_back(long(5));
  l5.push_back(long(1));

  BOOST_TEST(l1==l2&&l1<=l2&&l1>=l2);
  BOOST_TEST(
    get<1>(l1)==get<1>(l2)&&get<1>(l1)<=get<1>(l2)&&get<1>(l1)>=get<1>(l2));
  BOOST_TEST(
    get<1>(l1)==get<1>(l3)&&get<1>(l1)<=get<1>(l3)&&get<1>(l1)>=get<1>(l3));
  BOOST_TEST(l1!=l4&&l4<l1&&l1>l4);
  BOOST_TEST(
    get<1>(l1)!=get<1>(l4)&&get<1>(l1)<get<1>(l4)&&get<1>(l4)>get<1>(l1));
  BOOST_TEST(l3!=l5&&l5<l3&&l3>l5);
  BOOST_TEST(
    get<1>(l3)!=get<1>(l5)&&get<1>(l3)<get<1>(l5)&&get<1>(l5)>get<1>(l3));
}
