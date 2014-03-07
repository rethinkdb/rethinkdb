/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/fusion/sequence/intrinsic/swap.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/mpl/assert.hpp>

#include <boost/type_traits/is_same.hpp>

#include <vector>

int main()
{
    namespace fusion = boost::fusion;
    {
        typedef fusion::vector<std::vector<int>, char> test_vector;
        BOOST_MPL_ASSERT((boost::is_same<void, boost::fusion::result_of::swap<test_vector, test_vector>::type>));

        test_vector v1(std::vector<int>(1, 101), 'a'), v2(std::vector<int>(1, 202), 'b');

        fusion::swap(v1, v2);

        BOOST_TEST(v1 == fusion::make_vector(std::vector<int>(1, 202), 'b'));
        BOOST_TEST(v2 == fusion::make_vector(std::vector<int>(1, 101), 'a'));
    }
    return boost::report_errors();
}
