/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2007 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/algorithm/transformation/zip.hpp>
#include <boost/fusion/support/unused.hpp>
#include <boost/fusion/iterator.hpp>
#include <boost/fusion/sequence/intrinsic.hpp>

int main()
{
    using namespace boost::fusion;
    {
        vector<int, int> iv(1,2);
        vector<char, char> cv('a', 'b');
        BOOST_TEST(at_c<0>(at_c<0>(zip(iv, unused, cv))) == 1);
        BOOST_TEST(at_c<2>(at_c<0>(zip(iv, unused, cv))) == 'a');

        BOOST_TEST(at_c<0>(at_c<1>(zip(iv, unused, cv))) == 2);
        BOOST_TEST(at_c<2>(at_c<1>(zip(iv, unused, cv))) == 'b');
    }
    return boost::report_errors();
}
