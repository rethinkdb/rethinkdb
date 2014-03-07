/* Boost.Flyweight basic test.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include "test_basic.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/flyweight.hpp> 
#include "test_basic_template.hpp"

using namespace boost::flyweights;

struct basic_flyweight_specifier1
{
  template<typename T>
  struct apply
  {
    typedef flyweight<T> type;
  };
};

struct basic_flyweight_specifier2
{
  template<typename T>
  struct apply
  {
    typedef flyweight<
      T,tag<int>,
      static_holder_class<boost::mpl::_1>,
      hashed_factory_class<
        boost::mpl::_1,boost::mpl::_2,
        boost::hash<boost::mpl::_2>,std::equal_to<boost::mpl::_2>,
        std::allocator<boost::mpl::_1>
      >,
      simple_locking,
      refcounted
    > type;
  };
};

struct basic_flyweight_specifier3
{
  template<typename T>
  struct apply
  {
    typedef flyweight<
      T,
      hashed_factory<
        boost::hash<boost::mpl::_2>,std::equal_to<boost::mpl::_2>,
        std::allocator<boost::mpl::_1>
      >,
      tag<int>
    > type;
  };
};

void test_basic()
{
  test_basic_template<basic_flyweight_specifier1>();
  test_basic_template<basic_flyweight_specifier2>();
  test_basic_template<basic_flyweight_specifier3>();
}
