
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   error "variadic macros required"
#else

#include "addable.hpp"
#include <boost/local_function.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/concept_check.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <algorithm>

//[typeof_template
template<typename T>
T calculate(const T& factor) {
    T sum = 0;

    void BOOST_LOCAL_FUNCTION_TPL(const bind factor, bind& sum, T num) {
        // Local function `TYPEOF` does not need `typename`.
        BOOST_CONCEPT_ASSERT((Addable<typename boost::remove_reference<
                BOOST_LOCAL_FUNCTION_TYPEOF(sum)>::type>));
        sum += factor * num;
    } BOOST_LOCAL_FUNCTION_NAME_TPL(add)

    add(6);
    return sum;
}
//]

int main(void) {
    BOOST_TEST(calculate(10) == 60);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

