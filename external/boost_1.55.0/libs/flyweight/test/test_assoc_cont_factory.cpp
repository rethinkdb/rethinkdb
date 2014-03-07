/* Boost.Flyweight test of assoc_container_factory.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include "test_assoc_cont_factory.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/flyweight/assoc_container_factory.hpp> 
#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/refcounted.hpp>
#include <boost/flyweight/simple_locking.hpp>
#include <boost/flyweight/static_holder.hpp>
#include <functional>
#include <set>
#include "test_basic_template.hpp"

using namespace boost::flyweights;

struct reverse_set_specifier
{
  template<typename Entry,typename Key>
  struct apply
  {
    typedef std::set<Entry,std::greater<Key> > type;
  };
};

struct assoc_container_factory_flyweight_specifier1
{
  template<typename T>
  struct apply
  {
    typedef flyweight<
      T,
      assoc_container_factory<reverse_set_specifier>
    > type;
  };
};

struct assoc_container_factory_flyweight_specifier2
{
  template<typename T>
  struct apply
  {
    typedef flyweight<
      T,
      assoc_container_factory_class<
        std::set<
          boost::mpl::_1,
          std::greater<boost::mpl::_2>,
          std::allocator<boost::mpl::_1>
        >
      >
    > type;
  };
};

void test_assoc_container_factory()
{
  test_basic_template<assoc_container_factory_flyweight_specifier1>();
  test_basic_template<assoc_container_factory_flyweight_specifier2>();
}
