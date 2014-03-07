
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
#include <boost/preprocessor/cat.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>
    
//[same_line
#define LOCAL_INC_DEC(offset) \
    int BOOST_LOCAL_FUNCTION_ID(BOOST_PP_CAT(inc, __LINE__), /* unique ID */ \
            const bind offset, const int x) { \
        return x + offset; \
    } BOOST_LOCAL_FUNCTION_NAME(inc) \
    \
    int BOOST_LOCAL_FUNCTION_ID(BOOST_PP_CAT(dec, __LINE__), \
            const bind offset, const int x) { \
        return x - offset; \
    } BOOST_LOCAL_FUNCTION_NAME(dec)

#define LOCAL_INC_DEC_TPL(offset) \
    T BOOST_LOCAL_FUNCTION_ID_TPL(BOOST_PP_CAT(inc, __LINE__), \
            const bind offset, const T x) { \
        return x + offset; \
    } BOOST_LOCAL_FUNCTION_NAME_TPL(inc) \
    \
    T BOOST_LOCAL_FUNCTION_ID_TPL(BOOST_PP_CAT(dec, __LINE__), \
            const bind offset, const T x) { \
        return x - offset; \
    } BOOST_LOCAL_FUNCTION_NAME_TPL(dec)

template<typename T>
void f(T& delta) {
    LOCAL_INC_DEC_TPL(delta) // Multiple local functions on same line.
    BOOST_TEST(dec(inc(123)) == 123);
}

int main(void) {
    int delta = 10;
    LOCAL_INC_DEC(delta) // Multiple local functions on same line.
    BOOST_TEST(dec(inc(123)) == 123);
    f(delta);
    return boost::report_errors();
}
//]

#endif // VARIADIC_MACROS

