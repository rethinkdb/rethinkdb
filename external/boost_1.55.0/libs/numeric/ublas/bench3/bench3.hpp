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

#ifndef BENCH3_H
#define BENCH3_H

#include <iostream>
#include <string>
#include <valarray>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>

#include <boost/timer.hpp>

namespace ublas = boost::numeric::ublas;

void header (std::string text);

template<class T>
struct footer {
    void operator () (int multiplies, int plus, int runs, double elapsed) {
        std::cout << "elapsed: " << elapsed << " s, "
                  << (multiplies * ublas::type_traits<T>::multiplies_complexity +
                      plus * ublas::type_traits<T>::plus_complexity) * runs /
                     (1024 * 1024 * elapsed) << " Mflops" << std::endl;
    }
};

template<class T, int N>
struct c_vector_traits {
    typedef T type [N];
};
template<class T, int N, int M>
struct c_matrix_traits {
    typedef T type [N] [M];
};

template<class T, int N>
struct initialize_c_vector  {
    void operator () (typename c_vector_traits<T, N>::type &v) {
        for (int i = 0; i < N; ++ i)
            v [i] = std::rand () * 1.f;
//            v [i] = 0.f;
        }
};
template<class V>
BOOST_UBLAS_INLINE
void initialize_vector (V &v) {
    int size = v.size ();
    for (int i = 0; i < size; ++ i)
        v [i] = std::rand () * 1.f;
//      v [i] = 0.f;
}

template<class T, int N, int M>
struct initialize_c_matrix  {
    void operator () (typename c_matrix_traits<T, N, M>::type &m) {
        for (int i = 0; i < N; ++ i)
            for (int j = 0; j < M; ++ j)
                m [i] [j] = std::rand () * 1.f;
//                m [i] [j] = 0.f;
    }
};
template<class M>
BOOST_UBLAS_INLINE
void initialize_matrix (M &m) {
    int size1 = m.size1 ();
    int size2 = m.size2 ();
    for (int i = 0; i < size1; ++ i)
        for (int j = 0; j < size2; ++ j)
            m (i, j) = std::rand () * 1.f;
//            m (i, j) = 0.f;
}

template<class T>
BOOST_UBLAS_INLINE
void sink_scalar (const T &s) {
    static T g_s = s;
}

template<class T, int N>
struct sink_c_vector {
    void operator () (const typename c_vector_traits<T, N>::type &v) {
        static typename c_vector_traits<T, N>::type g_v;
        for (int i = 0; i < N; ++ i)
            g_v [i] = v [i];
    }
};
template<class V>
BOOST_UBLAS_INLINE
void sink_vector (const V &v) {
    static V g_v (v);
}

template<class T, int N, int M>
struct sink_c_matrix {
    void operator () (const typename c_matrix_traits<T, N, M>::type &m) {
    static typename c_matrix_traits<T, N, M>::type g_m;
    for (int i = 0; i < N; ++ i)
        for (int j = 0; j < M; ++ j)
            g_m [i] [j] = m [i] [j];
    }
};
template<class M>
BOOST_UBLAS_INLINE
void sink_matrix (const M &m) {
    static M g_m (m);
}

template<class T>
struct peak {
    void operator () (int runs);
};

template<class T, int N>
struct bench_1 {
    void operator () (int runs);
};

template<class T, int N>
struct bench_2 {
    void operator () (int runs);
};

template<class T, int N>
struct bench_3 {
    void operator () (int runs);
};

struct safe_tag {};
struct fast_tag {};

// #define USE_FLOAT
#define USE_DOUBLE
// #define USE_STD_COMPLEX

#define USE_C_ARRAY
// #define USE_BOUNDED_ARRAY
#define USE_UNBOUNDED_ARRAY
// #define USE_STD_VALARRAY
#define USE_STD_VECTOR

#endif
