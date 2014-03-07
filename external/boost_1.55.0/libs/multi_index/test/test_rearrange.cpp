/* Boost.MultiIndex test for rearrange operations.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_rearrange.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <iterator>
#include <boost/detail/lightweight_test.hpp>
#include "pre_multi_index.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/next_prior.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/ref.hpp>
#include <vector>

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

#if BOOST_WORKAROUND(__MWERKS__,<=0x3003)
/* The "ISO C++ Template Parser" option makes CW8.3 incorrectly fail at
 * expressions of the form sizeof(x) where x is an array local to a
 * template function.
 */

#pragma parse_func_templ off
#endif

template<typename Sequence>
static void local_test_rearrange(BOOST_EXPLICIT_TEMPLATE_TYPE(Sequence))
{
  typedef typename Sequence::iterator   iterator;
  typedef typename Sequence::value_type value_type;

  Sequence sc;
  sc.push_back(0);
  sc.push_back(1);
  sc.push_back(2);
  sc.push_back(3);
  sc.push_back(4);
  sc.push_back(5);

  iterator it;

  it=sc.begin();
  std::advance(it,3);
  sc.relocate(sc.begin(),it);
  CHECK_EQUAL(sc,(3)(0)(1)(2)(4)(5));
  BOOST_TEST(it==sc.begin());

  sc.relocate(it,it);
  CHECK_EQUAL(sc,(3)(0)(1)(2)(4)(5));

  std::advance(it,3);
  sc.relocate(sc.end(),it,sc.end());
  CHECK_EQUAL(sc,(3)(0)(1)(2)(4)(5));

  sc.relocate(sc.begin(),it,it);
  CHECK_EQUAL(sc,(3)(0)(1)(2)(4)(5));

  iterator it2;

  it2=sc.begin();
  ++it2;
  sc.relocate(it2,it,sc.end());
  CHECK_EQUAL(sc,(3)(2)(4)(5)(0)(1));
  BOOST_TEST(std::distance(it,it2)==3);

  sc.relocate(boost::prior(sc.end()),it,it2);
  CHECK_EQUAL(sc,(3)(0)(2)(4)(5)(1));

  std::vector<boost::reference_wrapper<const value_type> > v;
  for(iterator it3=sc.begin();it3!=sc.end();++it3){
    v.push_back(boost::cref(*it3));
  }

  sc.rearrange(v.begin());
  BOOST_TEST(std::equal(sc.begin(),sc.end(),v.begin()));

  std::reverse(v.begin(),v.end());
  sc.rearrange(v.begin());
  BOOST_TEST(std::equal(sc.begin(),sc.end(),v.begin()));

  std::sort(v.begin(),v.end());
  sc.rearrange(v.begin());
  BOOST_TEST(std::equal(sc.begin(),sc.end(),v.begin()));

  std::reverse(v.begin(),v.begin()+v.size()/2);
  sc.rearrange(v.begin());
  BOOST_TEST(std::equal(sc.begin(),sc.end(),v.begin()));
}

#if BOOST_WORKAROUND(__MWERKS__,<=0x3003)
#pragma parse_func_templ reset
#endif

void test_rearrange()
{
  typedef multi_index_container<
    int,
    indexed_by<sequenced<> >
  > int_list;

  /* MSVC++ 6.0 chokes on local_test_rearrange without this
   * explicit instantiation
   */
  int_list il;
  local_test_rearrange<int_list>();

  typedef multi_index_container<
    int,
    indexed_by<random_access<> >
  > int_vector;

  int_vector iv;
  local_test_rearrange<int_vector>();
}
