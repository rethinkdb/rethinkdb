/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_numeric.hpp>

#include "test.hpp"

int
main()
{
    using spirit_test::test_attr;
    using boost::spirit::qi::float_;
    
    std::cerr.precision(9);

    float f = 0.0f;

    // this should pass, but currently doesn't because of the way the real 
    // parser handles the fractional part of a number
    BOOST_TEST(test_attr("123233.4124", float_, f));
    BOOST_TEST_EQ(f, 123233.4124f); 
    BOOST_TEST(test_attr("987654.3219", float_, f));
    BOOST_TEST_EQ(f, 987654.3219f); 

    return boost::report_errors();
}

