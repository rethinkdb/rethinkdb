/* Boost.MultiIndex serialization tests template.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/mpl/size.hpp>
#include "pre_multi_index.hpp"
#include <boost/multi_index_container.hpp>
#include <string>
#include <sstream>
#include <vector>

template<int N>
struct all_indices_equal_helper
{
  template<class MultiIndexContainer>
  static bool compare(
    const MultiIndexContainer& m1,const MultiIndexContainer& m2)
  {
    if(!(boost::multi_index::get<N>(m1)==boost::multi_index::get<N>(m2))){
      return false;
    }
    return all_indices_equal_helper<N-1>::compare(m1,m2);
  }
};

template<>
struct all_indices_equal_helper<0>
{
  template<class MultiIndexContainer>
  static bool compare(
    const MultiIndexContainer&,const MultiIndexContainer&)
  {
    return true;
  }
};

template<class MultiIndexContainer>
bool all_indices_equal(
  const MultiIndexContainer& m1,const MultiIndexContainer& m2)
{
  BOOST_STATIC_CONSTANT(int,
    N=boost::mpl::size<
    BOOST_DEDUCED_TYPENAME MultiIndexContainer::index_type_list>::type::value);

  return all_indices_equal_helper<N-1>::compare(m1,m2);
}

template<class MultiIndexContainer>
void test_serialization(const MultiIndexContainer& m)
{
  typedef typename MultiIndexContainer::iterator       iterator;
  typedef typename MultiIndexContainer::const_iterator const_iterator;

  std::ostringstream oss;
  {
    boost::archive::text_oarchive oa(oss);
    oa<<m;

    std::vector<const_iterator> its(m.size());
    const_iterator              it_end=m.end();
    for(const_iterator it=m.begin();it!=it_end;++it){
      its.push_back(it);
      oa<<const_cast<const const_iterator&>(its.back());
    }
    oa<<const_cast<const const_iterator&>(it_end);
  }

  MultiIndexContainer m2;
  std::istringstream iss(oss.str());
  boost::archive::text_iarchive ia(iss);
  ia>>m2;
  BOOST_TEST(all_indices_equal(m,m2));

  iterator it_end=m2.end();
  for(iterator it=m2.begin();it!=it_end;++it){
    iterator it2;
    ia>>it2;
    BOOST_TEST(it==it2);

    /* exercise safe mode with this (unchecked) iterator */
    BOOST_TEST(*it==*it2);
    m2.erase(it,it2);
    m2.erase(it2,it2);
    m2.erase(it2,it);
    iterator it3(++it2);
    iterator it4;
    it4=--it2;
    BOOST_TEST(it==it4);
    BOOST_TEST(it==boost::multi_index::project<0>(m2,it4));
  }
  iterator it2;
  ia>>it2;
  BOOST_TEST(it_end==it2);
  BOOST_TEST(it_end==boost::multi_index::project<0>(m2,it2));
}
