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

#include "test4.hpp"

// Test matrix & vector expression templates
template<class V, class M, int N>
struct test_my_matrix_vector {
    typedef typename V::value_type value_type;

    template<class VP, class MP>
    void test_with (VP &v1, VP &v2, MP &m1) const {
        {
#ifndef USE_DIAGONAL
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
            v1 (N - 1) = 0;
            m1 = ublas::outer_prod (v1, v2);
            std::cout << "outer_prod (v1, v2) = " << m1 << std::endl;

            // Matrix vector product
            initialize_matrix (m1);
            initialize_vector (v1);
            v2 = ublas::prod (m1, v1);
            std::cout << "prod (m1, v1) = " << v2 << std::endl;
            v2 = ublas::prod (v1, m1);
            std::cout << "prod (v1, m1) = " << v2 << std::endl;
#endif
        }
    }
    void operator () () const {
        {
            V v1 (N), v2 (N);
#ifdef USE_BANDED
            M m1 (N, N, 1, 1);
#endif
#ifdef USE_DIAGONAL
            M m1 (N, N);
#endif
            test_with (v1, v2, m1);

            ublas::matrix_row<M> mr1 (m1, 1), mr2 (m1, 1);
            test_with (mr1, mr2, m1);

            ublas::matrix_column<M> mc1 (m1, 1), mc2 (m1, 1);
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
#ifdef USE_BANDED
            V v1 (N), v2 (N);
            M m1 (N, N, 1, 1);
            ublas::banded_adaptor<M> bam1 (m1, 1, 1);
            test_with (v1, v2, bam1);

            ublas::matrix_row<ublas::banded_adaptor<M> > mr1 (bam1, 1), mr2 (bam1, 1);
            test_with (mr1, mr2, bam1);

            ublas::matrix_column<ublas::banded_adaptor<M> > mc1 (bam1, 1), mc2 (bam1, 1);
            test_with (mc1, mc2, bam1);

#ifdef USE_RANGE
            ublas::matrix_vector_range<ublas::banded_adaptor<M> > mvr1 (bam1, ublas::range (0, N), ublas::range (0, N)),
                                                                  mvr2 (bam1, ublas::range (0, N), ublas::range (0, N));
            test_with (mvr1, mvr2, bam1);
#endif

#ifdef USE_SLICE
            ublas::matrix_vector_slice<ublas::banded_adaptor<M> > mvs1 (bam1, ublas::slice (0, 1, N), ublas::slice (0, 1, N)),
                                                                  mvs2 (bam1, ublas::slice (0, 1, N), ublas::slice (0, 1, N));
            test_with (mvs1, mvs2, bam1);
#endif
#endif
#ifdef USE_DIAGONAL
            V v1 (N), v2 (N);
            M m1 (N, N);
            ublas::diagonal_adaptor<M> dam1 (m1);
            test_with (v1, v2, dam1);

            ublas::matrix_row<ublas::diagonal_adaptor<M> > mr1 (dam1, 1), mr2 (dam1, 1);
            test_with (mr1, mr2, dam1);

            ublas::matrix_column<ublas::diagonal_adaptor<M> > mc1 (dam1, 1), mc2 (dam1, 1);
            test_with (mc1, mc2, dam1);

#ifdef USE_RANGE
            ublas::matrix_vector_range<ublas::diagonal_adaptor<M> > mvr1 (dam1, ublas::range (0, N), ublas::range (0, N)),
                                                                    mvr2 (dam1, ublas::range (0, N), ublas::range (0, N));
            test_with (mvr1, mvr2, dam1);
#endif

#ifdef USE_SLICE
            ublas::matrix_vector_slice<ublas::diagonal_adaptor<M> > mvs1 (dam1, ublas::slice (0, 1, N), ublas::slice (0, 1, N)),
                                                                    mvs2 (dam1, ublas::slice (0, 1, N), ublas::slice (0, 1, N));
            test_with (mvs1, mvs2, dam1);
#endif
#endif
        }
#endif
    }
};

