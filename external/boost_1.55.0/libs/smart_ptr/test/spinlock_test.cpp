//
// spinlock_test.cpp
//
// Copyright 2008 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/smart_ptr/detail/spinlock.hpp>

// Sanity check only

static boost::detail::spinlock sp = BOOST_DETAIL_SPINLOCK_INIT;
static boost::detail::spinlock sp2 = BOOST_DETAIL_SPINLOCK_INIT;

int main()
{
    sp.lock();
    sp2.lock();
    sp.unlock();
    sp2.unlock();

    {
        boost::detail::spinlock::scoped_lock lock( sp );
        boost::detail::spinlock::scoped_lock lock2( sp2 );
    }

    return 0;
}
