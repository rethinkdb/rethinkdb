/* Boost.Flyweight test of set_factory.
 *
 * Copyright 2006-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include "test_set_factory.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/refcounted.hpp>
#include <boost/flyweight/set_factory.hpp> 
#include <boost/flyweight/simple_locking.hpp>
#include <boost/flyweight/static_holder.hpp>
#include <functional>
#include "test_basic_template.hpp"

using namespace boost::flyweights;

struct set_factory_flyweight_specifier1
{
  template<typename T>
  struct apply
  {
    typedef flyweight<T,set_factory<> > type;
  };
};

struct set_factory_flyweight_specifier2
{
  template<typename T>
  struct apply
  {
    typedef flyweight<
      T,
      static_holder_class<boost::mpl::_1>,
      set_factory_class<
        boost::mpl::_1,boost::mpl::_2,
        std::greater<boost::mpl::_2>,
        std::allocator<boost::mpl::_1>
      >
    > type;
  };
};

struct set_factory_flyweight_specifier3
{
  template<typename T>
  struct apply
  {
    typedef flyweight<
      T,
      set_factory<
        std::greater<boost::mpl::_2>,
        std::allocator<boost::mpl::_1>
      >,
      static_holder_class<boost::mpl::_1>,
      tag<char>
    > type;
  };
};

void test_set_factory()
{
  test_basic_template<set_factory_flyweight_specifier1>();
  test_basic_template<set_factory_flyweight_specifier2>();
  test_basic_template<set_factory_flyweight_specifier3>();
}
