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

#include "test5.hpp"

// Test matrix & vector expression templates
template<class V, class M, int N>
struct test_my_matrix_vector {
    typedef typename V::value_type value_type;

    template<class VP, class MP>
    void test_with (VP &v1, VP &v2, MP &m1) const {
        {
            // Rows and columns
            initialize_matrix (m1);
            for (int i = 0; i < N; ++ i) {
                v2 = ublas::row (m1, i);
                std::cout << "row (m, " << i << ") = " << v2 << std::endl;
                v2 = ublas::column (m1, i);
                std::cout << "column (m, " << i << ") = " << v2 << std::endl;
            }

            // Outer product
            initialize_vector (v1);
            initialize_vector (v2);
            v1 (0) = 0;
            v2 (N - 1) = 0;
            m1 = ublas::outer_prod (v1, v2);
            std::cout << "outer_prod (v1, v2) = " << m1 << std::endl;

            // Matrix vector product
            initialize_matrix (m1);
            initialize_vector (v1);
            v2 = ublas::prod (m1, v1);
            std::cout << "prod (m1, v1) = " << v2 << std::endl;
            v2 = ublas::prod (v1, m1);
            std::cout << "prod (v1, m1) = " << v2 << std::endl;
        }
    }
    void operator () () const {
        {
            V v1 (N), v2 (N);
            M m1 (N, N);
            test_with (v1, v2, m1);

            ublas::matrix_row<M> mr1 (m1, N - 1), mr2 (m1, N - 1);
            test_with (mr1, mr2, m1);

            ublas::matrix_column<M> mc1 (m1, 0), mc2 (m1, 0);
            test_with (mc1, mc2, m1);

#ifdef USE_RANGE
            ublas::matrix_vector_range<M> mvr1 (m1, ublas::range (0, N), ublas::range (0, N)),
                                          mvr2 (m1, ublas::range (0, N), ublas::range (0, N));
            test_with (mvr1, mvr2, m1);
#endif

#ifdef USE_SLICE
            ublas::matrix_vector_slice<M> mvs1 (m1, ublas::slice (0, 1, N), ublas::slice (0, 1, N)),
                                          mvs2 (m1, ublas::slice (0, 1, N), ublas::slice (0, 1, N));
            test_with (mvs1, mvs2, m1);
#endif
        }
    }

    void operator () (int) const {
#ifdef USE_ADAPTOR
        {
            V v1 (N), v2 (N);
            M m1 (N, N);
            ublas::triangular_adaptor<M> tam1 (m1);
            test_with (v1, v2, tam1);

            ublas::matrix_row<ublas::triangular_adaptor<M> > mr1 (tam1, N - 1), mr2 (tam1, N - 1);
            test_with (mr1, mr2, tam1);

            ublas::matrix_column<ublas::triangular_adaptor<M> > mc1 (tam1, 0), mc2 (tam1, 0);
            test_with (mc1, mc2, tam1);

#ifdef USE_RANGE
            ublas::matrix_vector_range<ublas::triangular_adaptor<M> > mvr1 (tam1, ublas::range (0, N), ublas::range (0, N)),
                                                                      mvr2 (tam1, ublas::range (0, N), ublas::range (0, N));
            test_with (mvr1, mvr2, tam1);
#endif

#ifdef USE_SLICE
            ublas::matrix_vector_slice<ublas::triangular_adaptor<M> > mvs1 (tam1, ublas::slice (0, 1, N), ublas::slice (0, 1, N)),
                                                                      mvs2 (tam1, ublas::slice (0, 1, N), ublas::slice (0, 1, N));
            test_with (mvs1, mvs2, tam1);
#endif
        }
#endif
    }
};

// Test matrix & vector
void test_matrix_vector () {
    std::cout << "test_matrix_vector" << std::endl;

#ifdef USE_BOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<float, ublas::bounded_array<float, 3> >,
                          ublas::triangular_matrix<float, ublas::lower, ublas::row_major, ublas::bounded_array<float, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, ublas::bounded_array<float, 3> >,
                          ublas::triangular_matrix<float, ublas::lower, ublas::row_major, ublas::bounded_array<float, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<double, ublas::bounded_array<double, 3> >,
                          ublas::triangular_matrix<double, ublas::lower, ublas::row_major, ublas::bounded_array<double, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, ublas::bounded_array<double, 3> >,
                          ublas::triangular_matrix<double, ublas::lower, ublas::row_major, ublas::bounded_array<double, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::bounded_array<std::complex<float>, 3> >,
                          ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, ublas::bounded_array<std::complex<float>, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::bounded_array<std::complex<float>, 3> >,
                          ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, ublas::bounded_array<std::complex<float>, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::bounded_array<std::complex<double>, 3> >,
                          ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, ublas::bounded_array<std::complex<double>, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::bounded_array<std::complex<double>, 3> >,
                          ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, ublas::bounded_array<std::complex<double>, 3 * 3> >, 3> () (0);
#endif
#endif
#endif

#ifdef USE_UNBOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<float, ublas::unbounded_array<float> >,
                          ublas::triangular_matrix<float, ublas::lower, ublas::row_major, ublas::unbounded_array<float> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, ublas::unbounded_array<float> >,
                          ublas::triangular_matrix<float, ublas::lower, ublas::row_major, ublas::unbounded_array<float> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<double, ublas::unbounded_array<double> >,
                          ublas::triangular_matrix<double, ublas::lower, ublas::row_major, ublas::unbounded_array<double> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, ublas::unbounded_array<double> >,
                          ublas::triangular_matrix<double, ublas::lower, ublas::row_major, ublas::unbounded_array<double> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::unbounded_array<std::complex<float> > >,
                          ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, ublas::unbounded_array<std::complex<float> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::unbounded_array<std::complex<float> > >,
                          ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, ublas::unbounded_array<std::complex<float> > >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::unbounded_array<std::complex<double> > >,
                          ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, ublas::unbounded_array<std::complex<double> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::unbounded_array<std::complex<double> > >,
                          ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, ublas::unbounded_array<std::complex<double> > >, 3> () (0);
#endif
#endif
#endif

#ifdef USE_STD_VECTOR
#ifdef USE_FLOAT
    std::cout << "float, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<float, std::vector<float> >,
                          ublas::triangular_matrix<float, ublas::lower, ublas::row_major, std::vector<float> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, std::vector<float> >,
                          ublas::triangular_matrix<float, ublas::lower, ublas::row_major, std::vector<float> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<double, std::vector<double> >,
                          ublas::triangular_matrix<double, ublas::lower, ublas::row_major, std::vector<double> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, std::vector<double> >,
                          ublas::triangular_matrix<double, ublas::lower, ublas::row_major, std::vector<double> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, std::vector<std::complex<float> > >,
                          ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, std::vector<std::complex<float> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, std::vector<std::complex<float> > >,
                          ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, std::vector<std::complex<float> > >, 3> () (0);

    std::cout << "std::complex<double>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, std::vector<std::complex<double> > >,
                          ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, std::vector<std::complex<double> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, std::vector<std::complex<double> > >,
                          ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, std::vector<std::complex<double> > >, 3> () (0);
#endif
#endif
#endif
}
