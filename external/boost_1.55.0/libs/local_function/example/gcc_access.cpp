
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/detail/lightweight_test.hpp>

int main(void) {
    int nums[] = {1, 2, 3};
    int offset = -1;
    int BOOST_LOCAL_FUNCTION(const bind offset, int* array, int index) {
        return array[index + offset];
    } BOOST_LOCAL_FUNCTION_NAME(access)

    BOOST_TEST(access(nums, 1) == 1);
    BOOST_TEST(access(nums, 2) == 2);
    BOOST_TEST(access(nums, 3) == 3);
    return boost::report_errors();
}

