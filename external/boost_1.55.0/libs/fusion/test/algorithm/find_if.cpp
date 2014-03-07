/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/algorithm/query/find_if.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/less.hpp>
#include <boost/type_traits/is_same.hpp>

struct X
{
    operator int() const
    {
        return 12345;
    }
};

int
main()
{
    using namespace boost::fusion;

    {
        using boost::is_same;
        using boost::mpl::_;

        typedef vector<int, char, int, double> vector_type;
        vector_type v(12345, 'x', 678910, 3.36);

        std::cout << *find_if<is_same<_, char> >(v) << std::endl;
        BOOST_TEST((*find_if<is_same<_, char> >(v) == 'x'));

        std::cout << *find_if<is_same<_, int> >(v) << std::endl;
        BOOST_TEST((*find_if<is_same<_, int> >(v) == 12345));

        std::cout << *find_if<is_same<_, double> >(v) << std::endl;
        BOOST_TEST((*find_if<is_same<_, double> >(v) == 3.36));
    }

    {
        using boost::mpl::vector;
        using boost::is_same;
        using boost::mpl::_;

        typedef vector<int, char, X, double> mpl_vec;
        BOOST_TEST((*find_if<is_same<_, X> >(mpl_vec()) == 12345));
    }

    {
        using boost::mpl::vector_c;
        using boost::mpl::less;
        using boost::mpl::int_;
        using boost::is_same;
        using boost::mpl::_;

        typedef vector_c<int, 1, 2, 3, 4> mpl_vec;
        BOOST_TEST((*find_if<less<_, int_<3> > >(mpl_vec()) == 1));
    }

    return boost::report_errors();
}

