
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/detail/lightweight_test.hpp>
#include <algorithm>

//[add_global_functor
// Unfortunately, cannot be defined locally (so not a real alternative).
struct global_add { // Unfortunately, boilerplate code to program the class.
    global_add(int& _sum, int _factor): sum(_sum), factor(_factor) {}

    inline void operator()(int num) { // Body uses C++ statement syntax.
        sum += factor * num;
    }

private: // Unfortunately, cannot bind so repeat variable types.
    int& sum; // Access `sum` by reference.
    const int factor; // Make `factor` constant.
};

int main(void) {
    int sum = 0, factor = 10;

    global_add add(sum, factor);

    add(1);
    int nums[] = {2, 3};
    std::for_each(nums, nums + 2, add); // Passed as template parameter.

    BOOST_TEST(sum == 60);
    return boost::report_errors();
}
//]

