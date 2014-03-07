/* Boost.MultiIndex test for replace(), modify() and modify_key().
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_update.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <cstddef>
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include "pair_of_ints.hpp"
#include <boost/detail/lightweight_test.hpp>
#include <boost/next_prior.hpp>

struct do_nothing
{
  template<typename T>
  void operator()(const T&)const{}
};

struct null_hash
{
  template<typename T>
  std::size_t operator()(const T&)const{return 0;}
};

template<class MultiIndexContainer>
void test_stable_update(BOOST_EXPLICIT_TEMPLATE_TYPE(MultiIndexContainer))
{
  typedef typename MultiIndexContainer::iterator  iterator;
  typedef typename MultiIndexContainer::size_type size_type;

  MultiIndexContainer c;
  c.insert(0);c.insert(0);c.insert(0);c.insert(0);
  c.insert(1);c.insert(1);c.insert(1);
  c.insert(2);c.insert(2);
  c.insert(3);

  for(size_type n=c.size();n--;){
    iterator it=boost::next(c.begin(),n);

    c.replace(it,*it);
    BOOST_TEST((size_type)std::distance(c.begin(),it)==n);

    c.modify(it,do_nothing());
    BOOST_TEST((size_type)std::distance(c.begin(),it)==n);

    c.modify(it,do_nothing(),do_nothing());
    BOOST_TEST((size_type)std::distance(c.begin(),it)==n);
  }
}

using namespace boost::multi_index;

void test_update()
{
  employee_set              es;
  employee_set_as_inserted& i=get<as_inserted>(es);
  employee_set_randomly&    r=get<randomly>(es);

  es.insert(employee(0,"Joe",31,1123));
  es.insert(employee(1,"Robert",27,5601));
  es.insert(employee(2,"John",40,7889));
  es.insert(employee(3,"Olbert",20,9012));
  es.insert(employee(4,"John",57,1002));

  employee_set::iterator             it=es.find(employee(0,"Joe",31,1123));
  employee_set_as_inserted::iterator it1=
    project<as_inserted>(es,get<name>(es).find("Olbert"));
  employee_set_randomly::iterator    it2=
    project<randomly>(es,get<age>(es).find(57));

  BOOST_TEST(es.replace(it,*it));
  BOOST_TEST(i.replace(it1,*it1));
  BOOST_TEST(r.replace(it2,*it2));
  BOOST_TEST(!es.replace(it,employee(3,"Joe",31,1123))&&it->id==0);
  BOOST_TEST(es.replace(it,employee(0,"Joe",32,1123))&&it->age==32);
  BOOST_TEST(i.replace(it1,employee(3,"Albert",20,9012))&&it1->name==
                "Albert");
  BOOST_TEST(!r.replace(it2,employee(4,"John",57,5601)));

  {
    typedef multi_index_container<
      pair_of_ints,
      indexed_by<
        ordered_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,first)>,
        hashed_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,second)>,
        sequenced<> > >
    int_int_set;

    int_int_set iis;
    nth_index<int_int_set,1>::type& ii1=get<1>(iis);
    nth_index<int_int_set,2>::type& ii2=get<2>(iis);
    iis.insert(pair_of_ints(0,0));
    iis.insert(pair_of_ints(5,5));
    iis.insert(pair_of_ints(10,10));

    BOOST_TEST(!iis.replace(iis.begin(),pair_of_ints(5,0)));
    BOOST_TEST(!ii2.replace(ii2.begin(),pair_of_ints(0,5)));
    BOOST_TEST(!ii1.replace(project<1>(iis,iis.begin()),pair_of_ints(5,11)));
    BOOST_TEST(!iis.replace(iis.begin(),pair_of_ints(11,5)));
    BOOST_TEST(!iis.replace(boost::next(iis.begin()),pair_of_ints(10,5)));
    BOOST_TEST(!ii1.replace(
      project<1>(iis,boost::next(iis.begin())),pair_of_ints(5,10)));
    BOOST_TEST(!iis.replace(boost::prior(iis.end()),pair_of_ints(5,10)));
    BOOST_TEST(!ii2.replace(boost::prior(ii2.end()),pair_of_ints(10,5)));

    BOOST_TEST(iis.modify(iis.begin(),increment_first));
    BOOST_TEST(ii2.modify(ii2.begin(),increment_first));
    BOOST_TEST(ii1.modify(project<1>(iis,iis.begin()),increment_first));
    BOOST_TEST(ii2.modify(ii2.begin(),increment_first,decrement_first));

    BOOST_TEST(!iis.modify(iis.begin(),increment_first,decrement_first));
    BOOST_TEST(iis.size()==3);

    BOOST_TEST(!iis.modify(iis.begin(),increment_first));
    BOOST_TEST(iis.size()==2);

    iis.insert(pair_of_ints(0,0));
    BOOST_TEST(ii2.modify(boost::prior(ii2.end()),increment_second));
    BOOST_TEST(iis.modify(iis.begin(),increment_second));
    BOOST_TEST(ii2.modify(boost::prior(ii2.end()),increment_second));
    BOOST_TEST(iis.modify(iis.begin(),increment_second,decrement_second));

    BOOST_TEST(!ii2.modify(
      boost::prior(ii2.end()),increment_second,decrement_second));
    BOOST_TEST(ii2.size()==3);

    BOOST_TEST(!ii2.modify(boost::prior(ii2.end()),increment_second));
    BOOST_TEST(ii2.size()==2);

    iis.insert(pair_of_ints(0,0));
    BOOST_TEST(iis.modify_key(iis.begin(),increment_int));
    BOOST_TEST(iis.modify_key(iis.begin(),increment_int,decrement_int));
    BOOST_TEST(iis.modify_key(iis.begin(),increment_int));
    BOOST_TEST(iis.modify_key(iis.begin(),increment_int));

    BOOST_TEST(!iis.modify_key(iis.begin(),increment_int,decrement_int));
    BOOST_TEST(iis.size()==3);

    BOOST_TEST(!iis.modify_key(iis.begin(),increment_int));
    BOOST_TEST(iis.size()==2);

    nth_index_iterator<int_int_set,1>::type it=ii1.find(5);
    BOOST_TEST(ii1.modify_key(it,increment_int));
    BOOST_TEST(ii1.modify_key(it,increment_int));
    BOOST_TEST(ii1.modify_key(it,increment_int,decrement_int));
    BOOST_TEST(ii1.modify_key(it,increment_int));

    BOOST_TEST(!ii1.modify_key(it,increment_int,decrement_int));
    BOOST_TEST(ii1.size()==2);

    BOOST_TEST(!ii1.modify_key(it,increment_int));
    BOOST_TEST(ii1.size()==1);
  }
  {
    typedef multi_index_container<
      pair_of_ints,
      indexed_by<
        hashed_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,first)>,
        random_access<>,
        ordered_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,second)> > >
    int_int_set;

    int_int_set iis;
    nth_index<int_int_set,1>::type& ii1=get<1>(iis);
    int_int_set::iterator p1=iis.insert(pair_of_ints(0,0)).first;
    int_int_set::iterator p2=iis.insert(pair_of_ints(5,5)).first;
    int_int_set::iterator p3=iis.insert(pair_of_ints(10,10)).first;

    BOOST_TEST(!iis.replace(p1,pair_of_ints(5,0)));
    BOOST_TEST(!ii1.replace(ii1.begin(),pair_of_ints(0,5)));
    BOOST_TEST(!iis.replace(p1,pair_of_ints(5,11)));
    BOOST_TEST(!iis.replace(p1,pair_of_ints(11,5)));
    BOOST_TEST(!iis.replace(p2,pair_of_ints(10,5)));
    BOOST_TEST(!iis.replace(p2,pair_of_ints(5,10)));
    BOOST_TEST(!iis.replace(p3,pair_of_ints(5,10)));
    BOOST_TEST(!ii1.replace(boost::prior(ii1.end()),pair_of_ints(10,5)));

    BOOST_TEST(iis.modify(p1,increment_first));
    BOOST_TEST(ii1.modify(ii1.begin(),increment_first));
    BOOST_TEST(iis.modify(p1,increment_first));
    BOOST_TEST(ii1.modify(ii1.begin(),increment_first,decrement_first));

    BOOST_TEST(!iis.modify(p1,increment_first,decrement_first));
    BOOST_TEST(iis.size()==3);

    BOOST_TEST(!iis.modify(p1,increment_first));
    BOOST_TEST(iis.size()==2);

    p1=iis.insert(pair_of_ints(0,0)).first;
    BOOST_TEST(ii1.modify(boost::prior(ii1.end()),increment_second));
    BOOST_TEST(iis.modify(p1,increment_second,decrement_second));
    BOOST_TEST(ii1.modify(boost::prior(ii1.end()),increment_second));
    BOOST_TEST(iis.modify(p1,increment_second));

    BOOST_TEST(!ii1.modify(
      boost::prior(ii1.end()),increment_second,decrement_second));
    BOOST_TEST(ii1.size()==3);

    BOOST_TEST(!ii1.modify(boost::prior(ii1.end()),increment_second));
    BOOST_TEST(ii1.size()==2);
  }
  {
    typedef multi_index_container<
      int,
      indexed_by<
        ordered_non_unique<identity<int> >
      >
    > int_set;

    /* MSVC++ 6.0 needs this out-of-template definition */ 
    int_set dummy1;
    test_stable_update<int_set>();

    typedef multi_index_container<
      int,
      indexed_by<
        hashed_unique<identity<int> >
      >
    > int_hashed_set;

    int_hashed_set dummy2;
    test_stable_update<int_hashed_set>();

    typedef multi_index_container<
      int,
      indexed_by<
        hashed_unique<identity<int> >
      >
    > int_hashed_multiset;

    int_hashed_multiset dummy3;
    test_stable_update<int_hashed_multiset>();

    typedef multi_index_container<
      int,
      indexed_by<
        hashed_unique<identity<int>,null_hash>
      >
    > degenerate_int_hashed_set;

    degenerate_int_hashed_set dummy4;
    test_stable_update<degenerate_int_hashed_set>();

    typedef multi_index_container<
      int,
      indexed_by<
        hashed_non_unique<identity<int>,null_hash>
      >
    > degenerate_int_hashed_multiset;

    degenerate_int_hashed_multiset dummy5;
    test_stable_update<degenerate_int_hashed_multiset>();
  }
}
