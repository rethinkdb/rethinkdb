
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#ifdef BOOST_HASH_TEST_EXTENSIONS
#  ifdef BOOST_HASH_TEST_STD_INCLUDES
#    include <functional>
#  else
#    include <boost/functional/hash.hpp>
#  endif
#endif

#include <boost/detail/lightweight_test.hpp>

#include <map>

#ifdef BOOST_HASH_TEST_EXTENSIONS

using std::map;
#define CONTAINER_TYPE map
#include "./hash_map_test.hpp"

using std::multimap;
#define CONTAINER_TYPE multimap
#include "./hash_map_test.hpp"

#endif // BOOST_HASH_TEST_EXTENSIONS

int main()
{
#ifdef BOOST_HASH_TEST_EXTENSIONS
    map_tests::map_hash_integer_tests();
    multimap_tests::multimap_hash_integer_tests();
#endif

    return boost::report_errors();
}
