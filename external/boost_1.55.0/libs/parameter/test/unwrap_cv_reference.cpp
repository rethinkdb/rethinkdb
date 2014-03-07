// Copyright David Abrahams 2005. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/aux_/unwrap_cv_reference.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/ref.hpp>
#include <boost/type_traits/is_same.hpp>

namespace test
{
  using namespace boost::parameter::aux;
  using namespace boost;
  struct foo {};

  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<int>::type,int>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<int const>::type,int const>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<int volatile>::type,int volatile>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<int const volatile>::type,int const volatile>));
  
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<foo>::type,foo>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<foo const>::type,foo const>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<foo volatile>::type,foo volatile>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<foo const volatile>::type,foo const volatile>));
  
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<reference_wrapper<foo> >::type,foo>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<reference_wrapper<foo> const>::type,foo>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<reference_wrapper<foo> volatile>::type,foo>));
  BOOST_MPL_ASSERT((is_same<unwrap_cv_reference<reference_wrapper<foo> const volatile>::type,foo>));
}
