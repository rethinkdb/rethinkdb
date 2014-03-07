/*=============================================================================
    Copyright (c) 2008 Dan Marsden
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/include/transform.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/vector.hpp>

namespace fusion = boost::fusion;

namespace
{
  template<int n, int batch>
  struct distinct
  {
    static const int value = n;
  };

  struct f
  {
    typedef int result_type;

    template<typename T>
    result_type operator()(T const& t) const
    {
      return T::value;
    }
  };

  struct touch
  {
    template<typename T>
    void operator()(T const&) const
    {}
  };

  template<int batch>
  void test()
  {
    fusion::vector<
      distinct<0, batch>, distinct<1, batch>, distinct<2, batch>, distinct<3, batch>, distinct<4, batch>,
      distinct<5, batch>, distinct<6, batch>, distinct<7, batch>, distinct<8, batch>, distinct<9, batch> > v;

    // We're testing transform really
    // for_each call is to force iteration through the lazy
    // transform, otherwise very little will happen.
    fusion::for_each(fusion::transform(v, f()), touch());
  }
}

#include "./driver.hpp"
