///////////////////////////////////////////////////////////////////////////////
// utility.hpp header file
//
// Copyright 2005 Eric Niebler.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FOREACH_TEST_UTILITY_HPP
#define BOOST_FOREACH_TEST_UTILITY_HPP

#include <boost/config.hpp>
#include <boost/foreach.hpp>

///////////////////////////////////////////////////////////////////////////////
// sequence_equal_byval_n
inline bool sequence_equal_byval_n( foreach_container_type & rng, char const * result )
{
    BOOST_FOREACH( foreach_value_type i, rng )
    {
        if(0 == *result || i != *result)
            return false;
        ++result;
    }
    return 0 == *result;
}

///////////////////////////////////////////////////////////////////////////////
// sequence_equal_byval_c
inline bool sequence_equal_byval_c( foreach_const_container_type & rng, char const * result )
{
    BOOST_FOREACH( foreach_value_type i, rng )
    {
        if(0 == *result || i != *result)
            return false;
        ++result;
    }
    return 0 == *result;
}

///////////////////////////////////////////////////////////////////////////////
// sequence_equal_byref_n
inline bool sequence_equal_byref_n( foreach_container_type & rng, char const * result )
{
    BOOST_FOREACH( foreach_reference_type i, rng )
    {
        if(0 == *result || i != *result)
            return false;
        ++result;
    }
    return 0 == *result;
}

///////////////////////////////////////////////////////////////////////////////
// sequence_equal_byref_c
inline bool sequence_equal_byref_c( foreach_const_container_type & rng, char const * result )
{
    BOOST_FOREACH( foreach_const_reference_type i, rng )
    {
        if(0 == *result || i != *result)
            return false;
        ++result;
    }
    return 0 == *result;
}

///////////////////////////////////////////////////////////////////////////////
// mutate_foreach_byref
//
inline void mutate_foreach_byref( foreach_container_type & rng )
{
    BOOST_FOREACH( foreach_reference_type i, rng )
    {
        ++i;
    }
}


///////////////////////////////////////////////////////////////////////////////
// sequence_equal_byval_n_r
inline bool sequence_equal_byval_n_r( foreach_container_type & rng, char const * result )
{
    BOOST_REVERSE_FOREACH( foreach_value_type i, rng )
    {
        if(0 == *result || i != *result)
            return false;
        ++result;
    }
    return 0 == *result;
}

///////////////////////////////////////////////////////////////////////////////
// sequence_equal_byval_c_r
inline bool sequence_equal_byval_c_r( foreach_const_container_type & rng, char const * result )
{
    BOOST_REVERSE_FOREACH( foreach_value_type i, rng )
    {
        if(0 == *result || i != *result)
            return false;
        ++result;
    }
    return 0 == *result;
}

///////////////////////////////////////////////////////////////////////////////
// sequence_equal_byref_n_r
inline bool sequence_equal_byref_n_r( foreach_container_type & rng, char const * result )
{
    BOOST_REVERSE_FOREACH( foreach_reference_type i, rng )
    {
        if(0 == *result || i != *result)
            return false;
        ++result;
    }
    return 0 == *result;
}

///////////////////////////////////////////////////////////////////////////////
// sequence_equal_byref_c_r
inline bool sequence_equal_byref_c_r( foreach_const_container_type & rng, char const * result )
{
    BOOST_REVERSE_FOREACH( foreach_const_reference_type i, rng )
    {
        if(0 == *result || i != *result)
            return false;
        ++result;
    }
    return 0 == *result;
}

///////////////////////////////////////////////////////////////////////////////
// mutate_foreach_byref
//
inline void mutate_foreach_byref_r( foreach_container_type & rng )
{
    BOOST_REVERSE_FOREACH( foreach_reference_type i, rng )
    {
        ++i;
    }
}

#endif
