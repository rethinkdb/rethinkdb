/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/algorithm/transformation/join.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/mpl/vector/vector10_c.hpp>

int main()
{
    using namespace boost::fusion;
    {
        BOOST_TEST(join(make_vector(1,2), make_vector('a','b')) == make_vector(1,2,'a','b'));
    }
    {
        typedef boost::mpl::vector2_c<int,1,2> vec1;
        typedef boost::mpl::vector2_c<int,3,4> vec2;
        BOOST_TEST(join(vec1(), vec2()) == make_vector(1,2,3,4));
        
    }
    return boost::report_errors();
}
