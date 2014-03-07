/* Boost.Flyweight test of static data initialization facilities.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include "test_init.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/lightweight_test.hpp>
#include <boost/flyweight.hpp> 

using namespace boost::flyweights;

template<bool* pmark,typename Entry,typename Value>
struct marked_hashed_factory_class:hashed_factory_class<Entry,Value>
{
  marked_hashed_factory_class(){*pmark=true;}
};

template<bool* pmark>
struct marked_hashed_factory:factory_marker
{
  template<typename Entry,typename Value>
  struct apply
  {
    typedef marked_hashed_factory_class<pmark,Entry,Value> type;
  };
};

namespace{
bool mark1=false;
bool init1=flyweight<int,marked_hashed_factory<&mark1> >::init();

bool mark2=false;
flyweight<int,marked_hashed_factory<&mark2> >::initializer init2;
}

void test_init()
{
  BOOST_TEST(mark1);
  BOOST_TEST(mark2);
}
