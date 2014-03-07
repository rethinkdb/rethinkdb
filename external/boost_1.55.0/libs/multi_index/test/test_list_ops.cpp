/* Boost.MultiIndex test for standard list operations.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_list_ops.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <vector>
#include <boost/detail/lightweight_test.hpp>
#include "pre_multi_index.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/preprocessor/seq/enum.hpp>

using namespace boost::multi_index;

#undef CHECK_EQUAL
#define CHECK_EQUAL(p,check_seq) \
{\
  int v[]={BOOST_PP_SEQ_ENUM(check_seq)};\
  std::size_t size_v=sizeof(v)/sizeof(int);\
  BOOST_TEST(std::size_t(std::distance((p).begin(),(p).end()))==size_v);\
  BOOST_TEST(std::equal((p).begin(),(p).end(),&v[0]));\
}

#undef CHECK_VOID_RANGE
#define CHECK_VOID_RANGE(p) BOOST_TEST((p).first==(p).second)

struct is_even
{
  bool operator()(int x)const{return x%2==0;}
};

template <int m>
struct same_integral_div
{
  bool operator()(int x,int y)const{return (x/m)==(y/m);}
};

template <typename Container,typename Compare>
bool is_sorted(
  const Container& c,const Compare& comp=Compare())
{
  if(c.empty())return true;

  typedef typename Container::const_iterator const_iterator;
  for(const_iterator it(c.begin());;){
    const_iterator it2=it;
    ++it2;
    if(it2==c.end())return true;
    if(comp(*it2,*it))return false;
    it=it2;
  }
}

#if BOOST_WORKAROUND(__MWERKS__,<=0x3003)
/* The "ISO C++ Template Parser" option makes CW8.3 incorrectly fail at
 * expressions of the form sizeof(x) where x is an array local to a
 * template function.
 */

#pragma parse_func_templ off
#endif

template<typename Sequence>
static void test_list_ops_unique_seq(BOOST_EXPLICIT_TEMPLATE_TYPE(Sequence))
{
  typedef typename nth_index<Sequence,1>::type sequenced_index;

  Sequence         ss,ss2;
  sequenced_index &si=get<1>(ss),&si2=get<1>(ss2);

  si.push_front(0);                       /* 0        */
  si.push_front(4);                       /* 40       */
  ss.insert(2);                           /* 402      */
  ss.insert(5);                           /* 4025     */
  si.push_front(3);                       /* 34025    */
  si.push_back(6);                        /* 340256   */
  si.push_back(1);                        /* 3402561  */
  si.insert(project<1>(ss,ss.find(2)),8); /* 34082561 */
  si2=si;

  CHECK_EQUAL(si,(3)(4)(0)(8)(2)(5)(6)(1));

  si.remove(8);
  CHECK_EQUAL(si,(3)(4)(0)(2)(5)(6)(1));

  si.remove_if(is_even());

  CHECK_EQUAL(si,(3)(5)(1));

  si.splice(si.end(),si2);
  CHECK_EQUAL(si,(3)(5)(1)(4)(0)(8)(2)(6));
  CHECK_EQUAL(si2,(3)(5)(1));

  si.splice(project<1>(ss,ss.find(4)),si,project<1>(ss,ss.find(8)));
  CHECK_EQUAL(si,(3)(5)(1)(8)(4)(0)(2)(6));
  si2.clear();
  si2.splice(si2.begin(),si,si.begin());

  si.splice(si.end(),si2,si2.begin());
  CHECK_EQUAL(si,(5)(1)(8)(4)(0)(2)(6)(3));
  BOOST_TEST(si2.empty());

  si2.splice(si2.end(),si,project<1>(ss,ss.find(0)),project<1>(ss,ss.find(6)));
  CHECK_EQUAL(si,(5)(1)(8)(4)(6)(3));
  CHECK_EQUAL(si2,(0)(2));

  si.splice(si.begin(),si,si.begin(),si.begin());
  CHECK_EQUAL(si,(5)(1)(8)(4)(6)(3));

  si.splice(project<1>(ss,ss.find(8)),si,project<1>(ss,ss.find(4)),si.end());
  CHECK_EQUAL(si,(5)(1)(4)(6)(3)(8));

  si.sort();
  si2.sort();
  BOOST_TEST(is_sorted(si,std::less<int>()));
  BOOST_TEST(is_sorted(si2,std::less<int>()));

  si.merge(si2);
  BOOST_TEST(is_sorted(si,std::less<int>()));
  BOOST_TEST(si2.empty());

  {
    Sequence         ss3(ss);
    sequenced_index &si3=get<1>(ss3);

    si3.sort(std::greater<int>());
    si.reverse();
    BOOST_TEST(si==si3);
  }

  si2.splice(si2.end(),si,project<1>(ss,ss.find(6)),project<1>(ss,ss.find(3)));
  CHECK_EQUAL(si2,(6)(5)(4));

  si.merge(si2,std::greater<int>());
  BOOST_TEST(is_sorted(si,std::greater<int>()));
  BOOST_TEST(si2.empty());

  /* testcase for bug reported at
   * https://svn.boost.org/trac/boost/ticket/3076
   */
  {
    Sequence         ss3;
    sequenced_index &si3=get<1>(ss3);
    si3.sort();
  }
}

