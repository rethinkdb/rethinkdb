
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   error "variadic macros required"
#else

#include <boost/local_function.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <algorithm>

//[add
int main(void) {                            // Some local scope.
    int sum = 0, factor = 10;               // Variables in scope to bind.
    
    void BOOST_LOCAL_FUNCTION(const bind factor, bind& sum, int num) {
        sum += factor * num;
    } BOOST_LOCAL_FUNCTION_NAME(add)
    
    add(1);                                 // Call the local function.
    int nums[] = {2, 3};
    std::for_each(nums, nums + 2, add);     // Pass it to an algorithm.

    BOOST_TEST(sum == 60);                  // Assert final summation value.
    return boost::report_errors();
}
//]

#endif // VARIADIC_MACROS

