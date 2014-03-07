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
#include <boost/fusion/algorithm/transformation/remove.hpp>
#include <boost/mpl/vector.hpp>

struct X
{
    operator char const*() const
    {
        return "<X-object>";
    }
};

struct Y
{
    operator char const*() const
    {
        return "<Y-object>";
    }
};

int
main()
{
    using namespace boost::fusion;
    using boost::mpl::identity;
    using boost::mpl::vector;
    namespace fusion = boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing remove

    X x; Y y;
    typedef fusion::vector<Y, char, long, X, bool, double> vector_type;
    vector_type t(y, '@', 987654, x, true, 6.6);

    {
        std::cout << fusion::remove<X>(t) << std::endl;
        BOOST_TEST((fusion::remove<X>(t)
            == make_vector(y, '@', 987654, true, 6.6)));
    }

    {
        std::cout << fusion::remove<Y>(t) << std::endl;
        BOOST_TEST((fusion::remove<Y>(t)
            == make_vector('@', 987654, x, true, 6.6)));
    }

    {
        std::cout << fusion::remove<long>(t) << std::endl;
        BOOST_TEST((fusion::remove<long>(t)
            == make_vector(y, '@', x, true, 6.6)));
    }

    {
        typedef vector<Y, char, long, X, bool> mpl_vec;
        BOOST_TEST((fusion::remove<X>(mpl_vec())
            == vector<Y, char, long, bool>()));
        BOOST_TEST((fusion::remove<Y>(mpl_vec())
            == vector<char, long, X, bool>()));
        BOOST_TEST((fusion::remove<long>(mpl_vec())
            == vector<Y, char, X, bool>()));
    }

    return boost::report_errors();
}

