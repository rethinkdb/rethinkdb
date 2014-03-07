/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include "./example_struct.hpp"
#include "./example_struct_type.hpp"
#include <boost/detail/lightweight_test.hpp>

#include <boost/fusion/sequence/intrinsic.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/iterator.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/assert.hpp>

int main()
{
    example::example_struct bert("bert", 99);
    using namespace boost::fusion;

    BOOST_MPL_ASSERT((traits::is_associative<example::example_struct>));
    BOOST_MPL_ASSERT((traits::is_random_access<example::example_struct>));
    BOOST_MPL_ASSERT((traits::is_sequence<example::example_struct>));

    BOOST_TEST(deref(begin(bert)) == "bert");
    BOOST_TEST(*next(begin(bert)) == 99);
    BOOST_TEST(*prior(end(bert)) == 99);
    BOOST_TEST(*advance_c<1>(begin(bert)) == 99);
    BOOST_TEST(*advance_c<-1>(end(bert)) == 99);
    BOOST_TEST(distance(begin(bert), end(bert)) == 2);

    typedef result_of::begin<example::example_struct>::type first;
    typedef result_of::next<first>::type second;
    BOOST_MPL_ASSERT((boost::is_same<result_of::value_of<first>::type, std::string>));
    BOOST_MPL_ASSERT((boost::is_same<result_of::value_of<second>::type, int>));

    BOOST_TEST(begin(bert) != end(bert));
    BOOST_TEST(advance_c<2>(begin(bert)) == end(const_cast<const example::example_struct&>(bert)));

    BOOST_TEST(at_c<0>(bert) == "bert");
    BOOST_TEST(at_c<1>(bert) == 99);

    BOOST_TEST(at_key<fields::name>(bert) == "bert");
    BOOST_TEST(at_key<fields::age>(bert) == 99);

    BOOST_TEST(has_key<fields::name>(bert));
    BOOST_TEST(has_key<fields::age>(bert));
    BOOST_TEST(!has_key<int>(bert));

    BOOST_MPL_ASSERT((boost::is_same<result_of::value_at_c<example::example_struct, 0>::type, std::string>));
    BOOST_MPL_ASSERT((boost::is_same<result_of::value_at_c<example::example_struct, 1>::type, int>));

    BOOST_MPL_ASSERT((boost::is_same<result_of::value_at_key<example::example_struct, fields::name>::type, std::string>));
    BOOST_MPL_ASSERT((boost::is_same<result_of::value_at_key<example::example_struct, fields::age>::type, int>));

    BOOST_TEST(deref_data(begin(bert)) == "bert");
    BOOST_TEST(deref_data(next(begin(bert))) == 99);

    BOOST_TEST(size(bert) == 2);

    return boost::report_errors();
}
