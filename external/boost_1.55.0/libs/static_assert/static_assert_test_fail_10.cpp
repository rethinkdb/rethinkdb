//~ Copyright 2005 Redshift Software, Inc.
//~ Distributed under the Boost Software License, Version 1.0.
//~ (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <boost/static_assert.hpp>

template <int N>
int foo()
{
    BOOST_STATIC_ASSERT( N < 2 );
    
    return N;
}

int main()
{
    return foo<5>();
}
