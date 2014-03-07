/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/algorithm/transformation/insert.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/int.hpp>
#include <string>

int
main()
{
    using namespace boost::fusion;
    using boost::mpl::vector_c;
    using boost::mpl::advance;
    using boost::mpl::int_;
    namespace fusion = boost::fusion;
    namespace mpl = boost::mpl;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing insert

    {
        char const* s = "Ruby";
        typedef vector<int, char, double, char const*> vector_type;
        vector_type t1(1, 'x', 3.3, s);
        vector_iterator<vector_type, 2> pos(t1);

        std::cout << insert(t1, pos, 123456) << std::endl;
        BOOST_TEST((insert(t1, pos, 123456)
            == make_vector(1, 'x', 123456, 3.3, s)));

        std::cout << insert(t1, end(t1), 123456) << std::endl;
        BOOST_TEST((insert(t1, end(t1), 123456)
            == make_vector(1, 'x', 3.3, s, 123456)));

        std::cout << insert(t1, begin(t1), "glad") << std::endl;
        BOOST_TEST((insert(t1, begin(t1), "glad")
            == make_vector(std::string("glad"), 1, 'x', 3.3, s)));
    }

    {
        typedef vector_c<int, 1, 2, 3, 4, 5> mpl_vec;
        typedef mpl::begin<mpl_vec>::type mpl_vec_begin;
        typedef advance<mpl_vec_begin, int_<3> >::type mpl_vec_at3;

        std::cout << fusion::insert(mpl_vec(), mpl_vec_at3(), int_<66>()) << std::endl;
        BOOST_TEST((fusion::insert(mpl_vec(), mpl_vec_at3(), int_<66>())
            == make_vector(1, 2, 3, 66, 4, 5)));
    }

    return boost::report_errors();
}

