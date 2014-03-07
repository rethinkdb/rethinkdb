/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/algorithm/transformation/zip.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/mpl/vector.hpp>

int main()
{
    using namespace boost::fusion;
    {
        BOOST_TEST(zip(make_vector(1,2), make_vector('a','b')) == make_vector(make_vector(1,'a'), make_vector(2,'b')));
        BOOST_TEST(
            zip(
                make_vector(1,2),
                make_vector('a','b'),
                make_vector(-1,-2))
            == make_vector(
                make_vector(1,'a',-1),
                make_vector(2,'b',-2))); // Zip more than 2 sequences
    }
    return boost::report_errors();
}
