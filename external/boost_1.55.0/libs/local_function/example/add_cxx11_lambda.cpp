
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_LAMBDAS
#   error "lambda functions required"
#else

#include <boost/detail/lightweight_test.hpp>
#include <algorithm>

//[add_cxx11_lambda
int main(void) {                            // Some local scope.
    int sum = 0, factor = 10;               // Variables in scope to bind.
    
    auto add = [factor, &sum](int num) {    // C++11 only.
        sum += factor * num;
    };
    
    add(1);                                 // Call the lambda.
    int nums[] = {2, 3};
    std::for_each(nums, nums + 2, add);     // Pass it to an algorithm.
    
    BOOST_TEST(sum == 60);                  // Assert final summation value.
    return boost::report_errors();
}
//]

#endif

