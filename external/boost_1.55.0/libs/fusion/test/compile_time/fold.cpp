/*=============================================================================
    Copyright (c) 2008 Dan Marsden
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/include/fold.hpp>
#include <boost/fusion/include/vector.hpp>

namespace fusion = boost::fusion;

namespace
{
  template<int n, int batch>
  struct distinct
  {};

  struct f
  {
    typedef int result_type;

    template<int n, int batch>
    int operator()(int state, distinct<n, batch> const& d) const
    {
      return state + n;
    }
  };

  template<int batch>
  void test()
  {
    fusion::vector<
      distinct<0, batch>, distinct<1, batch>, distinct<2, batch>, distinct<3, batch>, distinct<4, batch>,
      distinct<5, batch>, distinct<6, batch>, distinct<7, batch>, distinct<8, batch>, distinct<9, batch> > v;

    fusion::fold(v, 0, f());
  }
}

#include "./driver.hpp"
