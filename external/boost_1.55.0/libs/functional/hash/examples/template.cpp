
// Copyright 2012 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "template.hpp"
#include <cassert>
#include <boost/unordered_set.hpp>

int main()
{
    typedef my_pair<int, float> pair;
    boost::unordered_set<pair> pair_set;
    pair_set.emplace(10, 0.5f);

    assert(pair_set.find(pair(10, 0.5f)) != pair_set.end());
    assert(pair_set.find(pair(10, 0.6f)) == pair_set.end());
}
