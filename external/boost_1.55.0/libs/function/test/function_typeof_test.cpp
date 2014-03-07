// Boost.Function library

//  Copyright Douglas Gregor 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org
#include <boost/function/function_typeof.hpp>
#include <boost/function.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>

void f(boost::function0<void> f, boost::function0<void> g)
{
  BOOST_STATIC_ASSERT((boost::is_same<boost::function0<void>, BOOST_TYPEOF(f = g)>::value));
}
