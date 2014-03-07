
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/function.hpp>
#include <boost/detail/lightweight_test.hpp>

void intermediate(boost::function<void (int, int)> store_func, int size) {
    store_func(size - 1, -1);
}

void hack(int* array, int size) {
    void BOOST_LOCAL_FUNCTION(bind array, int index, int value) {
        array[index] = value;
    } BOOST_LOCAL_FUNCTION_NAME(store)
    
    intermediate(store, size);
}

int main(void) {
    int nums[] = {1, 2, 3};
    hack(nums, 3);

    BOOST_TEST(nums[0] == 1);
    BOOST_TEST(nums[1] == 2);
    BOOST_TEST(nums[2] == -1);
    return boost::report_errors();
}

