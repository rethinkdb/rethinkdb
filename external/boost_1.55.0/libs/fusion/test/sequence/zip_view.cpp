/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/view/zip_view.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/list.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/front.hpp>
#include <boost/fusion/sequence/intrinsic/back.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/prior.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/vector_c.hpp>

#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>

int main()
{
    using namespace boost::fusion;
    {
        typedef vector2<int,int> int_vector;
        typedef vector2<char,char> char_vector;
        typedef vector<int_vector&, char_vector&> seqs_type;
        typedef zip_view<seqs_type> view;

        BOOST_MPL_ASSERT((boost::mpl::equal_to<boost::fusion::result_of::size<view>::type, boost::fusion::result_of::size<int_vector>::type>));
        BOOST_STATIC_ASSERT((boost::fusion::result_of::size<view>::value == 2));

        int_vector iv(1,2);
        char_vector cv('a', 'b');
        seqs_type seqs(iv, cv);
        view v(seqs);

        BOOST_TEST(at_c<0>(v) == make_vector(1, 'a'));
        BOOST_TEST(at_c<1>(v) == make_vector(2, 'b'));
        BOOST_TEST(front(v) == make_vector(1, 'a'));
        BOOST_TEST(back(v) == make_vector(2, 'b'));
        BOOST_TEST(*next(begin(v)) == make_vector(2, 'b'));
        BOOST_TEST(*prior(end(v)) == make_vector(2, 'b'));
        BOOST_TEST(advance_c<2>(begin(v)) == end(v));
        BOOST_TEST(advance_c<-2>(end(v)) == begin(v));
        BOOST_TEST(distance(begin(v), end(v)) == 2);

        BOOST_STATIC_ASSERT((boost::fusion::result_of::distance<boost::fusion::result_of::begin<view>::type, boost::fusion::result_of::end<view>::type>::value == 2));

        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::value_at_c<view,0>::type, vector2<int,char> >));
        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::value_of<boost::fusion::result_of::begin<view>::type>::type, vector2<int,char> >));
    }
    {
        using namespace boost;
        typedef mpl::vector2<int,bool> v1_type;
        typedef mpl::vector2<char,long> v2_type;
        typedef fusion::vector<v1_type&, v2_type&> seqs_type;
        typedef fusion::zip_view<seqs_type> view;

        v1_type v1;
        v2_type v2;
        seqs_type seqs(v1,v2);
        view v(seqs);
        BOOST_TEST((fusion::at_c<0>(v) == mpl::vector2<int,char>()));
        BOOST_TEST((fusion::at_c<1>(v) == mpl::vector2<bool,long>()));
        BOOST_TEST((fusion::front(v) == mpl::vector2<int,char>()));
        BOOST_TEST((fusion::back(v) == mpl::vector2<bool,long>()));
        BOOST_TEST((*fusion::next(fusion::begin(v)) == mpl::vector2<bool,long>()));
        BOOST_TEST((*fusion::prior(fusion::end(v)) == mpl::vector2<bool,long>()));
        BOOST_TEST(fusion::advance_c<2>(fusion::begin(v)) == fusion::end(v));
        BOOST_TEST(fusion::advance_c<-2>(fusion::end(v)) == fusion::begin(v));
        BOOST_TEST(fusion::distance(fusion::begin(v), fusion::end(v)) == 2);
    }
    return boost::report_errors();
}

