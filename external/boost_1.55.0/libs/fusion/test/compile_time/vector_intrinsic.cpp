/*=============================================================================
    Copyright (c) 2008 Dan Marsden
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/intrinsic.hpp>

namespace fusion = boost::fusion;

namespace
{
  template<int n, int batch>
  struct distinct
  {};

  template<int batch>
  void test()
  {
    typedef fusion::vector<
      distinct<0, batch>, distinct<1, batch>, distinct<2, batch>, distinct<3, batch>, distinct<4, batch>,
      distinct<5, batch>, distinct<6, batch>, distinct<7, batch>, distinct<8, batch>, distinct<9, batch> > v_type;

    v_type v;

    fusion::at_c<0>(v);
    fusion::at_c<1>(v);
    fusion::at_c<2>(v);
    fusion::at_c<3>(v);
    fusion::at_c<4>(v);

    fusion::at_c<5>(v);
    fusion::at_c<6>(v);
    fusion::at_c<7>(v);
    fusion::at_c<8>(v);
    fusion::at_c<9>(v);

    typedef typename boost::fusion::result_of::value_at_c<v_type, 0>::type va0;
    typedef typename boost::fusion::result_of::value_at_c<v_type, 1>::type va1;
    typedef typename boost::fusion::result_of::value_at_c<v_type, 2>::type va2;
    typedef typename boost::fusion::result_of::value_at_c<v_type, 3>::type va3;
    typedef typename boost::fusion::result_of::value_at_c<v_type, 4>::type va4;

    typedef typename boost::fusion::result_of::value_at_c<v_type, 5>::type va5;
    typedef typename boost::fusion::result_of::value_at_c<v_type, 6>::type va6;
    typedef typename boost::fusion::result_of::value_at_c<v_type, 7>::type va7;
    typedef typename boost::fusion::result_of::value_at_c<v_type, 8>::type va8;
    typedef typename boost::fusion::result_of::value_at_c<v_type, 9>::type va9;

    fusion::begin(v);
    fusion::end(v);
    fusion::size(v);
  }
}

#include "./driver.hpp"
