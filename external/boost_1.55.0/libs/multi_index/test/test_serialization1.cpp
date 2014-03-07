/* Boost.MultiIndex test for serialization, part 1.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_serialization1.hpp"
#include "test_serialization_template.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include "non_std_allocator.hpp"

using namespace boost::multi_index;

void test_serialization1()
{
  {
    typedef multi_index_container<
      int,
      indexed_by<
        sequenced<>,
        sequenced<>,
        random_access<>
      >
    > multi_index_t;

    multi_index_t m;
    for(int i=0;i<100;++i)m.push_back(i);
    m.reverse();
    test_serialization(m);

    m.clear();
    for(int j=50;j<100;++j)m.push_back(j);
    for(int k=0;k<50;++k)m.push_back(k);
    m.sort();
    test_serialization(m);
  }
  {
    typedef multi_index_container<
      int,
      indexed_by<
        random_access<>,
        sequenced<>,
        ordered_non_unique<identity<int> >
      >,
      non_std_allocator<int>
    > multi_index_t;

    multi_index_t m;
    for(int i=0;i<100;++i){
      m.push_back(i);
      m.push_back(i);
      m.push_back(i);
    }
    m.reverse();
    test_serialization(m);
  }
}
