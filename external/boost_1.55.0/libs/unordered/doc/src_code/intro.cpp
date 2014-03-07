
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//[intro_example1_1
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <cassert>
#include <iostream>
//]

int main() {
//[intro_example1_2
    typedef boost::unordered_map<std::string, int> map;
    map x;
    x["one"] = 1;
    x["two"] = 2;
    x["three"] = 3;

    assert(x.at("one") == 1);
    assert(x.find("missing") == x.end());
//]

//[intro_example1_3
    BOOST_FOREACH(map::value_type i, x) {
        std::cout<<i.first<<","<<i.second<<"\n";
    }
//]
}
