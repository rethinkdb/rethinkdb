
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifndef __GNUC__
#   error "GCC required (using non-standard GCC statement expressions)"
#else

#include "gcc_lambda.hpp"
#include <boost/detail/lightweight_test.hpp>
#include <algorithm>

int main(void) {
    //[gcc_lambda
    int val = 2;
    int nums[] = {1, 2, 3};
    int* end = nums + 3;

    int* iter = std::find_if(nums, end,
        GCC_LAMBDA(const bind val, int num, return bool) {
            return num == val;
        } GCC_LAMBDA_END
    );
    //]

    BOOST_TEST(iter != end);
    BOOST_TEST(*iter == val);
    return boost::report_errors();
}

#endif // GCC

