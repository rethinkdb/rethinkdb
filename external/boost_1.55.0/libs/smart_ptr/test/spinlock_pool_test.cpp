//
// spinlock_pool_test.cpp
//
// Copyright 2008 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/smart_ptr/detail/spinlock_pool.hpp>

// Sanity check only

int main()
{
    int x = 0;

    {
        boost::detail::spinlock_pool<0>::scoped_lock lock( &x );
        ++x;
    }

    {
        boost::detail::spinlock_pool<1>::scoped_lock lock( &x );
        boost::detail::spinlock_pool<2>::scoped_lock lock2( &x );
    }

    return 0;
}
