/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>

namespace phoenix = boost::phoenix;

int
main()
{
    //using phoenix::if_else;
    using phoenix::ref;
    using phoenix::val;
    using phoenix::arg_names::arg1;
    {
        int x = 0;
        int y = 0;
        bool c = false;
        
        BOOST_TEST(phoenix::if_else(arg1, 1234, 5678)(c) == 5678);
        BOOST_TEST(phoenix::if_else(arg1, 1234, 'x')(c) == 'x');
        
        int& r = phoenix::if_else(arg1, ref(x), ref(y))(c); // should be an lvalue
        BOOST_TEST(&y == &r);
        
        (phoenix::if_else(arg1, ref(x), ref(y)) = 986754321)(c);
        BOOST_TEST(y == 986754321);
    }
    
    return boost::report_errors();
}
