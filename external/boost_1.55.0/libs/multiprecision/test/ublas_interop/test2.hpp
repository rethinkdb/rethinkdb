//
//  Copyright (c) 2000-2002
//  Joerg Walter, Mathias Koch
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#ifndef TEST2_H
#define TEST2_H
#ifdef _MSC_VER
#  pragma warning(disable:4800 4996 4127 4100 4018)
#endif

#include <boost/multiprecision/cpp_dec_float.hpp>

#ifdef TEST_ET
typedef boost::multiprecision::number<boost::multiprecision::cpp_dec_float<50>, boost::multiprecision::et_on> mp_test_type;
#else
typedef boost::multiprecision::number<boost::multiprecision::cpp_dec_float<50>, boost::multiprecision::et_off> mp_test_type;
#endif
//typedef double mp_test_type;

#define USE_RANGE
#define USE_SLICE
#define USE_FLOAT
#define USE_UNBOUNDED_ARRAY 
#define USE_STD_VECTOR 
#define USE_BOUNDED_VECTOR USE_MATRIX
#define USE_UNBOUNDED_ARRAY
#define USE_MAP_ARRAY 
#define USE_STD_MAP
#define USE_MAPPED_VECTOR 
#define USE_COMPRESSED_VECTOR 
#define USE_COORDINATE_VECTOR
#define USE_MAPPED_MATRIX 
#define USE_COMPRESSED_MATRIX 
#define USE_COORDINATE_MATRIX

#include <iostream>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/blas.hpp>

namespace ublas = boost::numeric::ublas;

#include "common/init.hpp"

template<class V, int N>
struct test_blas_1 {
    typedef typename V::value_type value_type;
    typedef typename ublas::type_traits<value_type>::real_type real_type;

    void test ();
};

template<class V, class M, int N>
struct test_blas_2 {
    typedef typename V::value_type value_type;

    void test ();
};

template<class M, int N>
struct test_blas_3 {
    typedef typename M::value_type value_type;

    void test ();
};


#endif
