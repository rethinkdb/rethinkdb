/* Boost.MultiIndex test for copying and assignment.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_copy_assignment.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/move/utility.hpp>
#include <list>
#include <numeric>
#include <vector>
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace boost::multi_index;

#if BOOST_WORKAROUND(__MWERKS__,<=0x3003)
/* The "ISO C++ Template Parser" option makes CW8.3 incorrectly fail at
 * expressions of the form sizeof(x) where x is an array local to a
 * template function.
 */

#pragma parse_func_templ off
#endif

typedef multi_index_container<int> copyable_and_movable;

struct holder
{
  copyable_and_movable c;
};

template<typename Sequence>
static void test_assign(BOOST_EXPLICIT_TEMPLATE_TYPE(Sequence))
{
  Sequence s;

  int a[]={0,1,2,3,4,5};
  std::size_t sa=sizeof(a)/sizeof(a[0]);

  s.assign(&a[0],&a[sa]);
  BOOST_TEST(s.size()==sa&&std::equal(s.begin(),s.end(),&a[0]));

  s.assign((const int*)(&a[0]),(const int*)(&a[sa]));
  BOOST_TEST(s.size()==sa&&std::equal(s.begin(),s.end(),&a[0]));

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  s.assign({0,1,2,3,4,5});
#else
  s.assign(&a[0],&a[sa]);
#endif

  BOOST_TEST(s.size()==sa&&std::equal(s.begin(),s.end(),&a[0]));

  s.assign((std::size_t)18,37);
  BOOST_TEST(s.size()==18&&std::accumulate(s.begin(),s.end(),0)==666);

  s.assign((std::size_t)12,167);
  BOOST_TEST(s.size()==12&&std::accumulate(s.begin(),s.end(),0)==2004);
}

#if BOOST_WORKAROUND(__MWERKS__,<=0x3003)
#pragma parse_func_templ reset
#endif

template<typename Sequence>
static void test_integral_assign(BOOST_EXPLICIT_TEMPLATE_TYPE(Sequence))
{
  /* Special cases described in 23.1.1/9: integral types must not
   * be taken as iterators in assign(f,l) and insert(p,f,l).
   */

  Sequence s;

  s.assign(5,10);
  BOOST_TEST(s.size()==5&&std::accumulate(s.begin(),s.end(),0)==50);
  s.assign(2u,5u);
  BOOST_TEST(s.size()==2&&std::accumulate(s.begin(),s.end(),0)==10);

  s.clear();
  s.insert(s.begin(),5,10);
  BOOST_TEST(s.size()==5&&std::accumulate(s.begin(),s.end(),0)==50);
  s.insert(s.begin(),2u,5u);
  BOOST_TEST(s.size()==7&&std::accumulate(s.begin(),s.end(),0)==60);
}

employee_set produce_employee_set()
{
  employee_set es;
  es.insert(employee(0,"Petr",60,2837));
  es.insert(employee(1,"Jiri",25,2143));
  es.insert(employee(2,"Radka",40,4875));
  es.insert(employee(3,"Milan",38,3474));
  return es;
}

