
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <boost/detail/lightweight_test.hpp>
#include <vector>
#include <algorithm>

struct v;
BOOST_TYPEOF_REGISTER_TYPE(v) // Register before `bind this_` below.

struct v {
    std::vector<int> nums;
    
    v(const std::vector<int>& numbers): nums(numbers) {}

    void change_sign_all(const std::vector<int>& indices) {
        void BOOST_LOCAL_FUNCTION(bind this_, int i) { // Bind object `this`.
            this_->nums.at(i) = -this_->nums.at(i);
        } BOOST_LOCAL_FUNCTION_NAME(complement)

        std::for_each(indices.begin(), indices.end(), complement);
    }
};

int main(void) {
    std::vector<int> n(3);
    n[0] = 1; n[1] = 2; n[2] = 3;

    std::vector<int> i(2);
    i[0] = 0; i[1] = 2; // Will change n[0] and n[2] but not n[1].
    
    v vn(n);
    vn.change_sign_all(i);
    
    BOOST_TEST(vn.nums.at(0) == -1);
    BOOST_TEST(vn.nums.at(1) == 2);
    BOOST_TEST(vn.nums.at(2) == -3);
    return boost::report_errors();
}

