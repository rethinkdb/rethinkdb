/* Boost.Flyweight basic test template.
 *
 * Copyright 2006-2009 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_TEST_BASIC_TEMPLATE_HPP
#define BOOST_FLYWEIGHT_TEST_BASIC_TEMPLATE_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/lightweight_test.hpp>
#include <boost/flyweight/key_value.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/utility/value_init.hpp>
#include <string>
#include <sstream>
#include "heavy_objects.hpp"

#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

template<typename Flyweight,typename ForwardIterator>
void test_basic_template(
  ForwardIterator first,ForwardIterator last
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Flyweight))
{
  typedef typename Flyweight::value_type value_type;

  ForwardIterator it;

  for(it=first;it!=last;++it){
    /* construct/copy/destroy */

    Flyweight                            f1(*it);
    Flyweight                            f2;
    Flyweight                            c1(f1);
    Flyweight                            c2(static_cast<const Flyweight&>(f2));
    value_type                           v1(*it);
    boost::value_initialized<value_type> v2;
    BOOST_TEST(f1.get_key()==*it);
    BOOST_TEST((f1==f2)==(f1.get()==v2.data()));
    BOOST_TEST(f1==c1);
    BOOST_TEST(f2==c2);

    f1=f1;
    BOOST_TEST(f1==f1);

    c1=f2;
    BOOST_TEST(c1==f2);

    c1=f1;
    BOOST_TEST(c1==f1);

    /* convertibility to underlying type */

    BOOST_TEST(f1.get()==v1);

    /* identity of reference */

    BOOST_TEST(&f1.get()==&c1.get());

    /* modifiers */

    f1.swap(f1);
    BOOST_TEST(f1==c1);

    f1.swap(f2);
    BOOST_TEST(f1==c2);
    BOOST_TEST(f2==c1);

    boost::flyweights::swap(f1,f2);
    BOOST_TEST(f1==c1);
    BOOST_TEST(f2==c2);

    /* specialized algorithms */

    std::ostringstream oss1;
    oss1<<f1;
    std::ostringstream oss2;
    oss2<<f1.get();
    BOOST_TEST(oss1.str()==oss2.str());
  }
}

template<typename Flyweight,typename ForwardIterator>
void test_basic_with_assign_template(
  ForwardIterator first,ForwardIterator last
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Flyweight))
{
  typedef typename Flyweight::value_type value_type;

  ForwardIterator it;

  test_basic_template<Flyweight>(first,last);

  for(it=first;it!=last;++it){
    /* value construction */

    value_type v(*it);
    Flyweight  f1(v);
    Flyweight  f2(f1.get());
    BOOST_TEST(f1.get()==v);
    BOOST_TEST(f2.get()==v);
    BOOST_TEST(f1==f2);

    /* value assignment */

    Flyweight f3,f4;
    f3=v;
    f4=f1.get();
    BOOST_TEST(f2.get()==v);
    BOOST_TEST(f3.get()==v);
    BOOST_TEST(f2==f3);

    /* specialized algorithms */

    std::ostringstream oss1;
    oss1<<f1;
    std::istringstream iss1(oss1.str());
    Flyweight f5;
    iss1>>f5;
    BOOST_TEST(f5==f1);
  }
}

template<
  typename Flyweight1,typename Flyweight2,
  typename ForwardIterator1,typename ForwardIterator2
