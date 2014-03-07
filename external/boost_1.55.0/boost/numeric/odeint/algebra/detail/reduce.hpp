/*
 [auto_generated]
 boost/numeric/odeint/algebra/detail/reduce.hpp

 [begin_description]
 Default reduce implementation.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_ALGEBRA_DETAIL_REDUCE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_ALGEBRA_DETAIL_REDUCE_HPP_INCLUDED


namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

template< class ValueType , class Iterator1 , class Reduction >
inline ValueType reduce( Iterator1 first1 , Iterator1 last1 , Reduction red, ValueType init)
{
    for( ; first1 != last1 ; )
        init = red( init , *first1++ );
    return init;
}


template< class ValueType , class Iterator1 , class Iterator2 , class Reduction >
inline ValueType reduce2( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Reduction red, ValueType init)
{
    for( ; first1 != last1 ; )
        init = red( init , *first1++ , *first2++ );
    return init;
}

template< class ValueType , class Iterator1 , class Iterator2 , class Iterator3 , class Reduction >
inline ValueType reduce3( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3 , Reduction red, ValueType init)
{
    for( ; first1 != last1 ; )
        init = red( init , *first1++ , *first2++ , *first3++ );
    return init;
}

template< class ValueType , class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Reduction >
inline ValueType reduce4( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3 , Iterator4 first4 , Reduction red, ValueType init)
{
    for( ; first1 != last1 ; )
        init = red( init , *first1++ , *first2++ , *first3++ , *first4++ );
    return init;
}


} // detail
} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_ALGEBRA_DETAIL_REDUCE_HPP_INCLUDED
