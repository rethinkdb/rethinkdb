/* Boost.MultiIndex test for projection capabilities.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_projection.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace boost::multi_index;

void test_projection()
{
  employee_set          es;
  es.insert(employee(0,"Joe",31,1123));
  es.insert(employee(1,"Robert",27,5601));
  es.insert(employee(2,"John",40,7889));
  es.insert(employee(3,"Albert",20,9012));
  es.insert(employee(4,"John",57,1002));

  employee_set::iterator             it,itbis;
  employee_set_by_name::iterator     it1;
  employee_set_by_age::iterator      it2;
  employee_set_as_inserted::iterator it3;
  employee_set_by_ssn::iterator      it4;
  employee_set_randomly::iterator    it5;

  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set::iterator,
    nth_index_iterator<employee_set,0>::type >::value));
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_by_name::iterator,
    nth_index_iterator<employee_set,1>::type >::value));
#if defined(BOOST_NO_MEMBER_TEMPLATES)
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_by_age::iterator,
    index_iterator<employee_set,age>::type >::value));
#else
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_by_age::iterator,
    employee_set::index_iterator<age>::type >::value));
#endif
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_as_inserted::iterator,
    nth_index_iterator<employee_set,3>::type >::value));
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_by_ssn::iterator,
    nth_index_iterator<employee_set,4>::type >::value));
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_randomly::iterator,
    nth_index_iterator<employee_set,5>::type >::value));

  it=   es.find(employee(1,"Robert",27,5601));
  it1=  project<name>(es,it);
  it2=  project<age>(es,it1);
  it3=  project<as_inserted>(es,it2);
  it4=  project<ssn>(es,it3);
  it5=  project<randomly>(es,it4);
#if defined(BOOST_NO_MEMBER_TEMPLATES)
  itbis=project<0>(es,it5);
#else
  itbis=es.project<0>(it5);
#endif

  BOOST_TEST(
    *it==*it1&&*it1==*it2&&*it2==*it3&&*it3==*it4&&*it4==*it5&&itbis==it);

  BOOST_TEST(project<name>(es,es.end())==get<name>(es).end());
  BOOST_TEST(project<age>(es,es.end())==get<age>(es).end());
  BOOST_TEST(project<as_inserted>(es,es.end())==get<as_inserted>(es).end());
  BOOST_TEST(project<ssn>(es,es.end())==get<ssn>(es).end());
  BOOST_TEST(project<randomly>(es,es.end())==get<randomly>(es).end());

  const employee_set& ces=es;

  employee_set::const_iterator             cit,citbis;
  employee_set_by_name::const_iterator     cit1;
  employee_set_by_age::const_iterator      cit2;
  employee_set_as_inserted::const_iterator cit3;
  employee_set_by_ssn::const_iterator      cit4;
  employee_set_randomly::const_iterator    cit5;

  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set::const_iterator,
    nth_index_const_iterator<employee_set,0>::type >::value));
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_by_name::const_iterator,
    nth_index_const_iterator<employee_set,1>::type >::value));
#if defined(BOOST_NO_MEMBER_TEMPLATES)
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_by_age::const_iterator,
    index_const_iterator<employee_set,age>::type >::value));
#else
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_by_age::const_iterator,
    employee_set::index_const_iterator<age>::type >::value));
#endif
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_as_inserted::const_iterator,
    nth_index_const_iterator<employee_set,3>::type >::value));
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_by_ssn::const_iterator,
    nth_index_const_iterator<employee_set,4>::type >::value));
  BOOST_STATIC_ASSERT((boost::is_same<
    employee_set_randomly::const_iterator,
    nth_index_const_iterator<employee_set,5>::type >::value));

  cit=   ces.find(employee(4,"John",57,1002));
#if defined(BOOST_NO_MEMBER_TEMPLATES)
  cit1=  project<by_name>(ces,cit);
#else
  cit1=  ces.project<by_name>(cit);
#endif
  cit2=  project<age>(ces,cit1);
#if defined(BOOST_NO_MEMBER_TEMPLATES)
  cit3=  project<as_inserted>(ces,cit2);
#else
  cit3=  ces.project<as_inserted>(cit2);
#endif
  cit4=  project<ssn>(ces,cit3);
  cit5=  project<randomly>(ces,cit4);
  citbis=project<0>(ces,cit5);

  BOOST_TEST(
    *cit==*cit1&&*cit1==*cit2&&*cit2==*cit3&&*cit3==*cit4&&*cit4==*cit5&&
    citbis==cit);
}
