
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#define BOOST_HASH_TEST_NAMESPACE boost
#include <boost/functional/hash.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <vector>

int f(std::size_t hash1, int* x1) {

    // Check that BOOST_HASH_TEST_NAMESPACE::hash<int*> works in both files.
    BOOST_HASH_TEST_NAMESPACE::hash<int*> ptr_hasher;
    BOOST_TEST(hash1 == ptr_hasher(x1));

#if defined(BOOST_HASH_TEST_EXTENSIONS)

    // Check that std::vector<std::size_t> is avaiable in this file.
    std::vector<std::size_t> x;
    x.push_back(*x1);
    BOOST_HASH_TEST_NAMESPACE::hash<std::vector<std::size_t> > vector_hasher;
    return vector_hasher(x) != BOOST_HASH_TEST_NAMESPACE::hash_value(x);
    
#else

    return 0;

#endif
}

