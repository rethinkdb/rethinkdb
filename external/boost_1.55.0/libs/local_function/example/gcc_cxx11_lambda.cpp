
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

int main(void) {
    //[gcc_cxx11_lambda
    int val = 2;
    int nums[] = {1, 2, 3};
    int* end = nums + 3;

    int* iter = std::find_if(nums, end, 
        [val](int num) -> bool {
            return num == val;
        }
    );
    //]

    BOOST_TEST(iter != end);
    BOOST_TEST(*iter == val);
    return boost::report_errors();
}

#endif // LAMBDAS

