// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief test_limits.cpp

\details
Test numeric_limits specialization.

Output:
@verbatim
@endverbatim
**/

#include <complex>
#include <limits>

#include <boost/units/limits.hpp>
#include <boost/units/cmath.hpp>

#include "test_header.hpp"

typedef boost::units::length unit_type;
using boost::units::quantity;

template<bool>
struct check_quiet_NaN;

template<>
struct check_quiet_NaN<true> {
    template<class T>
    static void apply() {
        quantity<unit_type, T> q((std::numeric_limits<quantity<unit_type, T> >::quiet_NaN)());
        bool test = isnan BOOST_PREVENT_MACRO_SUBSTITUTION (q);
        BOOST_CHECK(test);
    }
};

template<>
struct check_quiet_NaN<false> {
    template<class T>
    static void apply() {}
};

template<bool>
struct check_signaling_NaN;

template<>
struct check_signaling_NaN<true> {
    template<class T>
    static void apply() {
        quantity<unit_type, T> q((std::numeric_limits<quantity<unit_type, T> >::signaling_NaN)());
        bool test = isnan BOOST_PREVENT_MACRO_SUBSTITUTION (q);
        BOOST_CHECK(test);
    }
};

template<>
struct check_signaling_NaN<false> {
    template<class T>
    static void apply() {}
};

template<class T>
void do_check() {
    #define CHECK_FUNCTION(name) BOOST_CHECK(((std::numeric_limits<T>::name)() == (std::numeric_limits<quantity<unit_type, T> >::name)().value()))
    #define CHECK_CONSTANT(name) BOOST_CHECK((std::numeric_limits<T>::name == std::numeric_limits<quantity<unit_type, T> >::name))
    CHECK_FUNCTION(min);
    CHECK_FUNCTION(max);
    CHECK_FUNCTION(epsilon);
    CHECK_FUNCTION(round_error);
    CHECK_FUNCTION(infinity);
    CHECK_FUNCTION(denorm_min);

    CHECK_CONSTANT(is_specialized);
    CHECK_CONSTANT(digits);
    CHECK_CONSTANT(digits10);
    CHECK_CONSTANT(is_signed);
    CHECK_CONSTANT(is_integer);
    CHECK_CONSTANT(is_exact);
    CHECK_CONSTANT(radix);
    CHECK_CONSTANT(min_exponent);
    CHECK_CONSTANT(min_exponent10);
    CHECK_CONSTANT(max_exponent);
    CHECK_CONSTANT(max_exponent10);
    CHECK_CONSTANT(has_infinity);
    CHECK_CONSTANT(has_quiet_NaN);
    CHECK_CONSTANT(has_signaling_NaN);
    CHECK_CONSTANT(has_denorm);
    CHECK_CONSTANT(has_denorm_loss);
    CHECK_CONSTANT(is_iec559);
    CHECK_CONSTANT(is_bounded);
    CHECK_CONSTANT(is_modulo);
    CHECK_CONSTANT(traps);
    CHECK_CONSTANT(tinyness_before);
    CHECK_CONSTANT(round_style);

    check_quiet_NaN<std::numeric_limits<quantity<unit_type, T> >::has_quiet_NaN>::template apply<T>();
    check_signaling_NaN<std::numeric_limits<quantity<unit_type, T> >::has_signaling_NaN>::template apply<T>();
}

int test_main(int,char *[])
{
    do_check<float>();
    do_check<double>();
    do_check<int>();
    do_check<long>();
    do_check<unsigned>();
    do_check<std::complex<double> >();

    return(0);
}
