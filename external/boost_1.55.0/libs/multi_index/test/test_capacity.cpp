/* Boost.MultiIndex test for capacity memfuns.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_capacity.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace boost::multi_index;

void test_capacity()
{
  employee_set es;

  es.insert(employee(0,"Joe",31,1123));
  es.insert(employee(1,"Robert",27,5601));
  es.insert(employee(2,"John",40,7889));
  es.insert(employee(3,"Albert",20,9012));
  es.insert(employee(4,"John",57,1002));

  BOOST_TEST(!es.empty());
  BOOST_TEST(es.size()==5);
  BOOST_TEST(es.size()<=es.max_size());

  es.erase(es.begin());
  BOOST_TEST(!get<name>(es).empty());
  BOOST_TEST(get<name>(es).size()==4);
  BOOST_TEST(get<name>(es).size()<=get<name>(es).max_size());

  es.erase(es.begin());
  BOOST_TEST(!get<as_inserted>(es).empty());
  BOOST_TEST(get<as_inserted>(es).size()==3);
  BOOST_TEST(get<as_inserted>(es).size()<=get<as_inserted>(es).max_size());

  multi_index_container<int,indexed_by<sequenced<> > > ss;

  ss.resize(10);
  BOOST_TEST(ss.size()==10);
  BOOST_TEST(ss.size()<=ss.max_size());

  ss.resize(20,666);
  BOOST_TEST(ss.size()==20);
  BOOST_TEST(ss.back()==666);

  ss.resize(5,10);
  BOOST_TEST(ss.size()==5);

  ss.resize(4);
  BOOST_TEST(ss.size()==4);

  multi_index_container<int,indexed_by<random_access<> > > rs;

  rs.resize(10);
  BOOST_TEST(rs.size()==10);
  BOOST_TEST(rs.size()<=rs.max_size());
  BOOST_TEST(rs.size()<=rs.capacity());

  rs.resize(20,666);
  BOOST_TEST(rs.size()==20);
  BOOST_TEST(rs.back()==666);
  BOOST_TEST(rs.size()<=rs.capacity());

  unsigned int c=rs.capacity();
  rs.resize(10,20);
  BOOST_TEST(rs.size()==10);
  BOOST_TEST(rs.capacity()==c);

  rs.resize(5);
  BOOST_TEST(rs.size()==5);
  BOOST_TEST(rs.capacity()==c);

  rs.reserve(100);
  BOOST_TEST(rs.size()==5);
  BOOST_TEST(rs.capacity()>=100);

  c=rs.capacity();
  rs.reserve(99);
  BOOST_TEST(rs.size()==5);
  BOOST_TEST(rs.capacity()==c);

  rs.shrink_to_fit();
  BOOST_TEST(rs.size()==5);
  BOOST_TEST(rs.capacity()==rs.size());

  rs.clear();
  rs.shrink_to_fit();
  BOOST_TEST(rs.capacity()==0);
}
