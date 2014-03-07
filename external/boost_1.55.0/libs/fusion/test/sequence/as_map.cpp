/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/generation/make_list.hpp>
#include <boost/fusion/container/map/convert.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/algorithm/transformation/push_back.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/support/pair.hpp>

int
main()
{
    using namespace boost::fusion;
    using namespace boost;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        vector0<> empty;
        std::cout << as_map(make_list(make_pair<int>('X'), make_pair<double>("Men"))) << std::endl;
        std::cout << as_map(push_back(empty, make_pair<int>(999))) << std::endl;
    }

    {
        typedef pair<int, char> p1;
        typedef pair<double, std::string> p2;
        boost::fusion::result_of::as_map<list<p1, p2> >::type map(make_pair<int>('X'), make_pair<double>("Men"));
        std::cout << at_key<int>(map) << std::endl;
        std::cout << at_key<double>(map) << std::endl;
        BOOST_TEST(at_key<int>(map) == 'X');
        BOOST_TEST(at_key<double>(map) == "Men");
    }

    {
        // test conversion
        typedef map<
            pair<int, char>
          , pair<double, std::string> > 
        map_type;

        map_type m(make_vector(make_pair<int>('X'), make_pair<double>("Men")));
        BOOST_TEST(as_vector(m) == make_vector(make_pair<int>('X'), make_pair<double>("Men")));
        m = (make_vector(make_pair<int>('X'), make_pair<double>("Men"))); // test assign
        BOOST_TEST(as_vector(m) == make_vector(make_pair<int>('X'), make_pair<double>("Men")));
    }

    return boost::report_errors();
}

