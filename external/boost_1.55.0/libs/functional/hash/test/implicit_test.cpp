
// Copyright 2010 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/functional/hash.hpp>

namespace test
{
    struct base {};
    std::size_t hash_value(base const&) { return 0; }
    
    struct converts { operator base() const { return base(); } };
}

int main() {
    boost::hash<test::converts> hash;
    test::converts x;
    
    hash(x);
}
