
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/spirit/include/phoenix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <algorithm>
#include <iostream>
    
//[add_phoenix
int main(void) {
    using boost::phoenix::let;
    using boost::phoenix::local_names::_f;
    using boost::phoenix::cref;
    using boost::phoenix::ref;
    using boost::phoenix::arg_names::_1;

    int sum = 0, factor = 10;
    int nums[] = {1, 2, 3};

    // Passed to template, `factor` by constant, and defined in expression.
    std::for_each(nums, nums + 3, let(_f = cref(factor))[
        // Unfortunately, body cannot use C++ statement syntax.
        ref(sum) += _f * _1, _1 // Access `sum` by reference.
    ]);

    BOOST_TEST(sum == 60);
    return boost::report_errors();
}
//]

