
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
#include <vector>
#include <algorithm>

struct adder;
BOOST_TYPEOF_REGISTER_TYPE(adder) // Register before `bind this_` below.

//[add_this
struct adder {
    adder() : sum_(0) {}

    int sum(const std::vector<int>& nums, const int factor = 10) {

        void BOOST_LOCAL_FUNCTION(const bind factor, bind this_, int num) {
            this_->sum_ += factor * num; // Use `this_` instead of `this`.
        } BOOST_LOCAL_FUNCTION_NAME(add)
        
        std::for_each(nums.begin(), nums.end(), add);
        return sum_;
    }

private:
    int sum_;
};
//]

int main(void) {
    std::vector<int> v(3);
    v[0] = 1; v[1] = 2; v[2] = 3;

    BOOST_TEST(adder().sum(v) == 60);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

