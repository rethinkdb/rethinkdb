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

#include "test3.hpp"

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
                v1 = ublas::row (m1, i);
                std::cout << "row (m, " << i << ") = " << v1 << std::endl;
                v1 = ublas::column (m1, i);
                std::cout << "column (m, " << i << ") = " << v1 << std::endl;
            }

            // Outer product
            initialize_vector (v1);
            initialize_vector (v2);
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
            V v1 (N, N), v2 (N, N);
            M m1 (N, N, N * N);

            test_with (v1, v2, m1);

            ublas::matrix_row<M> mr1 (m1, 0), mr2 (m1, N - 1);
            test_with (mr1, mr2, m1);

            ublas::matrix_column<M> mc1 (m1, 0), mc2 (m1, N - 1);
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
};

// Test matrix & vector
void test_matrix_vector () {
    std::cout << "test_matrix_vector" << std::endl;

#ifdef USE_SPARSE_MATRIX
#ifdef USE_MAP_ARRAY
#ifdef USE_FLOAT
    std::cout << "mp_test_type, map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<mp_test_type, ublas::map_array<std::size_t, mp_test_type> >,
                          ublas::mapped_matrix<mp_test_type, ublas::row_major, ublas::map_array<std::size_t, mp_test_type> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<double, ublas::map_array<std::size_t, double> >,
                          ublas::mapped_matrix<double, ublas::row_major, ublas::map_array<std::size_t, double> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<mp_test_type>, ublas::map_array<std::size_t, std::complex<mp_test_type> > >,
                          ublas::mapped_matrix<std::complex<mp_test_type>, ublas::row_major, ublas::map_array<std::size_t, std::complex<mp_test_type> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > >,
                          ublas::mapped_matrix<std::complex<double>, ublas::row_major, ublas::map_array<std::size_t, std::complex<double> > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_STD_MAP
#ifdef USE_FLOAT
    std::cout << "mp_test_type, std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<mp_test_type, std::map<std::size_t, mp_test_type> >,
                          ublas::mapped_matrix<mp_test_type, ublas::row_major, std::map<std::size_t, mp_test_type> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<double, std::map<std::size_t, double> >,
                          ublas::mapped_matrix<double, ublas::row_major, std::map<std::size_t, double> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<mp_test_type>, std::map<std::size_t, std::complex<mp_test_type> > >,
                          ublas::mapped_matrix<std::complex<mp_test_type>, ublas::row_major, std::map<std::size_t, std::complex<mp_test_type> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > >,
                          ublas::mapped_matrix<std::complex<double>, ublas::row_major, std::map<std::size_t, std::complex<double> > >, 3 > () ();
#endif
#endif
#endif
#endif

#ifdef USE_SPARSE_VECTOR_OF_SPARSE_VECTOR
#ifdef USE_MAP_ARRAY
#ifdef USE_FLOAT
    std::cout << "mp_test_type, mapped_vector map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<mp_test_type, ublas::map_array<std::size_t, mp_test_type> >,
                          ublas::mapped_vector<mp_test_type, ublas::row_major, ublas::map_array<std::size_t, ublas::map_array<std::size_t, mp_test_type> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, mapped_vector map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<double, ublas::map_array<std::size_t, double> >,
                          ublas::mapped_vector<double, ublas::row_major, ublas::map_array<std::size_t, ublas::map_array<std::size_t, double> > >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, mapped_vector map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<mp_test_type>, ublas::map_array<std::size_t, std::complex<mp_test_type> > >,
                          ublas::mapped_vector<std::complex<mp_test_type>, ublas::row_major, ublas::map_array<std::size_t, ublas::map_array<std::size_t, std::complex<mp_test_type> > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>,mapped_vector map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > >,
                          ublas::mapped_vector<std::complex<double>, ublas::row_major, ublas::map_array<std::size_t, ublas::map_array<std::size_t, std::complex<double> > > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_STD_MAP
#ifdef USE_FLOAT
    std::cout << "mp_test_type, mapped_vector std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<mp_test_type, std::map<std::size_t, mp_test_type> >,
                          ublas::mapped_vector<mp_test_type, ublas::row_major, std::map<std::size_t, std::map<std::size_t, mp_test_type> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, mapped_vector std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<double, std::map<std::size_t, double> >,
                          ublas::mapped_vector<double, ublas::row_major, std::map<std::size_t, std::map<std::size_t, double> > >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, mapped_vector std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<mp_test_type>, std::map<std::size_t, std::complex<mp_test_type> > >,
                          ublas::mapped_vector<std::complex<mp_test_type>, ublas::row_major, std::map<std::size_t, std::map<std::size_t, std::complex<mp_test_type> > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, mapped_vector std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > >,
                          ublas::mapped_vector<std::complex<double>, ublas::row_major, std::map<std::size_t, std::map<std::size_t, std::complex<double> > > >, 3 > () ();
#endif
#endif
#endif
#endif

#ifdef USE_GENERALIZED_VECTOR_OF_VECTOR
#ifdef USE_MAP_ARRAY
#ifdef USE_FLOAT
    std::cout << "mp_test_type, generalized_vector_of_vector map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<mp_test_type, ublas::map_array<std::size_t, mp_test_type> >,
                          ublas::generalized_vector_of_vector<mp_test_type, ublas::row_major, ublas::vector<ublas::mapped_vector<mp_test_type, ublas::map_array<std::size_t, mp_test_type> > > >, 3 > () ();
    test_my_matrix_vector<ublas::mapped_vector<mp_test_type, ublas::map_array<std::size_t, mp_test_type> >,
                          ublas::generalized_vector_of_vector<mp_test_type, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<mp_test_type, ublas::map_array<std::size_t, mp_test_type> >, ublas::map_array<std::size_t, ublas::mapped_vector<mp_test_type, ublas::map_array<std::size_t, mp_test_type> > > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, generalized_vector_of_vector map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<double, ublas::map_array<std::size_t, double> >,
                          ublas::generalized_vector_of_vector<double, ublas::row_major, ublas::vector<ublas::mapped_vector<double, ublas::map_array<std::size_t, double> > > >, 3 > () ();
    test_my_matrix_vector<ublas::mapped_vector<double, ublas::map_array<std::size_t, double> >,
                          ublas::generalized_vector_of_vector<double, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<double, ublas::map_array<std::size_t, double> >, ublas::map_array<std::size_t, ublas::mapped_vector<double, ublas::map_array<std::size_t, double> > > > >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, generalized_vector_of_vector map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<mp_test_type>, ublas::map_array<std::size_t, std::complex<mp_test_type> > >,
                          ublas::generalized_vector_of_vector<std::complex<mp_test_type>, ublas::row_major, ublas::vector<ublas::mapped_vector<std::complex<mp_test_type>, ublas::map_array<std::size_t, std::complex<mp_test_type> > > > >, 3 > () ();
    test_my_matrix_vector<ublas::mapped_vector<std::complex<mp_test_type>, ublas::map_array<std::size_t, std::complex<mp_test_type> > >,
                          ublas::generalized_vector_of_vector<std::complex<mp_test_type>, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<std::complex<mp_test_type>, ublas::map_array<std::size_t, std::complex<mp_test_type> > >, ublas::map_array<std::size_t, ublas::mapped_vector<std::complex<mp_test_type>, ublas::map_array<std::size_t, std::complex<mp_test_type> > > > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, generalized_vector_of_vector map_array" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > >,
                          ublas::generalized_vector_of_vector<std::complex<double>, ublas::row_major, ublas::vector<ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > > > >, 3 > () ();
    test_my_matrix_vector<ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > >,
                          ublas::generalized_vector_of_vector<std::complex<double>, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > >, ublas::map_array<std::size_t, ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > > > > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_STD_MAP
#ifdef USE_FLOAT
    std::cout << "mp_test_type, generalized_vector_of_vector std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<mp_test_type, std::map<std::size_t, mp_test_type> >,
                          ublas::generalized_vector_of_vector<mp_test_type, ublas::row_major, ublas::vector<ublas::mapped_vector<mp_test_type, std::map<std::size_t, mp_test_type> > > >, 3 > () ();
    test_my_matrix_vector<ublas::mapped_vector<mp_test_type, std::map<std::size_t, mp_test_type> >,
                          ublas::generalized_vector_of_vector<mp_test_type, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<mp_test_type, std::map<std::size_t, mp_test_type> >, std::map<std::size_t, ublas::mapped_vector<mp_test_type, std::map<std::size_t, mp_test_type> > > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, generalized_vector_of_vector std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<double, std::map<std::size_t, double> >,
                          ublas::generalized_vector_of_vector<double, ublas::row_major, ublas::vector<ublas::mapped_vector<double, std::map<std::size_t, double> > > >, 3 > () ();
    test_my_matrix_vector<ublas::mapped_vector<double, std::map<std::size_t, double> >,
                          ublas::generalized_vector_of_vector<double, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<double, std::map<std::size_t, double> >, std::map<std::size_t, ublas::mapped_vector<double, std::map<std::size_t, double> > > > >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, generalized_vector_of_vector std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<mp_test_type>, std::map<std::size_t, std::complex<mp_test_type> > >,
                          ublas::generalized_vector_of_vector<std::complex<mp_test_type>, ublas::row_major, ublas::vector<ublas::mapped_vector<std::complex<mp_test_type>, std::map<std::size_t, std::complex<mp_test_type> > > > >, 3 > () ();
    test_my_matrix_vector<ublas::mapped_vector<std::complex<mp_test_type>, std::map<std::size_t, std::complex<mp_test_type> > >,
                          ublas::generalized_vector_of_vector<std::complex<mp_test_type>, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<std::complex<mp_test_type>, std::map<std::size_t, std::complex<mp_test_type> > >, std::map<std::size_t, ublas::mapped_vector<std::complex<mp_test_type>, std::map<std::size_t, std::complex<mp_test_type> > > > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, generalized_vector_of_vector std::map" << std::endl;
    test_my_matrix_vector<ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > >,
                          ublas::generalized_vector_of_vector<std::complex<double>, ublas::row_major, ublas::vector<ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > > > >, 3 > () ();
    test_my_matrix_vector<ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > >,
                          ublas::generalized_vector_of_vector<std::complex<double>, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > >, std::map<std::size_t, ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > > > > >, 3 > () ();
#endif
#endif
#endif
#endif

#ifdef USE_COMPRESSED_MATRIX
#ifdef USE_FLOAT
    std::cout << "mp_test_type compressed" << std::endl;
    test_my_matrix_vector<ublas::compressed_vector<mp_test_type>,
                          ublas::compressed_matrix<mp_test_type>, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double compressed" << std::endl;
    test_my_matrix_vector<ublas::compressed_vector<double>,
                          ublas::compressed_matrix<double>, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type> compressed" << std::endl;
    test_my_matrix_vector<ublas::compressed_vector<std::complex<mp_test_type> >,
                          ublas::compressed_matrix<std::complex<mp_test_type> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double> compressed" << std::endl;
    test_my_matrix_vector<ublas::compressed_vector<std::complex<double> >,
                          ublas::compressed_matrix<std::complex<double> >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_COORDINATE_MATRIX
#ifdef USE_FLOAT
    std::cout << "mp_test_type coordinate" << std::endl;
    test_my_matrix_vector<ublas::coordinate_vector<mp_test_type>,
                          ublas::coordinate_matrix<mp_test_type>, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double coordinate" << std::endl;
    test_my_matrix_vector<ublas::coordinate_vector<double>,
                          ublas::coordinate_matrix<double>, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type> coordinate" << std::endl;
    test_my_matrix_vector<ublas::coordinate_vector<std::complex<mp_test_type> >,
                          ublas::coordinate_matrix<std::complex<mp_test_type> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double> coordinate" << std::endl;
    test_my_matrix_vector<ublas::coordinate_vector<std::complex<double> >,
                          ublas::coordinate_matrix<std::complex<double> >, 3 > () ();
#endif
#endif
#endif
}
