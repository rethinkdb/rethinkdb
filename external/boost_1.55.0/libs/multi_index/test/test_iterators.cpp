/* Boost.MultiIndex test for iterators.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_iterators.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include <boost/detail/lightweight_test.hpp>
#include <boost/next_prior.hpp>

using namespace boost::multi_index;

template<typename Index>
void test_non_const_iterators(Index& i,int target)
{
  typedef typename Index::iterator         iterator;
  typedef typename Index::reverse_iterator reverse_iterator;

  int n=0;
  for(iterator it=i.begin();it!=i.end();++it){
    BOOST_TEST(i.iterator_to(*it)==it);
    n+=it->id;
  }
  int m=0;
  for(reverse_iterator rit=i.rbegin();rit!=i.rend();++rit){
    m+=rit->id;
  }
  int p=0;
  for(iterator it2=i.end();it2!=i.begin();){
    --it2;
    p+=it2->id;
  }
  int q=0;
  for(reverse_iterator rit2=i.rend();rit2!=i.rbegin();){
    --rit2;
    q+=rit2->id;
  }

  BOOST_TEST(n==target&&n==m&&n==p&&n==q);
}

template<typename Index>
void test_const_iterators(const Index& i,int target)
{
  typedef typename Index::const_iterator         const_iterator;
  typedef typename Index::const_reverse_iterator const_reverse_iterator;

  BOOST_TEST(i.cbegin()==i.begin());
  BOOST_TEST(i.cend()==i.end());
  BOOST_TEST(i.crbegin()==i.rbegin());
  BOOST_TEST(i.crend()==i.rend());

  int n=0;
  for(const_iterator it=i.begin();it!=i.end();++it){
    BOOST_TEST(i.iterator_to(*it)==it);
    n+=it->id;
  }
  int m=0;
  for(const_reverse_iterator rit=i.rbegin();rit!=i.rend();++rit){
    m+=rit->id;
  }
  int p=0;
  for(const_iterator it2=i.end();it2!=i.begin();){
    --it2;
    p+=it2->id;
  }
  int q=0;
  for(const_reverse_iterator rit2=i.rend();rit2!=i.rbegin();){
    --rit2;
    q+=rit2->id;
  }

  BOOST_TEST(n==target&&n==m&&n==p&&n==q);
}

template<typename Index>
void test_non_const_hashed_iterators(Index& i,int target)
{
  typedef typename Index::iterator       iterator;
  typedef typename Index::local_iterator local_iterator;
  typedef typename Index::size_type      size_type;

  int n=0;
  for(iterator it=i.begin();it!=i.end();++it){
    BOOST_TEST(i.iterator_to(*it)==it);
    n+=it->id;
  }
  int m=0;
  for(size_type buc=0;buc<i.bucket_count();++buc){
    for(local_iterator it=i.begin(buc);it!=i.end(buc);++it){
      BOOST_TEST(i.local_iterator_to(*it)==it);
      m+=it->id;
    }
  }

  BOOST_TEST(n==target&&n==m);
}

template<typename Index>
void test_const_hashed_iterators(const Index& i,int target)
{
  typedef typename Index::const_iterator       const_iterator;
  typedef typename Index::const_local_iterator const_local_iterator;
  typedef typename Index::size_type            size_type;

  BOOST_TEST(i.cbegin()==i.begin());
  BOOST_TEST(i.cend()==i.end());

  int n=0;
  for(const_iterator it=i.begin();it!=i.end();++it){
    BOOST_TEST(i.iterator_to(*it)==it);
    n+=it->id;
  }
  int m=0;
  for(size_type buc=0;buc<i.bucket_count();++buc){
    BOOST_TEST(i.cbegin(buc)==i.begin(buc));
    BOOST_TEST(i.cend(buc)==i.end(buc));
    for(const_local_iterator it=i.begin(buc);it!=i.end(buc);++it){
      BOOST_TEST(i.local_iterator_to(*it)==it);
      m+=it->id;
    }
  }

  BOOST_TEST(n==target&&n==m);
}

template<typename Index>
void test_non_const_rnd_iterators(Index& i,int target)
{
  typedef typename Index::iterator         iterator;
  typedef typename Index::reverse_iterator reverse_iterator;
  typedef typename Index::difference_type  difference_type;

  iterator         middle=i.begin()+(i.end()-i.begin())/2;
  difference_type  off=middle-i.begin();
  reverse_iterator rmiddle=i.rbegin()+off;
  bool             odd=((i.end()-i.begin())%2)!=0;

  int n=0;
  for(iterator it=i.begin();it!=middle;++it){
    BOOST_TEST(i.iterator_to(*it)==it);
    n+=it->id;
    n+=it[off].id;
  }
  if(odd)n+=(boost::prior(i.end()))->id;
  int m=0;
  for(reverse_iterator rit=i.rbegin();rit!=rmiddle;++rit){
    m+=rit->id;
    m+=(rit+off)->id;
  }
  if(odd)m+=(boost::prior(i.rend()))->id;
  int p=0;
  for(iterator it2=i.end();it2!=middle;){
    --it2;
    p+=it2->id;
    p+=(it2-off)->id;
  }
  if(odd)p-=middle->id;
  int q=0;
  for(reverse_iterator rit2=i.rend();rit2!=rmiddle;){
    --rit2;
    q+=rit2->id;
    q+=(rit2-off)->id;
  }
  if(odd)q-=rmiddle->id;

  BOOST_TEST(n==target&&n==m&&n==p&&n==q);
}

template<typename Index>
void test_const_rnd_iterators(const Index& i,int target)
{
  typedef typename Index::const_iterator         const_iterator;
  typedef typename Index::const_reverse_iterator const_reverse_iterator;
  typedef typename Index::difference_type        difference_type;

  BOOST_TEST(i.cbegin()==i.begin());
  BOOST_TEST(i.cend()==i.end());
  BOOST_TEST(i.crbegin()==i.rbegin());
  BOOST_TEST(i.crend()==i.rend());

  const_iterator         middle=i.begin()+(i.end()-i.begin())/2;
  difference_type        off=middle-i.begin();
  const_reverse_iterator rmiddle=i.rbegin()+off;
  bool                   odd=((i.end()-i.begin())%2)!=0;

  int n=0;
  for(const_iterator it=i.begin();it!=middle;++it){
    BOOST_TEST(i.iterator_to(*it)==it);
    n+=it->id;
    n+=it[off].id;
  }
  if(odd)n+=(boost::prior(i.end()))->id;
  int m=0;
  for(const_reverse_iterator rit=i.rbegin();rit!=rmiddle;++rit){
    m+=rit->id;
    m+=(rit+off)->id;
  }
  if(odd)m+=(boost::prior(i.rend()))->id;
  int p=0;
  for(const_iterator it2=i.end();it2!=middle;){
    --it2;
    p+=it2->id;
    p+=(it2-off)->id;
  }
  if(odd)p-=middle->id;
  int q=0;
  for(const_reverse_iterator rit2=i.rend();rit2!=rmiddle;){
    --rit2;
    q+=rit2->id;
    q+=(rit2-off)->id;
  }
  if(odd)q-=rmiddle->id;

  BOOST_TEST(n==target&&n==m&&n==p&&n==q);
}

void test_iterators()
{
  employee_set es;

  es.insert(employee(0,"Joe",31,1123));
  es.insert(employee(1,"Robert",27,5601));
  es.insert(employee(2,"John",40,7889));
  es.insert(employee(3,"Albert",20,9012));
  es.insert(employee(4,"John",57,1002));

  int target=0+1+2+3+4;

  test_non_const_iterators       (es,target);
  test_const_iterators           (es,target);
  test_non_const_hashed_iterators(get<1>(es),target);
  test_const_hashed_iterators    (get<1>(es),target);
  test_non_const_iterators       (get<2>(es),target);
  test_const_iterators           (get<2>(es),target);
  test_non_const_iterators       (get<3>(es),target);
  test_const_iterators           (get<3>(es),target);
  test_non_const_hashed_iterators(get<4>(es),target);
  test_const_hashed_iterators    (get<4>(es),target);
  test_non_const_rnd_iterators   (get<5>(es),target);
  test_const_rnd_iterators       (get<5>(es),target);
}
