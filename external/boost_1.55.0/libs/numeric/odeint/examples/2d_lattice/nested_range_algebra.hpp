/*
 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


/* nested range algebra */

#ifndef NESTED_RANGE_ALGEBRA
#define NESTED_RANGE_ALGEBRA

namespace detail {

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Operation , class Algebra >
    void for_each3( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3, Operation op , Algebra &algebra )
{
    for( ; first1 != last1 ; )
        algebra.for_each3( *first1++ , *first2++ , *first3++ , op );
}
}


template< class InnerAlgebra >
struct nested_range_algebra
{
    
    nested_range_algebra() 
        : m_inner_algebra()
    { }

    template< class S1 , class S2 , class S3 , class Op >
    void for_each3( S1 &s1 , S2 &s2 , S3 &s3 , Op op )
    {
        detail::for_each3( boost::begin( s1 ) , boost::end( s1 ) , boost::begin( s2 ) , boost::begin( s3 ) , op , m_inner_algebra );
    }


private:
    InnerAlgebra m_inner_algebra;
};

#endif
