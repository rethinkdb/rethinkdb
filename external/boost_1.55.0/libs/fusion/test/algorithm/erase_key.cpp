/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/set/set.hpp>
#include <boost/fusion/container/generation/make_set.hpp>
#include <boost/fusion/container/map/map.hpp>
#include <boost/fusion/container/generation/make_map.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/container/set/convert.hpp>
#include <boost/fusion/container/map/convert.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/algorithm/transformation/erase_key.hpp>
#include <boost/fusion/algorithm/query/find.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/static_assert.hpp>
#include <iostream>
#include <string>

template <typename Set>
void test_set(Set const& set)
{
    using namespace boost::fusion;
    std::cout << set << std::endl;
    
    BOOST_STATIC_ASSERT(boost::fusion::result_of::size<Set>::value == 3);
    BOOST_TEST((*find<int>(set) == 1));
    BOOST_TEST((*find<double>(set) == 1.5));
    BOOST_TEST((*find<std::string>(set) == "hello"));
}

typedef boost::mpl::int_<1> _1;
typedef boost::mpl::int_<2> _2;
typedef boost::mpl::int_<3> _3;
typedef boost::mpl::int_<4> _4;
 
template <typename Map>
void test_map(Map const& map)
{
    using namespace boost::fusion;
    std::cout << map << std::endl;
    
    BOOST_STATIC_ASSERT(boost::fusion::result_of::size<Map>::value == 3);
    BOOST_TEST(((*find<_1>(map)).second == 1));
    BOOST_TEST(((*find<_3>(map)).second == 1.5));
    BOOST_TEST(((*find<_4>(map)).second == std::string("hello")));
}

int
main()
{
    using namespace boost::fusion;
    using namespace boost;
    using namespace std;
    using boost::fusion::pair;
    using boost::fusion::make_pair;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    test_set(erase_key<char>(make_set(1, 'x', 1.5, std::string("hello"))));
    test_map(erase_key<_2>(make_map<_1, _2, _3, _4>(1, 'x', 1.5, "hello")));

    return boost::report_errors();
}

