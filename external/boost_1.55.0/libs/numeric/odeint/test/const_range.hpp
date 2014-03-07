/*
  [auto_generated]
  libs/numeric/odeint/test/const_range.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef LIBS_NUMERIC_ODEINT_TEST_CONST_RANGE_HPP_DEFINED
#define LIBS_NUMERIC_ODEINT_TEST_CONST_RANGE_HPP_DEFINED

#include <boost/mpl/copy.hpp>
#include <boost/mpl/inserter.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/placeholders.hpp>


namespace mpl = boost::mpl;



template< class N , class T >
struct const_range
{
    typedef typename mpl::copy<
        mpl::range_c< typename N::value_type , 0 , N::value > ,
        mpl::inserter<
            mpl::vector0<> ,
            mpl::insert<
                mpl::_1 ,
                mpl::end< mpl::_1 > ,
                T
                >
            >
        >::type type;
};

#endif // LIBS_NUMERIC_ODEINT_TEST_CONST_RANGE_HPP_DEFINED