>
void test_basic_comparison_template(
  ForwardIterator1 first1,ForwardIterator1 last1,
  ForwardIterator2 first2
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Flyweight1)
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Flyweight2))
{
  typedef typename Flyweight1::value_type value_type1;
  typedef typename Flyweight2::value_type value_type2;

  for(;first1!=last1;++first1,++first2){
    value_type1 v1=value_type1(*first1);
    value_type2 v2=value_type2(*first2);
    Flyweight1  f1(v1);
    Flyweight2  f2(v2);

    BOOST_TEST((f1==f2)==(f1.get()==v2));
    BOOST_TEST((f1< f2)==(f1.get()< v2));
    BOOST_TEST((f1!=f2)==(f1.get()!=v2));
    BOOST_TEST((f1> f2)==(f1.get()> v2));
    BOOST_TEST((f1>=f2)==(f1.get()>=v2));
    BOOST_TEST((f1<=f2)==(f1.get()<=v2));

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
    BOOST_TEST((f1==v2)==(f1.get()==v2));
    BOOST_TEST((f1< v2)==(f1.get()< v2));
    BOOST_TEST((f1!=v2)==(f1.get()!=v2));
    BOOST_TEST((f1> v2)==(f1.get()> v2));
    BOOST_TEST((f1>=v2)==(f1.get()>=v2));
    BOOST_TEST((f1<=v2)==(f1.get()<=v2));

    BOOST_TEST((v1==f2)==(f1.get()==v2));
    BOOST_TEST((v1< f2)==(f1.get()< v2));
    BOOST_TEST((v1!=f2)==(f1.get()!=v2));
    BOOST_TEST((v1> f2)==(f1.get()> v2));
    BOOST_TEST((v1>=f2)==(f1.get()>=v2));
    BOOST_TEST((v1<=f2)==(f1.get()<=v2));
#endif

  }
}

template<typename FlyweightSpecifier>
void test_basic_template(BOOST_EXPLICIT_TEMPLATE_TYPE(FlyweightSpecifier))
{
  typedef typename boost::mpl::apply1<
    FlyweightSpecifier,int
  >::type int_flyweight;

  typedef typename boost::mpl::apply1<
    FlyweightSpecifier,std::string
  >::type string_flyweight;

  typedef typename boost::mpl::apply1<
    FlyweightSpecifier,char
  >::type char_flyweight;

  typedef typename boost::mpl::apply1<
    FlyweightSpecifier,
    boost::flyweights::key_value<std::string,texture,from_texture_to_string>
  >::type texture_flyweight;

  typedef typename boost::mpl::apply1<
    FlyweightSpecifier,
    boost::flyweights::key_value<int,factorization>
  >::type factorization_flyweight;

  int ints[]={0,1,1,0,1,2,3,4,3,4,0,0};
  test_basic_with_assign_template<int_flyweight>(
    &ints[0],&ints[0]+LENGTHOF(ints));

  const char* words[]={"hello","boost","flyweight","boost","bye","c++","c++"};
  test_basic_with_assign_template<string_flyweight>(
    &words[0],&words[0]+LENGTHOF(words));

  const char* textures[]={"wood","grass","sand","granite","terracotta"};
  test_basic_with_assign_template<texture_flyweight>(
    &textures[0],&textures[0]+LENGTHOF(textures));

  int factorizations[]={1098,102387,90846,2223978};
  test_basic_template<factorization_flyweight>(
    &factorizations[0],&factorizations[0]+LENGTHOF(factorizations));

  char chars[]={0,2,4,5,1,1,1,3,4,1,1,0};
  test_basic_comparison_template<int_flyweight,char_flyweight>(
    &ints[0],&ints[0]+LENGTHOF(ints),&chars[0]);

  test_basic_comparison_template<string_flyweight,string_flyweight>(
    &words[0],&words[0]+LENGTHOF(words)-1,&words[1]);

  test_basic_comparison_template<texture_flyweight,texture_flyweight>(
    &textures[0],&textures[0]+LENGTHOF(textures)-1,&textures[1]);

#if !defined(BOOST_NO_EXCEPTIONS)
  typedef typename boost::mpl::apply1<
    FlyweightSpecifier,
    boost::flyweights::key_value<int,throwing_value,from_throwing_value_to_int>
  >::type throwing_flyweight;

  try{
    throwing_flyweight fw(0);
  }catch(const throwing_value_exception&){}
  try{
    throwing_flyweight fw=throwing_flyweight(throwing_value());
  }catch(const throwing_value_exception&){}
#endif

}

#undef LENGTHOF

#endif
