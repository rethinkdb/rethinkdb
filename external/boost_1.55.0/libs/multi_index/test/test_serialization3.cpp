/* Boost.MultiIndex test for serialization, part 3.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_serialization3.hpp"
#include "test_serialization_template.hpp"

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include "non_std_allocator.hpp"

struct non_default_ctble
{
  non_default_ctble(int n_):n(n_){}

  bool operator==(const non_default_ctble& x)const{return n==x.n;}

  int n;
};

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace boost{
namespace serialization{
#endif

template<class Archive>
void save_construct_data(
  Archive& ar,const non_default_ctble* p,const unsigned int version)
{
#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  if(version<3)return;
#endif

  ar<<boost::serialization::make_nvp("n",p->n);
}

template<class Archive>
void load_construct_data(
  Archive& ar,non_default_ctble* p,const unsigned int version)
{
#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  if(version<3)return;
#endif

  int n=0;
  ar>>boost::serialization::make_nvp("n",n);
  ::new(p)non_default_ctble(n);
}

template<class Archive>
void serialize(Archive&,non_default_ctble&,const unsigned int)
{
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace serialization */
} /* namespace boost*/
#endif

namespace boost{
namespace serialization{
template<> struct version<non_default_ctble>
{
  BOOST_STATIC_CONSTANT(int,value=3);
};
} /* namespace serialization */
} /* namespace boost*/

using namespace boost::multi_index;

void test_serialization3()
{
  const int N=100;
  const int SHUFFLE=10232;

  typedef multi_index_container<
    int,
    indexed_by<
      hashed_unique<identity<int> >,
      sequenced<>
    >,
    non_std_allocator<int>
  > hashed_set;

  typedef hashed_set::iterator       iterator;
  typedef hashed_set::local_iterator local_iterator;

  hashed_set hs;

  for(int i=0;i<N;++i){
    hs.insert(i*SHUFFLE);
  }

  std::ostringstream oss;
  {
    boost::archive::text_oarchive oa(oss);
    oa<<const_cast<const hashed_set&>(hs);

    std::vector<iterator> its(N);
    for(int i=0;i<N;++i){
      iterator it=hs.find(i*SHUFFLE);
      its.push_back(it);
      oa<<const_cast<const iterator&>(its.back());
    }
    iterator it=hs.end();
    oa<<const_cast<const iterator&>(it);

    std::vector<local_iterator> lits(2*N);
    for(std::size_t buc=0;buc<hs.bucket_count();++buc){
      for(local_iterator lit=hs.begin(buc),lit_end=hs.end(buc);
          lit!=lit_end;++lit){
        oa<<*lit;
        lits.push_back(lit);
        oa<<const_cast<const local_iterator&>(lits.back());
      }
      local_iterator lit2=hs.end(buc);
      lits.push_back(lit2);
      oa<<const_cast<const local_iterator&>(lits.back());
    }
  }

  hashed_set hs2;
  std::istringstream iss(oss.str());
  boost::archive::text_iarchive ia(iss);
  ia>>hs2;
  BOOST_TEST(boost::multi_index::get<1>(hs)==boost::multi_index::get<1>(hs2));

  for(int j=0;j<N;++j){
    iterator it;
    ia>>it;
    BOOST_TEST(*it==j*SHUFFLE);
  }
  iterator it;
  ia>>it;
  BOOST_TEST(it==hs2.end());

  for(std::size_t buc=0;buc<hs2.bucket_count();++buc){
    for(std::size_t k=0;k<hs2.bucket_size(buc);++k){
      int n;
      local_iterator it;
      ia>>n;
      ia>>it;
      BOOST_TEST(*it==n);
    }
    local_iterator it2;
    ia>>it2;
    BOOST_TEST(it2==hs2.end(buc));
  }

  {
    typedef multi_index_container<
      non_default_ctble,
      indexed_by<
        ordered_unique<
          BOOST_MULTI_INDEX_MEMBER(non_default_ctble,int,n)
        >
      >
    > multi_index_t;

    multi_index_t m;
    for(int i=0;i<100;++i)m.insert(non_default_ctble(i));
    test_serialization(m);
  }
}
