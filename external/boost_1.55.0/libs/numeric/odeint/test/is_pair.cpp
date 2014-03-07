/*
  [auto_generated]
  libs/numeric/odeint/test/is_pair.cpp

  [begin_description]
  This file tests the is_pair meta-function.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#define BOOST_TEST_MODULE odeint_is_pair

#include <utility>

#include <boost/test/unit_test.hpp>
#include <boost/static_assert.hpp>

#include <boost/numeric/odeint/util/is_pair.hpp>

using namespace boost::numeric::odeint;



BOOST_AUTO_TEST_SUITE( is_pair_test )

BOOST_AUTO_TEST_CASE( test_is_pair )
{
    typedef std::pair< int , int > type1;
    typedef std::pair< int& , int > type2;
    typedef std::pair< int , int& > type3;
    typedef std::pair< int& , int& > type4;
    typedef std::pair< const int , int > type5;
    typedef std::pair< const int& , int > type6;

    BOOST_STATIC_ASSERT(( is_pair< type1 >::value ));
    BOOST_STATIC_ASSERT(( is_pair< type2 >::value ));
    BOOST_STATIC_ASSERT(( is_pair< type3 >::value ));
    BOOST_STATIC_ASSERT(( is_pair< type4 >::value ));
    BOOST_STATIC_ASSERT(( is_pair< type5 >::value ));
    BOOST_STATIC_ASSERT(( is_pair< type6 >::value ));
}

BOOST_AUTO_TEST_SUITE_END()
