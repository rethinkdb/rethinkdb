
// Copyright 2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Check that I haven't inadvertantly pulled namespace std into the global
// namespace.

#include "./config.hpp"

#include <list>
#include <boost/functional/hash.hpp>

typedef list<int> foo;

int main() {}
