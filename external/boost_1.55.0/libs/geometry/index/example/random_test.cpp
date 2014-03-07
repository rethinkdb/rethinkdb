// Boost.Geometry Index
// Additional tests

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#define BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
#include <boost/geometry/index/rtree.hpp>

#include <boost/foreach.hpp>
#include <boost/random.hpp>

int main()
{
    namespace bg = boost::geometry;
    namespace bgi = bg::index;

    size_t values_count = 1000000;
    size_t queries_count = 10000;
    size_t nearest_queries_count = 10000;
    unsigned neighbours_count = 10;

    std::vector< std::pair<float, float> > coords;

    //randomize values
    {
        boost::mt19937 rng;
        //rng.seed(static_cast<unsigned int>(std::time(0)));
        float max_val = static_cast<float>(values_count / 2);
        boost::uniform_real<float> range(-max_val, max_val);
        boost::variate_generator<boost::mt19937&, boost::uniform_real<float> > rnd(rng, range);

        coords.reserve(values_count);

        std::cout << "randomizing data\n";
        for ( size_t i = 0 ; i < values_count ; ++i )
        {
            coords.push_back(std::make_pair(rnd(), rnd()));
        }
        std::cout << "randomized\n";
    }

    typedef bg::model::point<double, 2, bg::cs::cartesian> P;
    typedef bg::model::box<P> B;
    typedef bgi::rtree<B, bgi::linear<16, 4> > RT;
    //typedef bgi::rtree<B, bgi::quadratic<8, 3> > RT;
    //typedef bgi::rtree<B, bgi::rstar<8, 3> > RT;

    std::cout << "sizeof rtree: " << sizeof(RT) << std::endl;

    {
        RT t;

        // inserting
        {
            for (size_t i = 0 ; i < values_count ; ++i )
            {
                float x = coords[i].first;
                float y = coords[i].second;
                B b(P(x - 0.5f, y - 0.5f), P(x + 0.5f, y + 0.5f));

                t.insert(b);
            }
            std::cout << "inserted values: " << values_count << '\n';
        }

        std::vector<B> result;
        result.reserve(100);
        
        // test
        std::vector<size_t> spatial_query_data;
        size_t spatial_query_index = 0;

        {
            size_t found_count = 0;
            for (size_t i = 0 ; i < queries_count ; ++i )
            {
                float x = coords[i].first;
                float y = coords[i].second;
                result.clear();
                t.query(bgi::intersects(B(P(x - 10, y - 10), P(x + 10, y + 10))), std::back_inserter(result));
                
                // test
                spatial_query_data.push_back(result.size());
                found_count += result.size();
            }
            std::cout << "spatial queries found: " << found_count << '\n';
        }

#ifdef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
        {
            size_t found_count = 0;
            for (size_t i = 0 ; i < queries_count ; ++i )
            {
                float x = coords[i].first;
                float y = coords[i].second;
                result.clear();
                std::copy(t.qbegin_(bgi::intersects(B(P(x - 10, y - 10), P(x + 10, y + 10)))),
                          t.qend_(bgi::intersects(B(P(x - 10, y - 10), P(x + 10, y + 10)))),
                          std::back_inserter(result));

                // test
                if ( spatial_query_data[spatial_query_index] != result.size() )
                    std::cout << "Spatial query error - should be: " << spatial_query_data[spatial_query_index] << ", is: " << result.size() << '\n';
                ++spatial_query_index;
                found_count += result.size();
            }
            std::cout << "incremental spatial queries found: " << found_count << '\n';
        }
#endif

        // test
        std::vector<float> nearest_query_data;
        size_t nearest_query_data_index = 0;

        {
            size_t found_count = 0;
            for (size_t i = 0 ; i < nearest_queries_count ; ++i )
            {
                float x = coords[i].first + 100;
                float y = coords[i].second + 100;
                result.clear();
                t.query(bgi::nearest(P(x, y), neighbours_count), std::back_inserter(result));

                // test
                {
                    float max_dist = 0;
                    BOOST_FOREACH(B const& b, result)
                    {
                        float curr_dist = bgi::detail::comparable_distance_near(P(x, y), b);
                        if ( max_dist < curr_dist )
                            max_dist = curr_dist;
                    }
                    nearest_query_data.push_back(max_dist);
                }
                found_count += result.size();
            }
            std::cout << "nearest queries found: " << found_count << '\n';
        }

#ifdef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
        {
            size_t found_count = 0;
            for (size_t i = 0 ; i < nearest_queries_count ; ++i )
            {
                float x = coords[i].first + 100;
                float y = coords[i].second + 100;
                result.clear();

                std::copy(t.qbegin_(bgi::nearest(P(x, y), neighbours_count)),
                          t.qend_(bgi::nearest(P(x, y), neighbours_count)),
                          std::back_inserter(result));

                // test
                {
                    float max_dist = 0;
                    BOOST_FOREACH(B const& b, result)
                    {
                        float curr_dist = bgi::detail::comparable_distance_near(P(x, y), b);
                        if ( max_dist < curr_dist )
                            max_dist = curr_dist;
                    }
                    if ( nearest_query_data_index < nearest_query_data.size() &&
                         nearest_query_data[nearest_query_data_index] != max_dist )
                         std::cout << "Max distance error - should be: " << nearest_query_data[nearest_query_data_index] << ", and is: " << max_dist << "\n";
                    ++nearest_query_data_index;
                }
                found_count += result.size();
            }
            std::cout << "incremental nearest queries found: " << found_count << '\n';
        }
#endif

        std::cout << "finished\n";
    }

    return 0;
}
