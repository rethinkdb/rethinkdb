
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef SCOPE_EXIT_HPP_
#define SCOPE_EXIT_HPP_

#include <boost/local_function.hpp>
#include <boost/local_function/detail/preprocessor/line_counter.hpp>
#include <boost/function.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/config.hpp>

//[scope_exit_class
struct scope_exit {
    scope_exit(boost::function<void (void)> f): f_(f) {}
    ~scope_exit(void) { f_(); }
private:
    boost::function<void (void)> f_;
};
//]

// PRIVATE //

//[scope_exit_end_macro
#define SCOPE_EXIT_END_(id) \
    BOOST_LOCAL_FUNCTION_NAME(BOOST_PP_CAT(scope_exit_func_, id)) \
    scope_exit BOOST_PP_CAT(scope_exit_, id)( \
            BOOST_PP_CAT(scope_exit_func_, id));
//]

// PUBLIC //

#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   define SCOPE_EXIT(void_or_seq) \
        void BOOST_LOCAL_FUNCTION(void_or_seq)
#else
//[scope_exit_macro
#   define SCOPE_EXIT(...) \
        void BOOST_LOCAL_FUNCTION(__VA_ARGS__)
//]
#endif

#define SCOPE_EXIT_END \
    SCOPE_EXIT_END_(BOOST_LOCAL_FUNCTION_DETAIL_PP_LINE_COUNTER)

#endif // #include guard

