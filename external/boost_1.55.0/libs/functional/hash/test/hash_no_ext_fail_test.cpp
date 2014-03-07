
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

// Simple test to make sure BOOST_HASH_NO_EXTENSIONS does disable extensions
// (or at least one of them).
#if !defined(BOOST_HASH_NO_EXTENSIONS)
#  define BOOST_HASH_NO_EXTENSIONS
#endif

#ifdef BOOST_HASH_TEST_STD_INCLUDES
#  include <functional>
#else
#  include <boost/functional/hash.hpp>
#endif

template <class T> void ignore(T const&) {}

int main()
{
    BOOST_HASH_TEST_NAMESPACE::hash< int[10] > hasher;
    ignore(hasher);

    return 0;
}
