/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <string>
#include <boost/static_assert.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/prior.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/distance.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/value_of.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>

void test()
{
    using namespace boost::fusion;
    using namespace boost;

    { // Testing deref, next, prior, begin, end

        char const* s = "Hello";
        typedef FUSION_SEQUENCE<int, char, double, char const*> seq_type;
        seq_type v(1, 'x', 3.3, s);
        boost::fusion::result_of::begin<seq_type>::type i(v);

        BOOST_TEST(*i == 1);
        BOOST_TEST(*next(i) == 'x');
        BOOST_TEST(*next(next(i)) == 3.3);
        BOOST_TEST(*next(next(next(i))) == s);
        next(next(next(next(i)))); // end

#if !defined(FUSION_NO_PRIOR)
        BOOST_TEST(*prior(next(next(next(i)))) == 3.3);
        BOOST_TEST(*prior(prior(next(next(next(i))))) == 'x');
        BOOST_TEST(*prior(prior(prior(next(next(next(i)))))) == 1);
#endif
        BOOST_TEST(*begin(v) == 1);
#if !defined(FUSION_NO_PRIOR)
        BOOST_TEST(*prior(end(v)) == s);
#endif

        *i = 3;
        BOOST_TEST(*i == 3);
        BOOST_TEST(&*i == &at_c<0>(v));

        // prove that it is mutable
        *i = 987;
        BOOST_TEST(*i == 987);
    }

    { // Testing const sequence and const iterator

        char const* s = "Hello";
        typedef FUSION_SEQUENCE<int, char, double, char const*> const seq_type;
        seq_type t(1, 'x', 3.3, s);
        boost::fusion::result_of::begin<seq_type>::type i(t);

        BOOST_TEST(*i == 1);
        BOOST_TEST(*next(i) == 'x');
        BOOST_TEST(*begin(t) == 1);
#if !defined(FUSION_NO_PRIOR)
        BOOST_TEST(*prior(end(t)) == s);
#endif

#ifdef FUSION_TEST_FAIL
        *i = 3; // must not compile
#endif
    }

    { // Testing iterator equality

        typedef FUSION_SEQUENCE<int, char, double, char const*> seq_type;
        typedef FUSION_SEQUENCE<int, char, double, char const*> const cseq_type;
        typedef boost::fusion::result_of::begin<seq_type>::type vi1;
        typedef boost::fusion::result_of::begin<cseq_type>::type vi2;
        BOOST_STATIC_ASSERT((boost::fusion::result_of::equal_to<vi1 const, vi1>::value));
        BOOST_STATIC_ASSERT((boost::fusion::result_of::equal_to<vi1, vi1 const>::value));
        BOOST_STATIC_ASSERT((boost::fusion::result_of::equal_to<vi1, vi2>::value));
        BOOST_STATIC_ASSERT((boost::fusion::result_of::equal_to<vi1 const, vi2>::value));
        BOOST_STATIC_ASSERT((boost::fusion::result_of::equal_to<vi1, vi2 const>::value));
        BOOST_STATIC_ASSERT((boost::fusion::result_of::equal_to<vi1 const, vi2 const>::value));
    }

    {
        typedef FUSION_SEQUENCE<int, int> seq_type;
        typedef boost::fusion::result_of::begin<seq_type>::type begin_type;
        typedef boost::fusion::result_of::end<seq_type>::type end_type;
        typedef boost::fusion::result_of::next<begin_type>::type i1;
        typedef boost::fusion::result_of::next<i1>::type i2;

        BOOST_STATIC_ASSERT((is_same<end_type, i2>::value));
    }

    { // testing deref, next, prior, begin, end

        char const* s = "Hello";
        typedef FUSION_SEQUENCE<int, char, double, char const*> seq_type;
        seq_type t(1, 'x', 3.3, s);
        boost::fusion::result_of::begin<seq_type>::type i(t);

        BOOST_TEST(*i == 1);
        BOOST_TEST(*next(i) == 'x');
        BOOST_TEST(*next(next(i)) == 3.3);
        BOOST_TEST(*next(next(next(i))) == s);

        next(next(next(next(i)))); // end
        
#ifdef FUSION_TEST_FAIL
        next(next(next(next(next(i))))); // past the end: must not compile
#endif

#if !defined(FUSION_NO_PRIOR)
        BOOST_TEST(*prior(next(next(next(i)))) == 3.3);
        BOOST_TEST(*prior(prior(next(next(next(i))))) == 'x');
        BOOST_TEST(*prior(prior(prior(next(next(next(i)))))) == 1);
#endif
        BOOST_TEST(*begin(t) == 1);
#if !defined(FUSION_NO_PRIOR)
        BOOST_TEST(*prior(end(t)) == s);
#endif

        *i = 3;
        BOOST_TEST(*i == 3);
        BOOST_TEST(*i == at_c<0>(t));
    }

    { // Testing distance

        typedef FUSION_SEQUENCE<int, char, double, char const*> seq_type;
        seq_type t(1, 'x', 3.3, "Hello");

        BOOST_STATIC_ASSERT((boost::fusion::result_of::distance<
            boost::fusion::result_of::begin<seq_type>::type
          , boost::fusion::result_of::end<seq_type>::type >::value == 4));

        BOOST_TEST(distance(begin(t), end(t)).value == 4);
    }

    { // Testing tuple iterator boost::fusion::result_of::value_of, boost::fusion::result_of::deref, boost::fusion::result_of::value_at

        typedef FUSION_SEQUENCE<int, char&> seq_type;
        typedef boost::fusion::result_of::begin<seq_type>::type i0;
        typedef boost::fusion::result_of::next<i0>::type i1;
        typedef boost::fusion::result_of::next<boost::fusion::result_of::begin<const seq_type>::type>::type i2;

        BOOST_STATIC_ASSERT((
            is_same<boost::fusion::result_of::value_at_c<seq_type, 0>::type, int>::value));

        BOOST_STATIC_ASSERT((
            is_same<boost::fusion::result_of::value_at_c<seq_type, 1>::type, char&>::value));

        BOOST_STATIC_ASSERT((
            is_same<traits::category_of<i0>::type, FUSION_TRAVERSAL_TAG>::value));

        BOOST_STATIC_ASSERT((is_same<boost::fusion::result_of::deref<i0>::type, int&>::value));
        BOOST_STATIC_ASSERT((is_same<boost::fusion::result_of::deref<i1>::type, char&>::value));

        BOOST_STATIC_ASSERT((is_same<boost::fusion::result_of::value_of<i0>::type, int>::value));
        BOOST_STATIC_ASSERT((is_same<boost::fusion::result_of::value_of<i1>::type, char&>::value));
    }

    { // Testing advance

        typedef FUSION_SEQUENCE<int, char, double, char const*> seq_type;
        seq_type t(1, 'x', 3.3, "Hello");

        BOOST_TEST(*advance_c<0>(begin(t)) == at_c<0>(t));
        BOOST_TEST(*advance_c<1>(begin(t)) == at_c<1>(t));
        BOOST_TEST(*advance_c<2>(begin(t)) == at_c<2>(t));
        BOOST_TEST(*advance_c<3>(begin(t)) == at_c<3>(t));

#if !defined(FUSION_NO_PRIOR)
        BOOST_TEST(*advance_c<-1>(end(t)) == at_c<3>(t));
        BOOST_TEST(*advance_c<-2>(end(t)) == at_c<2>(t));
        BOOST_TEST(*advance_c<-3>(end(t)) == at_c<1>(t));
        BOOST_TEST(*advance_c<-4>(end(t)) == at_c<0>(t));
#endif

        BOOST_TEST(&*advance_c<0>(begin(t)) == &at_c<0>(t));
        BOOST_TEST(&*advance_c<1>(begin(t)) == &at_c<1>(t));
        BOOST_TEST(&*advance_c<2>(begin(t)) == &at_c<2>(t));
        BOOST_TEST(&*advance_c<3>(begin(t)) == &at_c<3>(t));
    }
}




