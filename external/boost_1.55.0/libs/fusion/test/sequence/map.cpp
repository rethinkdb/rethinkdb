/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/map/map.hpp>
#include <boost/fusion/container/generation/make_map.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>
#include <boost/fusion/sequence/intrinsic/value_at_key.hpp>
#include <boost/fusion/sequence/intrinsic/has_key.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/iterator/key_of.hpp>
#include <boost/fusion/iterator/deref_data.hpp>
#include <boost/fusion/iterator/value_of_data.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/assert.hpp>
#include <iostream>
#include <string>

int
main()
{
    using namespace boost::fusion;
    using namespace boost;
    namespace fusion = boost::fusion;
    using boost::fusion::pair;
    using boost::fusion::make_pair;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        typedef map<
            pair<int, char>
          , pair<double, std::string> > 
        map_type;

        BOOST_MPL_ASSERT((traits::is_associative<map_type>));
        
        map_type m(
            make_pair<int>('X')
          , make_pair<double>("Men"));
        
        std::cout << at_key<int>(m) << std::endl;
        std::cout << at_key<double>(m) << std::endl;
        
        BOOST_TEST(at_key<int>(m) == 'X');
        BOOST_TEST(at_key<double>(m) == "Men");

        BOOST_STATIC_ASSERT((
            boost::is_same<boost::fusion::result_of::value_at_key<map_type, int>::type, char>::value));
        BOOST_STATIC_ASSERT((
            boost::is_same<boost::fusion::result_of::value_at_key<map_type, double>::type, std::string>::value));
        
        std::cout << m << std::endl;

        BOOST_STATIC_ASSERT((boost::fusion::result_of::has_key<map_type, int>::value));
        BOOST_STATIC_ASSERT((boost::fusion::result_of::has_key<map_type, double>::value));
        BOOST_STATIC_ASSERT((!boost::fusion::result_of::has_key<map_type, std::string>::value));

        std::cout << deref_data(begin(m)) << std::endl;
        std::cout << deref_data(fusion::next(begin(m))) << std::endl;

        BOOST_TEST(deref_data(begin(m)) == 'X');
        BOOST_TEST(deref_data(fusion::next(begin(m))) == "Men");

        BOOST_STATIC_ASSERT((boost::is_same<boost::fusion::result_of::key_of<boost::fusion::result_of::begin<map_type>::type>::type, int>::value));
        BOOST_STATIC_ASSERT((boost::is_same<boost::fusion::result_of::key_of<boost::fusion::result_of::next<boost::fusion::result_of::begin<map_type>::type>::type>::type, double>::value));
        BOOST_STATIC_ASSERT((boost::is_same<boost::fusion::result_of::value_of_data<boost::fusion::result_of::begin<map_type>::type>::type, char>::value));
        BOOST_STATIC_ASSERT((boost::is_same<boost::fusion::result_of::value_of_data<boost::fusion::result_of::next<boost::fusion::result_of::begin<map_type>::type>::type>::type, std::string>::value));
    }
    
    {
        std::cout << make_map<char, int>('X', 123) << std::endl;
        BOOST_TEST(at_key<char>(make_map<char, int>('X', 123)) == 'X');
        BOOST_TEST(at_key<int>(make_map<char, int>('X', 123)) == 123);
    }

    return boost::report_errors();
}

