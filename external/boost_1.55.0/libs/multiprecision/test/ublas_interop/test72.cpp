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

#include "test7.hpp"

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
            V v1 (N), v2 (N);
            M m1 (N, N);
            test_with (v1, v2, m1);

            ublas::matrix_row<M> mr1 (m1, 0), mr2 (m1, 1);
            test_with (mr1, mr2, m1);

            ublas::matrix_column<M> mc1 (m1, 0), mc2 (m1, 1);
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

#ifdef USE_MATRIX
#ifdef USE_BOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "boost::numeric::interval<mp_test_type>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<mp_test_type>, ublas::bounded_array<boost::numeric::interval<mp_test_type>, 3> >,
                          ublas::matrix<boost::numeric::interval<mp_test_type>, ublas::row_major, ublas::bounded_array<boost::numeric::interval<mp_test_type>, 3 * 3> >, 3> () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "boost::numeric::interval<double>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<double>, ublas::bounded_array<boost::numeric::interval<double>, 3> >,
                          ublas::matrix<boost::numeric::interval<double>, ublas::row_major, ublas::bounded_array<boost::numeric::interval<double>, 3 * 3> >, 3> () ();
#endif
#endif

#ifdef USE_UNBOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "boost::numeric::interval<mp_test_type>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<mp_test_type>, ublas::unbounded_array<boost::numeric::interval<mp_test_type> > >,
                          ublas::matrix<boost::numeric::interval<mp_test_type>, ublas::row_major, ublas::unbounded_array<boost::numeric::interval<mp_test_type> > >, 3> () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "boost::numeric::interval<double>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<double>, ublas::unbounded_array<boost::numeric::interval<double> > >,
                          ublas::matrix<boost::numeric::interval<double>, ublas::row_major, ublas::unbounded_array<boost::numeric::interval<double> > >, 3> () ();
#endif
#endif

#ifdef USE_STD_VECTOR
#ifdef USE_FLOAT
    std::cout << "boost::numeric::interval<mp_test_type>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<mp_test_type>, std::vector<boost::numeric::interval<mp_test_type> > >,
                          ublas::matrix<boost::numeric::interval<mp_test_type>, ublas::row_major, std::vector<boost::numeric::interval<mp_test_type> > >, 3> () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "boost::numeric::interval<double>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<double>, std::vector<boost::numeric::interval<double> > >,
                          ublas::matrix<boost::numeric::interval<double>, ublas::row_major, std::vector<boost::numeric::interval<double> > >, 3> () ();
#endif
#endif
#endif

#ifdef USE_VECTOR_OF_VECTOR
#ifdef USE_BOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "boost::numeric::interval<mp_test_type>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<mp_test_type>, ublas::bounded_array<boost::numeric::interval<mp_test_type>, 3> >,
                          ublas::vector_of_vector<boost::numeric::interval<mp_test_type>, ublas::row_major, ublas::bounded_array<ublas::bounded_array<boost::numeric::interval<mp_test_type>, 3>, 3 + 1> >, 3> () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "boost::numeric::interval<double>, bounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<double>, ublas::bounded_array<boost::numeric::interval<double>, 3> >,
                          ublas::vector_of_vector<boost::numeric::interval<double>, ublas::row_major, ublas::bounded_array<ublas::bounded_array<boost::numeric::interval<double>, 3>, 3 + 1> >, 3> () ();
#endif
#endif

#ifdef USE_UNBOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "boost::numeric::interval<mp_test_type>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<mp_test_type>, ublas::unbounded_array<boost::numeric::interval<mp_test_type> > >,
                          ublas::vector_of_vector<boost::numeric::interval<mp_test_type>, ublas::row_major, ublas::unbounded_array<ublas::unbounded_array<boost::numeric::interval<mp_test_type> > > >, 3> () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "boost::numeric::interval<double>, unbounded_array" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<double>, ublas::unbounded_array<boost::numeric::interval<double> > >,
                          ublas::vector_of_vector<boost::numeric::interval<double>, ublas::row_major, ublas::unbounded_array<ublas::unbounded_array<boost::numeric::interval<double> > > >, 3> () ();
#endif
#endif

#ifdef USE_STD_VECTOR
#ifdef USE_FLOAT
    std::cout << "boost::numeric::interval<mp_test_type>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<mp_test_type>, std::vector<boost::numeric::interval<mp_test_type> > >,
                          ublas::vector_of_vector<boost::numeric::interval<mp_test_type>, ublas::row_major, std::vector<std::vector<boost::numeric::interval<mp_test_type> > > >, 3> () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "boost::numeric::interval<double>, std::vector" << std::endl;
    test_my_matrix_vector<ublas::vector<boost::numeric::interval<double>, std::vector<boost::numeric::interval<double> > >,
                          ublas::vector_of_vector<boost::numeric::interval<double>, ublas::row_major, std::vector<std::vector<boost::numeric::interval<double> > > >, 3> () ();
#endif
#endif
#endif
}
