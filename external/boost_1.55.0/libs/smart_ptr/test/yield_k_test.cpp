//
// yield_k_test.cpp
//
// Copyright 2008 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/smart_ptr/detail/yield_k.hpp>

// Sanity check only

int main()
{
    for( unsigned k = 0; k < 256; ++k )
    {
        boost::detail::yield( k );
    }

    return 0;
}
