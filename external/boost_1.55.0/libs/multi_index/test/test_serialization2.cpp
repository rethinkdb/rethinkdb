/* Boost.MultiIndex test for serialization, part 2.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_serialization2.hpp"
#include "test_serialization_template.hpp"

#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include "non_std_allocator.hpp"
#include "pair_of_ints.hpp"

using namespace boost::multi_index;

void test_serialization2()
{
  {
    typedef multi_index_container<
      pair_of_ints,
      indexed_by<
        ordered_unique<
          BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,first)
        >,
        ordered_non_unique<
          BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,second)
        >,
        random_access<>,
        sequenced<>
      >,
      non_std_allocator<pair_of_ints>
    > multi_index_t;

    multi_index_t m;
    test_serialization(m);

    m.insert(pair_of_ints(4,0));
    test_serialization(m);

    m.insert(pair_of_ints(3,1));
    m.insert(pair_of_ints(2,1));
    test_serialization(m);

    m.insert(pair_of_ints(1,1));
    test_serialization(m);

    m.insert(pair_of_ints(0,0));
    test_serialization(m);

    m.insert(pair_of_ints(5,1));
    m.insert(pair_of_ints(7,1));
    m.insert(pair_of_ints(6,1));
    test_serialization(m);
    
    m.insert(pair_of_ints(8,1));
    m.insert(pair_of_ints(9,1));
    m.insert(pair_of_ints(12,1));
    m.insert(pair_of_ints(11,1));
    m.insert(pair_of_ints(10,1));
    test_serialization(m);

    /* testcase for bug reported at
     * http://lists.boost.org/boost-users/2006/05/19362.php
     */

    m.clear();
    m.insert(pair_of_ints(0,0));
    m.insert(pair_of_ints(1,0));
    m.insert(pair_of_ints(2,1));
    m.insert(pair_of_ints(4,2));
    m.insert(pair_of_ints(3,2));
    test_serialization(m);
  }
}
