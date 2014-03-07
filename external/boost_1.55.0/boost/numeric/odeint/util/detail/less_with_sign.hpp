/*
 [auto_generated]
 boost/numeric/odeint/integrate/detail/less_with_sign.hpp

 [begin_description]
 Helper function to compare times taking into account the sign of dt
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_LESS_WITH_SIGN_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_LESS_WITH_SIGN_HPP_INCLUDED

#include <boost/numeric/odeint/util/unit_helper.hpp>

namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

/**
 * return t1 < t2 if dt > 0 and t1 > t2 if dt < 0
 */
template< typename T1 , typename T2 , typename T3 >
bool less_with_sign( T1 t1 , T2 t2 , T3 dt )
{
    if( get_unit_value(dt) > 0 )
        return t1 < t2;
    else
        return t1 > t2;
}

/**
 * return t1 <= t2 if dt > 0 and t1 => t2 if dt < 0
 */
template< typename T1 , typename T2 , typename T3>
bool less_eq_with_sign( T1 t1 , T2 t2 , T3 dt )
{
    if( get_unit_value(dt) > 0 )
        return t1 <= t2;
    else
        return t1 >= t2;
}

} } } }

#endif
