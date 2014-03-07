/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2007 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/view/zip_view.hpp>
#include <boost/fusion/support/unused.hpp>
#include <boost/fusion/iterator.hpp>
#include <boost/fusion/sequence/intrinsic.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

int main()
{
    using namespace boost::fusion;
    {
        typedef vector2<int,int> int_vector;
        typedef vector2<char,char> char_vector;
        typedef vector<int_vector&, unused_type const&, char_vector&> seqs_type;
        typedef zip_view<seqs_type> view;

        int_vector iv(1,2);
        char_vector cv('a','b');
        seqs_type seq(iv, unused, cv);
        view v(seq);

        BOOST_TEST(at_c<0>(front(v)) == 1);
        BOOST_TEST(at_c<2>(front(v)) == 'a');
        BOOST_TEST(at_c<0>(back(v)) == 2);
        BOOST_TEST(at_c<2>(back(v)) == 'b');

        typedef boost::fusion::result_of::begin<view>::type first_iterator;
        typedef boost::fusion::result_of::value_of<first_iterator>::type first_element;

        typedef boost::fusion::result_of::at_c<first_element, 0>::type e0;
        typedef boost::fusion::result_of::at_c<first_element, 2>::type e2;
        BOOST_MPL_ASSERT((boost::is_same<e0, int&>));
        BOOST_MPL_ASSERT((boost::is_same<e2, char&>));

        BOOST_TEST(size(front(v)) == 3);

        typedef boost::fusion::result_of::value_at_c<view, 0>::type first_value_at;
        typedef boost::fusion::result_of::value_at_c<first_value_at, 0>::type v0;
        typedef boost::fusion::result_of::value_at_c<first_value_at, 2>::type v2;

        BOOST_MPL_ASSERT((boost::is_same<v0, int>));
        BOOST_MPL_ASSERT((boost::is_same<v2, char>));

        BOOST_TEST(at_c<0>(at_c<0>(v)) == 1);
        BOOST_TEST(at_c<2>(at_c<0>(v)) == 'a');
    }
    return boost::report_errors();
}
