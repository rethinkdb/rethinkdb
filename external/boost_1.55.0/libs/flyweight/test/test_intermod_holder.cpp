/* Boost.Flyweight test of intermodule_holder.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include "test_intermod_holder.hpp"

#include "intermod_holder_dll.hpp"
#include "test_basic_template.hpp"

using namespace boost::flyweights;

struct intermodule_holder_flyweight_specifier1
{
  template<typename T>
  struct apply
  {
    typedef flyweight<T,intermodule_holder> type;
  };
};

void test_intermodule_holder()
{
  test_basic_template<intermodule_holder_flyweight_specifier1>();

  intermodule_flyweight_string str=
    create_intermodule_flyweight_string("boost");
  BOOST_TEST(str==intermodule_flyweight_string("boost"));
}