void test_copy_assignment()
{
  employee_set es;
  employee_set es2(es);

  employee_set::allocator_type al=es.get_allocator();
  al=get<1>(es).get_allocator();
  al=get<2>(es).get_allocator();
  al=get<3>(es).get_allocator();
  al=get<4>(es).get_allocator();
  al=get<5>(es).get_allocator();

  BOOST_TEST(es2.empty());

  es2.insert(employee(0,"Joe",31,1123));
  es2.insert(employee(1,"Robert",27,5601));
  es2.insert(employee(2,"John",40,7889));
  es2.insert(employee(2,"Aristotle",2388,3357)); /* clash */
  es2.insert(employee(3,"Albert",20,9012));
  es2.insert(employee(4,"John",57,1002));
  es2.insert(employee(0,"Andrew",60,2302));      /* clash */

  employee_set es3(es2);

  BOOST_TEST(es2==es3);
  BOOST_TEST(get<2>(es2)==get<2>(es3));
  BOOST_TEST(get<3>(es2)==get<3>(es3));
  BOOST_TEST(get<5>(es2)==get<5>(es3));

  employee_set es4=employee_set(non_std_allocator<employee>());
  employee_set_by_name& i1=get<name>(es4);
  i1=get<1>(es2);

  BOOST_TEST(es4==es2);

  employee_set es5=employee_set(employee_set::ctor_args_list());
  employee_set_by_age& i2=get<age>(es5);
  i2=get<2>(es2);

  BOOST_TEST(i2==get<2>(es2));

  employee_set es6;
  employee_set_as_inserted& i3=get<as_inserted>(es6);
  i3=get<3>(es2);

  BOOST_TEST(i3==get<3>(es2));

  employee_set es7;
  employee_set_randomly& i5=get<randomly>(es7);
  i5=get<5>(es2);

  BOOST_TEST(i5==get<5>(es2));

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  employee_set es8({{0,"Rose",40,4512},{1,"Mary",38,3345},{2,"Jo",25,7102}});
  employee_set es9;
  es9={{0,"Rose",40,4512},{1,"Mary",38,3345},{2,"Jo",25,7102},
       {0,"Rose",40,4512}};

  BOOST_TEST(es8.size()==3);
  BOOST_TEST(es9==es8);

  es9.clear();
  get<0>(es9)={{0,"Rose",40,4512},{1,"Mary",38,3345},{2,"Jo",25,7102},
               {0,"Rose",40,4512}};
  BOOST_TEST(es9==es8);

  es9.clear();
  get<0>(es9)={{0,"Rose",40,4512},{2,"Jo",25,7102},{1,"Mary",38,3345}};
  BOOST_TEST(es9==es8);

  es9.clear();
  get<1>(es9)={{1,"Mary",38,3345},{0,"Rose",40,4512},{2,"Jo",25,7102},
               {0,"Rose",40,4512}};
  BOOST_TEST(es9==es8);

  es9.clear();
  get<2>(es9)={{2,"Jo",25,7102},{0,"Rose",40,4512},{1,"Mary",38,3345}};
  BOOST_TEST(es9==es8);

  es9.clear();
  get<3>(es9)={{0,"Rose",40,4512},{1,"Mary",38,3345},{1,"Mary",38,3345},
               {2,"Jo",25,7102}};
  BOOST_TEST(es9==es8);

  es9.clear();
  get<4>(es9)={{1,"Mary",38,3345},{2,"Jo",25,7102},{0,"Rose",40,4512}};
  BOOST_TEST(es9==es8);

  es9.clear();
  get<5>(es9)={{1,"Mary",38,3345},{2,"Jo",25,7102},{0,"Rose",40,4512},
               {2,"Jo",25,7102}};
  BOOST_TEST(es9==es8);
#endif

  employee_set es10(produce_employee_set()),es11(produce_employee_set());
  BOOST_TEST(es10==es11);

  employee_set es12(boost::move(es10));
  BOOST_TEST(es10.empty());
  BOOST_TEST(es11==es12);

  es10=boost::move(es12);
  BOOST_TEST(es12.empty());
  BOOST_TEST(es11==es10);

  std::list<employee> l;
  l.push_back(employee(3,"Anna",31,5388));
  l.push_back(employee(1,"Rachel",27,9012));
  l.push_back(employee(2,"Agatha",40,1520));

#if BOOST_WORKAROUND(BOOST_MSVC,<1300)
  employee_set es13;
  es13.insert(l.begin(),l.end());
#else
  employee_set es13(l.begin(),l.end());
#endif

  l.sort();

  BOOST_TEST(es13.size()==l.size()&&
              std::equal(es13.begin(),es13.end(),l.begin()));

  /* MSVC++ 6.0 chokes on test_assign without this explicit instantiation */
  multi_index_container<int,indexed_by<sequenced<> > > s1;
  test_assign<multi_index_container<int,indexed_by<sequenced<> > > >();
  test_integral_assign<
    multi_index_container<int,indexed_by<sequenced<> > > >();

  multi_index_container<int,indexed_by<random_access<> > > s2;
  test_assign<multi_index_container<int,indexed_by<random_access<> > > >();
  test_integral_assign<
    multi_index_container<int,indexed_by<random_access<> > > >();

  /* Testcase for problem described at  http://www.boost.org/doc/html/move/
   * emulation_limitations.html#move.emulation_limitations.assignment_operator
   */

  holder h((holder()));
  h=holder();
}
