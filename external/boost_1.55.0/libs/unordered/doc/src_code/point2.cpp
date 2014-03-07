
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#include <boost/detail/lightweight_test.hpp>

//[point_example2
    struct point {
        int x;
        int y;
    };

    bool operator==(point const& p1, point const& p2)
    {
        return p1.x == p2.x && p1.y == p2.y;
    }

    std::size_t hash_value(point const& p) {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.x);
        boost::hash_combine(seed, p.y);
        return seed;
    }

    // Now the default function objects work.
    boost::unordered_multiset<point> points;
//]

int main() {
    point x[] = {{1,2}, {3,4}, {1,5}, {1,2}};
    for(int i = 0; i < sizeof(x) / sizeof(point); ++i)
        points.insert(x[i]);
    BOOST_TEST(points.count(x[0]) == 2);
    BOOST_TEST(points.count(x[1]) == 1);
    point y = {10, 2};
    BOOST_TEST(points.count(y) == 0);

    return boost::report_errors();
}