// Test matrix & vector
void test_matrix_vector () {
    std::cout << "test_matrix_vector" << std::endl;

#ifdef USE_BANDED
#ifdef USE_BOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<float, ublas::bounded_array<float, 3> >,
                          ublas::banded_matrix<float, ublas::row_major, ublas::bounded_array<float, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, ublas::bounded_array<float, 3> >,
                          ublas::banded_matrix<float, ublas::row_major, ublas::bounded_array<float, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<double, ublas::bounded_array<double, 3> >,
                          ublas::banded_matrix<double, ublas::row_major, ublas::bounded_array<double, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, ublas::bounded_array<double, 3> >,
                          ublas::banded_matrix<double, ublas::row_major, ublas::bounded_array<double, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::bounded_array<std::complex<float>, 3> >,
                          ublas::banded_matrix<std::complex<float>, ublas::row_major, ublas::bounded_array<std::complex<float>, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::bounded_array<std::complex<float>, 3> >,
                          ublas::banded_matrix<std::complex<float>, ublas::row_major, ublas::bounded_array<std::complex<float>, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::bounded_array<std::complex<double>, 3> >,
                          ublas::banded_matrix<std::complex<double>, ublas::row_major, ublas::bounded_array<std::complex<double>, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::bounded_array<std::complex<double>, 3> >,
                          ublas::banded_matrix<std::complex<double>, ublas::row_major, ublas::bounded_array<std::complex<double>, 3 * 3> >, 3> () (0);
#endif
#endif
#endif

#ifdef USE_UNBOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<float, ublas::unbounded_array<float> >,
                          ublas::banded_matrix<float, ublas::row_major, ublas::unbounded_array<float> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, ublas::unbounded_array<float> >,
                          ublas::banded_matrix<float, ublas::row_major, ublas::unbounded_array<float> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<double, ublas::unbounded_array<double> >,
                          ublas::banded_matrix<double, ublas::row_major, ublas::unbounded_array<double> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, ublas::unbounded_array<double> >,
                          ublas::banded_matrix<double, ublas::row_major, ublas::unbounded_array<double> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::unbounded_array<std::complex<float> > >,
                          ublas::banded_matrix<std::complex<float>, ublas::row_major, ublas::unbounded_array<std::complex<float> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::unbounded_array<std::complex<float> > >,
                          ublas::banded_matrix<std::complex<float>, ublas::row_major, ublas::unbounded_array<std::complex<float> > >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::unbounded_array<std::complex<double> > >,
                          ublas::banded_matrix<std::complex<double>, ublas::row_major, ublas::unbounded_array<std::complex<double> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::unbounded_array<std::complex<double> > >,
                          ublas::banded_matrix<std::complex<double>, ublas::row_major, ublas::unbounded_array<std::complex<double> > >, 3> () (0);
#endif
#endif
#endif

#ifdef USE_STD_VECTOR
#ifdef USE_FLOAT
    std::cout << "float, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<float, std::vector<float> >,
                          ublas::banded_matrix<float, ublas::row_major, std::vector<float> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, std::vector<float> >,
                          ublas::banded_matrix<float, ublas::row_major, std::vector<float> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<double, std::vector<double> >,
                          ublas::banded_matrix<double, ublas::row_major, std::vector<double> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, std::vector<double> >,
                          ublas::banded_matrix<double, ublas::row_major, std::vector<double> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, std::vector<std::complex<float> > >,
                          ublas::banded_matrix<std::complex<float>, ublas::row_major, std::vector<std::complex<float> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, std::vector<std::complex<float> > >,
                          ublas::banded_matrix<std::complex<float>, ublas::row_major, std::vector<std::complex<float> > >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, std::vector<std::complex<double> > >,
                          ublas::banded_matrix<std::complex<double>, ublas::row_major, std::vector<std::complex<double> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, std::vector<std::complex<double> > >,
                          ublas::banded_matrix<std::complex<double>, ublas::row_major, std::vector<std::complex<double> > >, 3> () (0);
#endif
#endif
#endif
#endif

#ifdef USE_DIAGONAL
#ifdef USE_BOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<float, ublas::bounded_array<float, 3> >,
                          ublas::diagonal_matrix<float, ublas::row_major, ublas::bounded_array<float, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, ublas::bounded_array<float, 3> >,
                          ublas::diagonal_matrix<float, ublas::row_major, ublas::bounded_array<float, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<double, ublas::bounded_array<double, 3> >,
                          ublas::diagonal_matrix<double, ublas::row_major, ublas::bounded_array<double, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, ublas::bounded_array<double, 3> >,
                          ublas::diagonal_matrix<double, ublas::row_major, ublas::bounded_array<double, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::bounded_array<std::complex<float>, 3> >,
                          ublas::diagonal_matrix<std::complex<float>, ublas::row_major, ublas::bounded_array<std::complex<float>, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::bounded_array<std::complex<float>, 3> >,
                          ublas::diagonal_matrix<std::complex<float>, ublas::row_major, ublas::bounded_array<std::complex<float>, 3 * 3> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::bounded_array<std::complex<double>, 3> >,
                          ublas::diagonal_matrix<std::complex<double>, ublas::row_major, ublas::bounded_array<std::complex<double>, 3 * 3> >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::bounded_array<std::complex<double>, 3> >,
                          ublas::diagonal_matrix<std::complex<double>, ublas::row_major, ublas::bounded_array<std::complex<double>, 3 * 3> >, 3> () (0);
#endif
#endif
#endif

#ifdef USE_UNBOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<float, ublas::unbounded_array<float> >,
                          ublas::diagonal_matrix<float, ublas::row_major, ublas::unbounded_array<float> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, ublas::unbounded_array<float> >,
                          ublas::diagonal_matrix<float, ublas::row_major, ublas::unbounded_array<float> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<double, ublas::unbounded_array<double> >,
                          ublas::diagonal_matrix<double, ublas::row_major, ublas::unbounded_array<double> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, ublas::unbounded_array<double> >,
                          ublas::diagonal_matrix<double, ublas::row_major, ublas::unbounded_array<double> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::unbounded_array<std::complex<float> > >,
                          ublas::diagonal_matrix<std::complex<float>, ublas::row_major, ublas::unbounded_array<std::complex<float> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, ublas::unbounded_array<std::complex<float> > >,
                          ublas::diagonal_matrix<std::complex<float>, ublas::row_major, ublas::unbounded_array<std::complex<float> > >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::unbounded_array<std::complex<double> > >,
                          ublas::diagonal_matrix<std::complex<double>, ublas::row_major, ublas::unbounded_array<std::complex<double> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, ublas::unbounded_array<std::complex<double> > >,
                          ublas::diagonal_matrix<std::complex<double>, ublas::row_major, ublas::unbounded_array<std::complex<double> > >, 3> () (0);
#endif
#endif
#endif

#ifdef USE_STD_VECTOR
#ifdef USE_FLOAT
    std::cout << "float, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<float, std::vector<float> >,
                          ublas::diagonal_matrix<float, ublas::row_major, std::vector<float> >, 3> () ();
    test_my_matrix_vector<ublas::vector<float, std::vector<float> >,
                          ublas::diagonal_matrix<float, ublas::row_major, std::vector<float> >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "double, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<double, std::vector<double> >,
                          ublas::diagonal_matrix<double, ublas::row_major, std::vector<double> >, 3> () ();
    test_my_matrix_vector<ublas::vector<double, std::vector<double> >,
                          ublas::diagonal_matrix<double, ublas::row_major, std::vector<double> >, 3> () (0);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<float>, std::vector<std::complex<float> > >,
                          ublas::diagonal_matrix<std::complex<float>, ublas::row_major, std::vector<std::complex<float> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<float>, std::vector<std::complex<float> > >,
                          ublas::diagonal_matrix<std::complex<float>, ublas::row_major, std::vector<std::complex<float> > >, 3> () (0);
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<std::complex<double>, std::vector<std::complex<double> > >,
                          ublas::diagonal_matrix<std::complex<double>, ublas::row_major, std::vector<std::complex<double> > >, 3> () ();
    test_my_matrix_vector<ublas::vector<std::complex<double>, std::vector<std::complex<double> > >,
                          ublas::diagonal_matrix<std::complex<double>, ublas::row_major, std::vector<std::complex<double> > >, 3> () (0);
#endif
#endif
#endif
#endif
}
