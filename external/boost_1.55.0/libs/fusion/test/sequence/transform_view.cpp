/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/view/transform_view/transform_view.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/prior.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/distance.hpp>

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

struct square
{
    template<typename T>
    struct result;

    template <typename T>
    struct result<square(T)>
    {
        typedef int type;
    };

    template <typename T>
    int operator()(T x) const
    {
        return x * x;
    }
};

struct add
{
    template<typename T>
    struct result;

    template <typename A, typename B>
    struct result<add(A,B)>
    {
        typedef int type;
    };

    template <typename A, typename B>
    int operator()(A a, B b) const
    {
        return a + b;
    }
};

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing the transform_view

    {
        typedef boost::mpl::range_c<int, 5, 9> sequence_type;
        sequence_type sequence;
        square sq;
        typedef transform_view<sequence_type, square> xform_type;
        xform_type xform(sequence, sq);

        std::cout << xform << std::endl;
        BOOST_TEST((xform == make_vector(25, 36, 49, 64)));

        typedef boost::fusion::result_of::begin<xform_type>::type first_type;
        first_type first_it(boost::fusion::begin(xform));

        typedef boost::fusion::result_of::next<first_type>::type next_type;
        next_type next_it(boost::fusion::next(first_it));
        BOOST_TEST((*next_it == 36));
        BOOST_TEST((*boost::fusion::prior(next_it) == 25));
        BOOST_TEST((boost::fusion::distance(first_it, next_it) == 1));

        BOOST_TEST((*boost::fusion::advance_c<3>(boost::fusion::begin(xform)) == 64));
        BOOST_TEST((boost::fusion::at_c<2>(xform) == 49));
        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::value_at_c<xform_type, 0>::type, int>));
    }
    
    {
        typedef boost::mpl::range_c<int, 5, 9> sequence1_type;
        typedef boost::mpl::range_c<int, 10, 14> sequence2_type;
        sequence1_type sequence1;
        sequence2_type sequence2;
        add f;
        typedef transform_view<sequence1_type, sequence2_type, add> xform_type;
        xform_type xform(sequence1, sequence2, f);

        std::cout << xform << std::endl;
        BOOST_TEST((xform == make_vector(15, 17, 19, 21)));
        BOOST_TEST((boost::fusion::at_c<2>(xform) == 19));
        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::value_at_c<xform_type, 0>::type, int>));
    }

    return boost::report_errors();
}

