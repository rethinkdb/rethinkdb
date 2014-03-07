/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/view/iterator_range/iterator_range.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/has_key.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/key_of.hpp>
#include <boost/fusion/iterator/value_of_data.hpp>
#include <boost/fusion/iterator/deref_data.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/static_assert.hpp>

int
main()
{
    using namespace boost::fusion;
    namespace fusion = boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        char const* s = "Ruby";
        typedef vector<int, char, double, char const*> vector_type;
        vector_type vec(1, 'x', 3.3, s);

        {
            typedef vector_iterator<vector_type, 1> i1t;
            typedef vector_iterator<vector_type, 3> i3t;

            i1t i1(vec);
            i3t i3(vec);

            typedef iterator_range<i1t, i3t> slice_t;
            slice_t slice(i1, i3);
            std::cout << slice << std::endl;
            BOOST_TEST((slice == make_vector('x', 3.3)));
            BOOST_STATIC_ASSERT(boost::fusion::result_of::size<slice_t>::value == 2);
        }

        {
            typedef vector_iterator<vector_type, 0> i1t;
            typedef vector_iterator<vector_type, 0> i3t;

            i1t i1(vec);
            i3t i3(vec);

            typedef iterator_range<i1t, i3t> slice_t;
            slice_t slice(i1, i3);
            std::cout << slice << std::endl;
            BOOST_TEST(slice == make_vector());
            BOOST_STATIC_ASSERT(boost::fusion::result_of::size<slice_t>::value == 0);
        }
    }

    {
        typedef boost::mpl::vector_c<int, 2, 3, 4, 5, 6> mpl_vec;
        typedef boost::mpl::begin<mpl_vec>::type it0;
        typedef boost::mpl::next<it0>::type it1;
        typedef boost::mpl::next<it1>::type it2;
        typedef boost::mpl::next<it2>::type it3;

        it1 f;
        it3 l;
        
        typedef iterator_range<it1, it3> slice_t;
        slice_t slice(f, l);
        std::cout << slice << std::endl;
        BOOST_TEST((slice == make_vector(3, 4)));
        BOOST_STATIC_ASSERT(boost::fusion::result_of::size<slice_t>::value == 2);
    }

    {
        typedef map<pair<void,std::string>, pair<double,char>,pair<void*, int> > map_type;
        map_type m(make_pair<void>("foo"), make_pair<double>('x'), make_pair<void*>(2));

        typedef iterator_range<
            boost::fusion::result_of::begin<map_type>::type
          , boost::fusion::result_of::advance_c<boost::fusion::result_of::begin<map_type>::type,2>::type
        > range_type;
        range_type r(begin(m), advance_c<2>(begin(m)));

        BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<range_type, void>::type));
        BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<range_type, double>::type));

        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::key_of<boost::fusion::result_of::begin<range_type>::type>::type, void>));
        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::key_of<boost::fusion::result_of::next<boost::fusion::result_of::begin<range_type>::type>::type>::type, double>));

        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::value_of_data<boost::fusion::result_of::begin<range_type>::type>::type, std::string>));
        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::value_of_data<boost::fusion::result_of::next<boost::fusion::result_of::begin<range_type>::type>::type>::type, char>));

        std::cout << deref_data(begin(r)) << std::endl;
        std::cout << deref_data(fusion::next(begin(r))) << std::endl;
        BOOST_TEST((deref_data(begin(r)) == "foo"));
        BOOST_TEST((deref_data(fusion::next(begin(r))) == 'x'));
    }

    return boost::report_errors();
}

