
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
#include <algorithm>

//[add_template
template<typename T>
T total(const T& x, const T& y, const T& z) {
    T sum = T(), factor = 10;

    // Must use the `..._TPL` macros within templates.
    T BOOST_LOCAL_FUNCTION_TPL(const bind factor, bind& sum, T num) {
        return sum += factor * num;
    } BOOST_LOCAL_FUNCTION_NAME_TPL(add)

    add(x);
    T nums[2]; nums[0] = y; nums[1] = z;
    std::for_each(nums, nums + 2, add);

    return sum;
}
//]

int main(void) {
    BOOST_TEST(total(1, 2, 3) == 60);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

