// Copyright 2010 Gunter Winkler <guwi17@gmx.de>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <complex>

#include "libs/numeric/ublas/test/utils.hpp"

using namespace boost::numeric::ublas;

static const double TOL(1.0e-5); ///< Used for comparing two real numbers.

BOOST_UBLAS_TEST_DEF ( test_double_complex_norm_inf ) {
    typedef std::complex<double> dComplex;
    vector<dComplex> v(4);
    for (unsigned int i = 0; i < v.size(); ++i)
        v[i] = dComplex(i, i + 1);

    const double expected = abs(v[3]);

    BOOST_UBLAS_DEBUG_TRACE( "norm is " << norm_inf(v) );
    BOOST_UBLAS_TEST_CHECK(std::abs(norm_inf(v) - expected) < TOL);
    v *= 3.;
    BOOST_UBLAS_TEST_CHECK(std::abs(norm_inf(v) - (3.0*expected)) < TOL);
}

BOOST_UBLAS_TEST_DEF ( test_double_complex_norm_2 ) {
    typedef std::complex<double> dComplex;
    vector<dComplex> v(4);
    for (unsigned int i = 0; i < v.size(); ++i)
        v[i] = dComplex(i, i + 1);

    const double expected = sqrt(44.0);

    BOOST_UBLAS_DEBUG_TRACE( "norm is " << norm_2(v) );
    BOOST_UBLAS_TEST_CHECK(std::abs(norm_2(v) - expected) < TOL);
    v *= 3.;
    BOOST_UBLAS_TEST_CHECK(std::abs(norm_2(v) - (3.0*expected)) < TOL);
}

BOOST_UBLAS_TEST_DEF ( test_float_complex_norm_inf ) {
    typedef std::complex<float> dComplex;
    vector<dComplex> v(4);
    for (unsigned int i = 0; i < v.size(); ++i)
        v[i] = dComplex(i, i + 1);

    const float expected = abs(v[3]);

    BOOST_UBLAS_DEBUG_TRACE( "norm is " << norm_inf(v) );
    BOOST_UBLAS_TEST_CHECK(std::abs(norm_inf(v) - expected) < TOL);
    v *= 3.;
    BOOST_UBLAS_TEST_CHECK(std::abs(norm_inf(v) - (3.0*expected)) < TOL);
}

BOOST_UBLAS_TEST_DEF ( test_float_complex_norm_2 ) {
    typedef std::complex<float> dComplex;
    vector<dComplex> v(4);
    for (unsigned int i = 0; i < v.size(); ++i)
        v[i] = dComplex(i, i + 1);

    const float expected = sqrt(44.0);

    BOOST_UBLAS_DEBUG_TRACE( "norm is " << norm_2(v) );
    BOOST_UBLAS_TEST_CHECK(std::abs(norm_2(v) - expected) < TOL);
    v *= 3.;
    BOOST_UBLAS_TEST_CHECK(std::abs(norm_2(v) - (3.0*expected)) < TOL);
}

int main() {
    BOOST_UBLAS_TEST_BEGIN();

    BOOST_UBLAS_TEST_DO( test_double_complex_norm_inf );
    BOOST_UBLAS_TEST_DO( test_float_complex_norm_inf );
    BOOST_UBLAS_TEST_DO( test_double_complex_norm_2 );
    BOOST_UBLAS_TEST_DO( test_float_complex_norm_2 );

    BOOST_UBLAS_TEST_END();
}
