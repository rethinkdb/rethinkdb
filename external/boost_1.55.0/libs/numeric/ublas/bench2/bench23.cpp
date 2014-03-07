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

#include "bench2.hpp"

template<class T, int N>
struct bench_c_matrix_prod {
    typedef T value_type;

    void operator () (int runs) const {
        try {
            static typename c_matrix_traits<T, N, N>::type m1, m2, m3;
            initialize_c_matrix<T, N, N> () (m1);
            initialize_c_matrix<T, N, N> () (m2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                for (int j = 0; j < N; ++ j) {
                    for (int k = 0; k < N; ++ k) {
                        m3 [j] [k] = 0;
                        for (int l = 0; l < N; ++ l) {
                            m3 [j] [k] += m1 [j] [l] * m2 [l] [k];
                        }
                    }
                }
//                sink_c_matrix<T, N, N> () (m3);
            }
            footer<value_type> () (N * N * N, N * N * (N - 1), runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class M1, class M2, int N>
struct bench_my_matrix_prod {
    typedef typename M1::value_type value_type;

    void operator () (int runs, safe_tag) const {
        try {
            static M1 m1 (N, N, N * N), m3 (N, N, N * N);
            static M2 m2 (N, N, N * N);
            initialize_matrix (m1);
            initialize_matrix (m2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                m3 = ublas::prod (m1, m2);
//                sink_matrix (m3);
            }
            footer<value_type> () (N * N * N, N * N * (N - 1), runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
    void operator () (int runs, fast_tag) const {
        try {
            static M1 m1 (N, N, N * N), m3 (N, N, N * N);
            static M2 m2 (N, N, N * N);
            initialize_matrix (m1);
            initialize_matrix (m2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                m3.assign (ublas::prod (m1, m2));
//                sink_matrix (m3);
            }
            footer<value_type> () (N * N * N, N * N * (N - 1), runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class M, int N>
struct bench_cpp_matrix_prod {
    typedef typename M::value_type value_type;

    void operator () (int runs) const {
        try {
            static M m1 (N * N), m2 (N * N), m3 (N * N);
            initialize_vector (m1);
            initialize_vector (m2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                for (int j = 0; j < N; ++ j) {
                    std::valarray<value_type> row (m1 [std::slice (N * j, N, 1)]);
                    for (int k = 0; k < N; ++ k) {
                        std::valarray<value_type> column (m2 [std::slice (k, N, N)]);
                        m3 [N * j + k] = (row * column).sum ();
                    }
                }
//                sink_vector (m3);
            }
            footer<value_type> () (N * N * N, N * N * (N - 1), runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};

// Benchmark O (n ^ 3)
template<class T, int N>
void bench_3<T, N>::operator () (int runs) {
    header ("bench_3");

    header ("prod (matrix, matrix)");

    header ("C array");
    bench_c_matrix_prod<T, N> () (runs);

#ifdef USE_SPARSE_MATRIX
#ifdef USE_MAP_ARRAY
    header ("sparse_matrix<row_major, map_array>, sparse_matrix<column_major, map_array> safe");
    bench_my_matrix_prod<ublas::sparse_matrix<T, ublas::row_major, ublas::map_array<std::size_t, T> >,
                         ublas::sparse_matrix<T, ublas::column_major, ublas::map_array<std::size_t, T> >, N> () (runs, safe_tag ());

    header ("sparse_matrix<row_major, map_array>, sparse_matrix<column_major, map_array> fast");
    bench_my_matrix_prod<ublas::sparse_matrix<T, ublas::row_major, ublas::map_array<std::size_t, T> >,
                         ublas::sparse_matrix<T, ublas::column_major, ublas::map_array<std::size_t, T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_MAP
    header ("sparse_matrix<row_major, std::map>, sparse_matrix<column_major, std::map> safe");
    bench_my_matrix_prod<ublas::sparse_matrix<T, ublas::row_major, std::map<std::size_t, T> >,
                         ublas::sparse_matrix<T, ublas::column_major, std::map<std::size_t, T> >, N> () (runs, safe_tag ());

    header ("sparse_matrix<row_major, std::map>, sparse_matrix<column_major, std::map> fast");
    bench_my_matrix_prod<ublas::sparse_matrix<T, ublas::row_major, std::map<std::size_t, T> >,
                         ublas::sparse_matrix<T, ublas::column_major, std::map<std::size_t, T> >, N> () (runs, fast_tag ());
#endif
#endif

#ifdef USE_COMPRESSED_MATRIX
    header ("compressed_matrix<row_major>, compressed_matrix<column_major> safe");
    bench_my_matrix_prod<ublas::compressed_matrix<T, ublas::row_major>,
                         ublas::compressed_matrix<T, ublas::column_major>, N> () (runs, safe_tag ());

    header ("compressed_matrix<row_major>, compressed_matrix<column_major> fast");
    bench_my_matrix_prod<ublas::compressed_matrix<T, ublas::row_major>,
                         ublas::compressed_matrix<T, ublas::column_major>, N> () (runs, fast_tag ());
#endif

#ifdef USE_COORDINATE_MATRIX
    header ("coordinate_matrix<row_major>, coordinate_matrix<column_major> safe");
    bench_my_matrix_prod<ublas::coordinate_matrix<T, ublas::row_major>,
                         ublas::coordinate_matrix<T, ublas::column_major>, N> () (runs, safe_tag ());

    header ("coordinate_matrix<row_major>, coordinate_matrix<column_major> fast");
    bench_my_matrix_prod<ublas::coordinate_matrix<T, ublas::row_major>,
                         ublas::coordinate_matrix<T, ublas::column_major>, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VALARRAY
    header ("std::valarray");
    bench_cpp_matrix_prod<std::valarray<T>, N> () (runs);
#endif
}

#ifdef USE_FLOAT
template struct bench_3<float, 3>;
template struct bench_3<float, 10>;
template struct bench_3<float, 30>;
template struct bench_3<float, 100>;
#endif

#ifdef USE_DOUBLE
template struct bench_3<double, 3>;
template struct bench_3<double, 10>;
template struct bench_3<double, 30>;
template struct bench_3<double, 100>;
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
template struct bench_3<std::complex<float>, 3>;
template struct bench_3<std::complex<float>, 10>;
template struct bench_3<std::complex<float>, 30>;
template struct bench_3<std::complex<float>, 100>;
#endif

#ifdef USE_DOUBLE
template struct bench_3<std::complex<double>, 3>;
template struct bench_3<std::complex<double>, 10>;
template struct bench_3<std::complex<double>, 30>;
template struct bench_3<std::complex<double>, 100>;
#endif
#endif
