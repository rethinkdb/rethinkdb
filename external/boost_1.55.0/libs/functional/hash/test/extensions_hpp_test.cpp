
// Copyright 2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Check that boost/functional/hash/extensions.hpp works okay.
//
// It probably should be in boost/functional/hash/detail, but since it isn't it
// should work.

#include "./config.hpp"

#include <boost/functional/hash/extensions.hpp>

int main() {
    int x[2] = { 2, 3 };
    boost::hash<int[2]> hf;
    hf(x);
}