template<typename Sequence>
static void test_list_ops_non_unique_seq(
  BOOST_EXPLICIT_TEMPLATE_TYPE(Sequence))
{
  typedef typename Sequence::iterator iterator;

  Sequence ss;
  for(int i=0;i<10;++i){
    ss.push_back(i);
    ss.push_back(i);
    ss.push_front(i);
    ss.push_front(i);
  } /* 9988776655443322110000112233445566778899 */

  ss.unique();
  CHECK_EQUAL(
    ss,
    (9)(8)(7)(6)(5)(4)(3)(2)(1)(0)
    (1)(2)(3)(4)(5)(6)(7)(8)(9));

  iterator it=ss.begin();
  for(int j=0;j<9;++j,++it){} /* it points to o */

  Sequence ss2;
  ss2.splice(ss2.end(),ss,ss.begin(),it);
  ss2.reverse();
  ss.merge(ss2);
  CHECK_EQUAL(
    ss,
    (0)(1)(1)(2)(2)(3)(3)(4)(4)(5)(5)
    (6)(6)(7)(7)(8)(8)(9)(9));

  ss.unique(same_integral_div<3>());
  CHECK_EQUAL(ss,(0)(3)(6)(9));

  ss.unique(same_integral_div<1>());
  CHECK_EQUAL(ss,(0)(3)(6)(9));

  /* testcases for bugs reported at
   * http://lists.boost.org/boost-users/2006/09/22604.php
   */
  {
    Sequence ss,ss2;
    ss.push_back(0);
    ss2.push_back(0);
    ss.splice(ss.end(),ss2,ss2.begin());
    CHECK_EQUAL(ss,(0)(0));
    BOOST_TEST(ss2.empty());

    ss.clear();
    ss2.clear();
    ss.push_back(0);
    ss2.push_back(0);
    ss.splice(ss.end(),ss2,ss2.begin(),ss2.end());
    CHECK_EQUAL(ss,(0)(0));
    BOOST_TEST(ss2.empty());

    ss.clear();
    ss2.clear();
    ss.push_back(0);
    ss2.push_back(0);
    ss.merge(ss2);
    CHECK_EQUAL(ss,(0)(0));
    BOOST_TEST(ss2.empty());

    typedef typename Sequence::value_type value_type;
    ss.clear();
    ss2.clear();
    ss.push_back(0);
    ss2.push_back(0);
    ss.merge(ss2,std::less<value_type>());
    CHECK_EQUAL(ss,(0)(0));
    BOOST_TEST(ss2.empty());
  }
}

#if BOOST_WORKAROUND(__MWERKS__,<=0x3003)
#pragma parse_func_templ reset
#endif

void test_list_ops()
{
  typedef multi_index_container<
    int,
    indexed_by<
      ordered_unique<identity<int> >,
      sequenced<>
    >
  > sequenced_set;
  
  /* MSVC++ 6.0 chokes on test_list_ops_unique_seq without this
   * explicit instantiation
   */
  sequenced_set ss;
  test_list_ops_unique_seq<sequenced_set>();


  typedef multi_index_container<
    int,
    indexed_by<
      ordered_unique<identity<int> >,
      random_access<>
    >
  > random_access_set;
  
  random_access_set rs;
  test_list_ops_unique_seq<random_access_set>();

  typedef multi_index_container<
    int,
    indexed_by<sequenced<> >
  > int_list;

  int_list il;
  test_list_ops_non_unique_seq<int_list>();

  typedef multi_index_container<
    int,
    indexed_by<random_access<> >
  > int_vector;

  int_vector iv;
  test_list_ops_non_unique_seq<int_vector>();
}
