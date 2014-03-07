// Boost.Geometry Index
// Additional tests

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/geometry/index/rtree.hpp>

#include <boost/chrono.hpp>
#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <set>

int main()
{
    namespace bg = boost::geometry;
    namespace bgi = bg::index;
    typedef boost::chrono::thread_clock clock_t;
    typedef boost::chrono::duration<float> dur_t;

    size_t stored_count = 100000;

    std::vector< std::pair<float, float> > coords;

    //randomize values
    {
        boost::mt19937 rng;
        //rng.seed(static_cast<unsigned int>(std::time(0)));
        float max_val = static_cast<float>(stored_count / 10);
        boost::uniform_real<float> range(-max_val, max_val);
        boost::variate_generator<boost::mt19937&, boost::uniform_real<float> > rnd(rng, range);

        coords.reserve(stored_count);

        std::cout << "randomizing data\n";
        for ( size_t i = 0 ; i < stored_count ; ++i )
        {
            coords.push_back(std::make_pair(rnd(), rnd()));
        }
        std::cout << "randomized\n";
    }

    typedef bg::model::point<float, 2, bg::cs::cartesian> P;
    typedef bgi::rtree<P, bgi::dynamic_linear > RTL;
    typedef bgi::rtree<P, bgi::dynamic_quadratic > RTQ;
    typedef bgi::rtree<P, bgi::dynamic_rstar > RTR;

    for ( size_t m = 4 ; m < 33 ; m += 2 )
    {
        size_t mm = ::ceil(m / 3.0f);

        RTL rtl(bgi::dynamic_linear(m, mm));
        RTQ rtq(bgi::dynamic_quadratic(m, mm));
        RTR rtr(bgi::dynamic_rstar(m, mm));

        std::cout << m << ' ' << mm << ' ';

        // inserting test
        {
            clock_t::time_point start = clock_t::now();
            for (size_t i = 0 ; i < stored_count ; ++i )
            {
                P p(coords[i].first, coords[i].second);
                rtl.insert(p);
            }            
            dur_t time = clock_t::now() - start;
            std::cout << time.count() << ' ';

            start = clock_t::now();
            for (size_t i = 0 ; i < stored_count ; ++i )
            {
                P p(coords[i].first, coords[i].second);
                rtq.insert(p);
            }
            time = clock_t::now() - start;
            std::cout << time.count() << ' ';

            start = clock_t::now();
            for (size_t i = 0 ; i < stored_count ; ++i )
            {
                float v = i / 100.0f;
                P p(coords[i].first, coords[i].second);
                rtr.insert(p);
            }
            time = clock_t::now() - start;
            std::cout << time.count() << ' ';
        }

        std::cout << '\n';
    }

    return 0;
}
