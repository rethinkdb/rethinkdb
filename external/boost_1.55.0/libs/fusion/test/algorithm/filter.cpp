/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/algorithm/transformation/filter.hpp>
#include <boost/mpl/vector.hpp>

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    typedef boost::fusion::vector<char,double,char> vector_type;
    vector_type t('a', 6.6, 'b');

    {
        std::cout << filter<char>(t) << std::endl;
        BOOST_TEST((filter<char>(t)
            == make_vector('a', 'b')));
    }

    {
        typedef boost::mpl::vector<char,double,char> mpl_vec;
        BOOST_TEST((filter<char>(mpl_vec())
            == make_vector('\0', '\0')));
    }

    return boost::report_errors();
}

