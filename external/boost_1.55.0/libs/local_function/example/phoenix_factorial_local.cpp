
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/function.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/detail/lightweight_test.hpp>

//[phoenix_factorial_local
int main(void) {
    using boost::phoenix::arg_names::arg1;
    
    int BOOST_LOCAL_FUNCTION(int n) { // Unfortunately, monomorphic.
        return (n <= 0) ? 1 : n * factorial_impl(n - 1);
    } BOOST_LOCAL_FUNCTION_NAME(recursive factorial_impl)

    boost::phoenix::function< boost::function<int (int)> >
            factorial(factorial_impl); // Phoenix function from local function.
    
    int i = 4;
    BOOST_TEST(factorial(i)() == 24);      // Call.
    BOOST_TEST(factorial(arg1)(i) == 24);  // Lazy call.
    return boost::report_errors();
}
//]

