
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#if defined(BOOST_HASH_TEST_EXTENSIONS)

// Include header with BOOST_HASH_NO_EXTENSIONS defined
#  if !defined(BOOST_HASH_NO_EXTENSIONS)
#    define BOOST_HASH_NO_EXTENSIONS
#  endif
#  include <boost/functional/hash.hpp>

// Include header without BOOST_HASH_NO_EXTENSIONS defined
#  undef BOOST_HASH_NO_EXTENSIONS
#  include <boost/functional/hash.hpp>
#endif

#include <boost/detail/lightweight_test.hpp>
#include <map>

int main()
{
#if defined(BOOST_HASH_TEST_EXTENSIONS)
    std::map<int, int> x;

    x.insert(std::map<int, int>::value_type(53, -42));
    x.insert(std::map<int, int>::value_type(14, -75));

    BOOST_HASH_TEST_NAMESPACE::hash<std::map<int, int> > hasher;
    BOOST_TEST(hasher(x) == BOOST_HASH_TEST_NAMESPACE::hash_value(x));
#endif
    
    return boost::report_errors();
}
