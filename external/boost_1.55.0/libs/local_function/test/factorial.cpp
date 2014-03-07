
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
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <boost/detail/lightweight_test.hpp>
#include <algorithm>
#include <vector>

struct calculator;
BOOST_TYPEOF_REGISTER_TYPE(calculator) // Register before `bind this_` below.

//[factorial
struct calculator {
    std::vector<int> results;

    void factorials(const std::vector<int>& nums) {
        int BOOST_LOCAL_FUNCTION(bind this_, int num,
                bool recursion, default false) {
            int result = 0;
            
            if(num <= 0) result = 1;
            else result = num * factorial(num - 1, true); // Recursive call.

            if(!recursion) this_->results.push_back(result);
            return result;
        } BOOST_LOCAL_FUNCTION_NAME(recursive factorial) // Recursive.
    
        std::for_each(nums.begin(), nums.end(), factorial);
    }
};
//]

int main(void) {
    std::vector<int> v(3);
    v[0] = 1; v[1] = 3; v[2] = 4;

    calculator calc;
    calc.factorials(v);
    BOOST_TEST(calc.results[0] == 1);
    BOOST_TEST(calc.results[1] == 6);
    BOOST_TEST(calc.results[2] == 24);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

