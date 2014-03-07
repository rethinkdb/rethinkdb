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

#include "bench3.hpp"

template<class T, int N>
struct bench_c_outer_prod {
    typedef T value_type;

    void operator () (int runs) const {
        try {
            static typename c_matrix_traits<T, N, N>::type m;
            static typename c_vector_traits<T, N>::type v1, v2;
            initialize_c_vector<T, N> () (v1);
            initialize_c_vector<T, N> () (v2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                for (int j = 0; j < N; ++ j) {
                    for (int k = 0; k < N; ++ k) {
                        m [j] [k] = - v1 [j] * v2 [k];
                    }
                }
//                sink_c_matrix<T, N, N> () (m);
            }
            footer<value_type> () (N * N, N * N, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class M, class V, int N>
struct bench_my_outer_prod {
    typedef typename M::value_type value_type;

    void operator () (int runs, safe_tag) const {
        try {
            static M m (N, N);
            ublas::matrix_range<M> mr (m, ublas::range (0, N), ublas::range (0, N));
            static V v1 (N), v2 (N);
            ublas::vector_range<V> vr1 (v1, ublas::range (0, N)),
                                   vr2 (v2, ublas::range (0, N));
            initialize_vector (vr1);
            initialize_vector (vr2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                mr = - ublas::outer_prod (vr1, vr2);
//                sink_matrix (mr);
            }
            footer<value_type> () (N * N, N * N, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
    void operator () (int runs, fast_tag) const {
        try {
            static M m (N, N);
            ublas::matrix_range<M> mr (m, ublas::range (0, N), ublas::range (0, N));
            static V v1 (N), v2 (N);
            ublas::vector_range<V> vr1 (v1, ublas::range (0, N)),
                                   vr2 (v2, ublas::range (0, N));
            initialize_vector (vr1);
            initialize_vector (vr2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                mr.assign (- ublas::outer_prod (vr1, vr2));
//                sink_matrix (mr);
            }
            footer<value_type> () (N * N, N * N, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class M, class V, int N>
struct bench_cpp_outer_prod {
    typedef typename M::value_type value_type;

    void operator () (int runs) const {
        try {
            static M m (N * N);
            static V v1 (N), v2 (N);
            initialize_vector (v1);
            initialize_vector (v2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                for (int j = 0; j < N; ++ j) {
                    for (int k = 0; k < N; ++ k) {
                        m [N * j + k] = - v1 [j] * v2 [k];
                    }
                }
//                sink_vector (m);
            }
            footer<value_type> () (N * N, N * N, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};

template<class T, int N>
struct bench_c_matrix_vector_prod {
    typedef T value_type;

    void operator () (int runs) const {
        try {
            static typename c_matrix_traits<T, N, N>::type m;
            static typename c_vector_traits<T, N>::type v1, v2;
            initialize_c_matrix<T, N, N> () (m);
            initialize_c_vector<T, N> () (v1);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                for (int j = 0; j < N; ++ j) {
                    v2 [j] = 0;
                    for (int k = 0; k < N; ++ k) {
                        v2 [j] += m [j] [k] * v1 [k];
                    }
                }
//                sink_c_vector<T, N> () (v2);
            }
            footer<value_type> () (N * N, N * (N - 1), runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class M, class V, int N>
struct bench_my_matrix_vector_prod {
    typedef typename M::value_type value_type;

    void operator () (int runs, safe_tag) const {
        try {
            static M m (N, N);
            ublas::matrix_range<M> mr (m, ublas::range (0, N), ublas::range (0, N));
            static V v1 (N), v2 (N);
            ublas::vector_range<V> vr1 (v1, ublas::range (0, N)),
                                   vr2 (v2, ublas::range (0, N));
            initialize_matrix (mr);
            initialize_vector (vr1);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                vr2 = ublas::prod (mr, vr1);
//                sink_vector (vr2);
            }
            footer<value_type> () (N * N, N * (N - 1), runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
    void operator () (int runs, fast_tag) const {
        try {
            static M m (N, N);
            ublas::matrix_range<M> mr (m, ublas::range (0, N), ublas::range (0, N));
            static V v1 (N), v2 (N);
            ublas::vector_range<V> vr1 (v1, ublas::range (0, N)),
                                   vr2 (v2, ublas::range (0, N));
            initialize_matrix (mr);
            initialize_vector (vr1);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                vr2.assign (ublas::prod (mr, vr1));
//                sink_vector (vr2);
            }
            footer<value_type> () (N * N, N * (N - 1), runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class M, class V, int N>
struct bench_cpp_matrix_vector_prod {
    typedef typename M::value_type value_type;

    void operator () (int runs) const {
        try {
            static M m (N * N);
            static V v1 (N), v2 (N);
            initialize_vector (m);
            initialize_vector (v1);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                for (int j = 0; j < N; ++ j) {
                    std::valarray<value_type> row (m [std::slice (N * j, N, 1)]);
                    v2 [j] = (row * v1).sum ();
                }
//                sink_vector (v2);
            }
            footer<value_type> () (N * N, N * (N - 1), runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};

template<class T, int N>
struct bench_c_matrix_add {
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
                        m3 [j] [k] = - (m1 [j] [k] + m2 [j] [k]);
                    }
                }
//                sink_c_matrix<T, N, N> () (m3);
            }
            footer<value_type> () (0, 2 * N * N, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class M, int N>
struct bench_my_matrix_add {
    typedef typename M::value_type value_type;

    void operator () (int runs, safe_tag) const {
        try {
            static M m1 (N, N), m2 (N, N), m3 (N, N);
            initialize_matrix (m1);
            initialize_matrix (m2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                m3 = - (m1 + m2);
//                sink_matrix (m3);
            }
            footer<value_type> () (0, 2 * N * N, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
    void operator () (int runs, fast_tag) const {
        try {
            static M m1 (N, N), m2 (N, N), m3 (N, N);
            initialize_matrix (m1);
            initialize_matrix (m2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                m3.assign (- (m1 + m2));
//                sink_matrix (m3);
            }
            footer<value_type> () (0, 2 * N * N, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class M, int N>
struct bench_cpp_matrix_add {
    typedef typename M::value_type value_type;

    void operator () (int runs) const {
        try {
            static M m1 (N * N), m2 (N * N), m3 (N * N);
            initialize_vector (m1);
            initialize_vector (m2);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                m3 = - (m1 + m2);
//                sink_vector (m3);
            }
            footer<value_type> () (0, 2 * N * N, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};

// Benchmark O (n ^ 2)
template<class T, int N>
void bench_2<T, N>::operator () (int runs) {
    header ("bench_2");

    header ("outer_prod");

    header ("C array");
    bench_c_outer_prod<T, N> () (runs);

#ifdef USE_C_ARRAY
    header ("c_matrix, c_vector safe");
    bench_my_outer_prod<ublas::c_matrix<T, N, N>,
                        ublas::c_vector<T, N>, N> () (runs, safe_tag ());

    header ("c_matrix, c_vector fast");
    bench_my_outer_prod<ublas::c_matrix<T, N, N>,
                        ublas::c_vector<T, N>, N> () (runs, fast_tag ());
#endif

#ifdef USE_BOUNDED_ARRAY
    header ("matrix<bounded_array>, vector<bounded_array> safe");
    bench_my_outer_prod<ublas::matrix<T, ublas::row_major, ublas::bounded_array<T, N * N> >,
                        ublas::vector<T, ublas::bounded_array<T, N> >, N> () (runs, safe_tag ());

    header ("matrix<bounded_array>, vector<bounded_array> fast");
    bench_my_outer_prod<ublas::matrix<T, ublas::row_major, ublas::bounded_array<T, N * N> >,
                        ublas::vector<T, ublas::bounded_array<T, N> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_UNBOUNDED_ARRAY
    header ("matrix<unbounded_array>, vector<unbounded_array> safe");
    bench_my_outer_prod<ublas::matrix<T, ublas::row_major, ublas::unbounded_array<T> >,
                        ublas::vector<T, ublas::unbounded_array<T> >, N> () (runs, safe_tag ());

    header ("matrix<unbounded_array>, vector<unbounded_array> fast");
    bench_my_outer_prod<ublas::matrix<T, ublas::row_major, ublas::unbounded_array<T> >,
                        ublas::vector<T, ublas::unbounded_array<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VALARRAY
    header ("matrix<std::valarray>, vector<std::valarray> safe");
    bench_my_outer_prod<ublas::matrix<T, ublas::row_major, std::valarray<T> >,
                        ublas::vector<T, std::valarray<T> >, N> () (runs, safe_tag ());

    header ("matrix<std::valarray>, vector<std::valarray> fast");
    bench_my_outer_prod<ublas::matrix<T, ublas::row_major, std::valarray<T> >,
                        ublas::vector<T, std::valarray<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VECTOR
    header ("matrix<std::vector>, vector<std::vector> safe");
    bench_my_outer_prod<ublas::matrix<T, ublas::row_major, std::vector<T> >,
                        ublas::vector<T, std::vector<T> >, N> () (runs, safe_tag ());

    header ("matrix<std::vector>, vector<std::vector> fast");
    bench_my_outer_prod<ublas::matrix<T, ublas::row_major, std::vector<T> >,
                        ublas::vector<T, std::vector<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VALARRAY
    header ("std::valarray");
    bench_cpp_outer_prod<std::valarray<T>, std::valarray<T>, N> () (runs);
#endif

    header ("prod (matrix, vector)");

    header ("C array");
    bench_c_matrix_vector_prod<T, N> () (runs);

#ifdef USE_C_ARRAY
    header ("c_matrix, c_vector safe");
    bench_my_matrix_vector_prod<ublas::c_matrix<T, N, N>,
                                ublas::c_vector<T, N>, N> () (runs, safe_tag ());

    header ("c_matrix, c_vector fast");
    bench_my_matrix_vector_prod<ublas::c_matrix<T, N, N>,
                                ublas::c_vector<T, N>, N> () (runs, fast_tag ());
#endif

#ifdef USE_BOUNDED_ARRAY
    header ("matrix<bounded_array>, vector<bounded_array> safe");
    bench_my_matrix_vector_prod<ublas::matrix<T, ublas::row_major, ublas::bounded_array<T, N * N> >,
                                ublas::vector<T, ublas::bounded_array<T, N> >, N> () (runs, safe_tag ());

    header ("matrix<bounded_array>, vector<bounded_array> fast");
    bench_my_matrix_vector_prod<ublas::matrix<T, ublas::row_major, ublas::bounded_array<T, N * N> >,
                                ublas::vector<T, ublas::bounded_array<T, N> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_UNBOUNDED_ARRAY
    header ("matrix<unbounded_array>, vector<unbounded_array> safe");
    bench_my_matrix_vector_prod<ublas::matrix<T, ublas::row_major, ublas::unbounded_array<T> >,
                                ublas::vector<T, ublas::unbounded_array<T> >, N> () (runs, safe_tag ());

    header ("matrix<unbounded_array>, vector<unbounded_array> fast");
    bench_my_matrix_vector_prod<ublas::matrix<T, ublas::row_major, ublas::unbounded_array<T> >,
                                ublas::vector<T, ublas::unbounded_array<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VALARRAY
    header ("matrix<std::valarray>, vector<std::valarray> safe");
    bench_my_matrix_vector_prod<ublas::matrix<T, ublas::row_major, std::valarray<T> >,
                                ublas::vector<T, std::valarray<T> >, N> () (runs, safe_tag ());

    header ("matrix<std::valarray>, vector<std::valarray> fast");
    bench_my_matrix_vector_prod<ublas::matrix<T, ublas::row_major, std::valarray<T> >,
                                ublas::vector<T, std::valarray<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VECTOR
    header ("matrix<std::vector>, vector<std::vector> safe");
    bench_my_matrix_vector_prod<ublas::matrix<T, ublas::row_major, std::vector<T> >,
                                ublas::vector<T, std::vector<T> >, N> () (runs, safe_tag ());

    header ("matrix<std::vector>, vector<std::vector> fast");
    bench_my_matrix_vector_prod<ublas::matrix<T, ublas::row_major, std::vector<T> >,
                                ublas::vector<T, std::vector<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VALARRAY
    header ("std::valarray");
    bench_cpp_matrix_vector_prod<std::valarray<T>, std::valarray<T>, N> () (runs);
#endif

    header ("matrix + matrix");

    header ("C array");
    bench_c_matrix_add<T, N> () (runs);

#ifdef USE_C_ARRAY
    header ("c_matrix safe");
    bench_my_matrix_add<ublas::c_matrix<T, N, N>, N> () (runs, safe_tag ());

    header ("c_matrix fast");
    bench_my_matrix_add<ublas::c_matrix<T, N, N>, N> () (runs, fast_tag ());
#endif

#ifdef USE_BOUNDED_ARRAY
    header ("matrix<bounded_array> safe");
    bench_my_matrix_add<ublas::matrix<T, ublas::row_major, ublas::bounded_array<T, N * N> >, N> () (runs, safe_tag ());

    header ("matrix<bounded_array> fast");
    bench_my_matrix_add<ublas::matrix<T, ublas::row_major, ublas::bounded_array<T, N * N> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_UNBOUNDED_ARRAY
    header ("matrix<unbounded_array> safe");
    bench_my_matrix_add<ublas::matrix<T, ublas::row_major, ublas::unbounded_array<T> >, N> () (runs, safe_tag ());

    header ("matrix<unbounded_array> fast");
    bench_my_matrix_add<ublas::matrix<T, ublas::row_major, ublas::unbounded_array<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VALARRAY
    header ("matrix<std::valarray> safe");
    bench_my_matrix_add<ublas::matrix<T, ublas::row_major, std::valarray<T> >, N> () (runs, safe_tag ());

    header ("matrix<std::valarray> fast");
    bench_my_matrix_add<ublas::matrix<T, ublas::row_major, std::valarray<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VECTOR
    header ("matrix<std::vector> safe");
    bench_my_matrix_add<ublas::matrix<T, ublas::row_major, std::vector<T> >, N> () (runs, safe_tag ());

    header ("matrix<std::vector> fast");
    bench_my_matrix_add<ublas::matrix<T, ublas::row_major, std::vector<T> >, N> () (runs, fast_tag ());
#endif

#ifdef USE_STD_VALARRAY
    header ("std::valarray");
    bench_cpp_matrix_add<std::valarray<T>, N> () (runs);
#endif
}

#ifdef USE_FLOAT
template struct bench_2<float, 3>;
template struct bench_2<float, 10>;
template struct bench_2<float, 30>;
template struct bench_2<float, 100>;
#endif

#ifdef USE_DOUBLE
template struct bench_2<double, 3>;
template struct bench_2<double, 10>;
template struct bench_2<double, 30>;
template struct bench_2<double, 100>;
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
template struct bench_2<std::complex<float>, 3>;
template struct bench_2<std::complex<float>, 10>;
template struct bench_2<std::complex<float>, 30>;
template struct bench_2<std::complex<float>, 100>;
#endif

#ifdef USE_DOUBLE
template struct bench_2<std::complex<double>, 3>;
template struct bench_2<std::complex<double>, 10>;
template struct bench_2<std::complex<double>, 30>;
template struct bench_2<std::complex<double>, 100>;
#endif
#endif
