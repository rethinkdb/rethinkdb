
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
#include <boost/detail/lightweight_test.hpp>
#include <vector>
#include <algorithm>

int main(void) {
    //[add_inline
    int sum = 0, factor = 10;

    void BOOST_LOCAL_FUNCTION(const bind factor, bind& sum, int num) {
        sum += factor * num;
    } BOOST_LOCAL_FUNCTION_NAME(inline add) // Inlining.

    std::vector<int> v(100);
    std::fill(v.begin(), v.end(), 1);

    for(size_t i = 0; i < v.size(); ++i) add(v[i]); // Cannot use for_each.
    //]

    BOOST_TEST(sum == 1000);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

