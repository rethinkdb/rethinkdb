/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/vector/vector_iterator.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/algorithm/transformation/erase.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/int.hpp>

int
main()
{
    using namespace boost::fusion;
    using boost::mpl::vector_c;
    using boost::mpl::begin;
    using boost::mpl::advance;
    using boost::mpl::int_;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing erase

    {
        typedef vector<int, char, double, char const*> vector_type;
        vector_type t1(1, 'x', 3.3, "Ruby");
        vector_iterator<vector_type, 2> pos(t1);

        std::cout << erase(t1, pos) << std::endl;
        BOOST_TEST((erase(t1, pos) == make_vector(1, 'x', std::string("Ruby"))));
        BOOST_TEST((erase(t1, end(t1)) == make_vector(1, 'x', 3.3, std::string("Ruby"))));
    }

    {
        typedef vector_c<int, 1, 2, 3, 4, 5> mpl_vec;
        typedef boost::mpl::begin<mpl_vec>::type mpl_vec_begin;
        typedef boost::mpl::advance<mpl_vec_begin, int_<3> >::type mpl_vec_at3;
        typedef boost::mpl::next<mpl_vec_begin>::type n1;
        typedef boost::mpl::next<n1>::type n2;
        typedef boost::mpl::next<n2>::type n3;

        BOOST_STATIC_ASSERT((boost::is_same<mpl_vec_at3, n3>::value));
        
        
        std::cout << erase(mpl_vec(), mpl_vec_at3()) << std::endl;
        BOOST_TEST((erase(mpl_vec(), mpl_vec_at3())
            == make_vector(1, 2, 3, 5)));
    }

    return boost::report_errors();
}

