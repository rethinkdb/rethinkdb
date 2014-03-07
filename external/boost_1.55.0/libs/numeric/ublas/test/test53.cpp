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

// Test matrix expression templates
template<class M, int N>
struct test_my_matrix {
    typedef typename M::value_type value_type;

    template<class MP>
    void test_with (MP &m1, MP &m2, MP &m3) const {
        {
            value_type t;

            // Default Construct
            default_construct<MP>::test ();
            
            // Copy and swap
            initialize_matrix (m1);
            initialize_matrix (m2);
            m1 = m2;
            std::cout << "m1 = m2 = " << m1 << std::endl;
            m1.assign_temporary (m2);
            std::cout << "m1.assign_temporary (m2) = " << m1 << std::endl;
            m1.swap (m2);
            std::cout << "m1.swap (m2) = " << m1 << " " << m2 << std::endl;

            // Zero assignment
            m1 = ublas::zero_matrix<> (m1.size1 (), m1.size2 ());
            std::cout << "m1.zero_matrix = " << m1 << std::endl;
            m1 = m2;

            // Unary matrix operations resulting in a matrix
            initialize_matrix (m1);
            m2 = - m1;
            std::cout << "- m1 = " << m2 << std::endl;
            m2 = ublas::conj (m1);
            std::cout << "conj (m1) = " << m2 << std::endl;

            // Binary matrix operations resulting in a matrix
            initialize_matrix (m1);
            initialize_matrix (m2);
            m3 = m1 + m2;
            std::cout << "m1 + m2 = " << m3 << std::endl;
            m3 = m1 - m2;
            std::cout << "m1 - m2 = " << m3 << std::endl;

            // Scaling a matrix
            t = N;
            initialize_matrix (m1);
            m2 = value_type (1.) * m1;
            std::cout << "1. * m1 = " << m2 << std::endl;
            m2 = t * m1;
            std::cout << "N * m1 = " << m2 << std::endl;
            initialize_matrix (m1);
            m2 = m1 * value_type (1.);
            std::cout << "m1 * 1. = " << m2 << std::endl;
            m2 = m1 * t;
            std::cout << "m1 * N = " << m2 << std::endl;

            // Some assignments
            initialize_matrix (m1);
            initialize_matrix (m2);
            m2 += m1;
            std::cout << "m2 += m1 = " << m2 << std::endl;
            m2 -= m1;
            std::cout << "m2 -= m1 = " << m2 << std::endl;
            m2 = m2 + m1;
            std::cout << "m2 = m2 + m1 = " << m2 << std::endl;
            m2 = m2 - m1;
            std::cout << "m2 = m2 - m1 = " << m2 << std::endl;
            m1 *= value_type (1.);
            std::cout << "m1 *= 1. = " << m1 << std::endl;
            m1 *= t;
            std::cout << "m1 *= N = " << m1 << std::endl;

            // Transpose
            initialize_matrix (m1);
            // Transpose of a triangular isn't triangular of the same kind
            std::cout << "trans (m1) = " << ublas::trans (m1) << std::endl;

            // Hermitian
            initialize_matrix (m1);
            // Hermitian of a triangular isn't hermitian of the same kind
            std::cout << "herm (m1) = " << ublas::herm (m1) << std::endl;

            // Matrix multiplication
            initialize_matrix (m1);
            initialize_matrix (m2);
            m3 = ublas::prod (m1, m2);
            std::cout << "prod (m1, m2) = " << m3 << std::endl;
        }
    }
    void operator () () const {
        {
            M m1 (N, N), m2 (N, N), m3 (N, N);
            test_with (m1, m2, m3);

#ifdef USE_RANGE
            ublas::matrix_range<M> mr1 (m1, ublas::range (0, N), ublas::range (0, N)),
                                   mr2 (m2, ublas::range (0, N), ublas::range (0, N)),
                                   mr3 (m3, ublas::range (0, N), ublas::range (0, N));
            test_with (mr1, mr2, mr3);
#endif

#ifdef USE_SLICE
            ublas::matrix_slice<M> ms1 (m1, ublas::slice (0, 1, N), ublas::slice (0, 1, N)),
                                   ms2 (m2, ublas::slice (0, 1, N), ublas::slice (0, 1, N)),
                                   ms3 (m3, ublas::slice (0, 1, N), ublas::slice (0, 1, N));
            test_with (ms1, ms2, ms3);
#endif
        }

#ifdef USE_ADAPTOR
        {
            M m1 (N, N), m2 (N, N), m3 (N, N);
            ublas::triangular_adaptor<M> tam1 (m1), tam2 (m2), tam3 (m3);
            test_with (tam1, tam2, tam3);

#ifdef USE_RANGE
            ublas::matrix_range<ublas::triangular_adaptor<M> > mr1 (tam1, ublas::range (0, N), ublas::range (0, N)),
                                                               mr2 (tam2, ublas::range (0, N), ublas::range (0, N)),
                                                               mr3 (tam3, ublas::range (0, N), ublas::range (0, N));
            test_with (mr1, mr2, mr3);
#endif

#ifdef USE_SLICE
            ublas::matrix_slice<ublas::triangular_adaptor<M> > ms1 (tam1, ublas::slice (0, 1, N), ublas::slice (0, 1, N)),
                                                               ms2 (tam2, ublas::slice (0, 1, N), ublas::slice (0, 1, N)),
                                                               ms3 (tam3, ublas::slice (0, 1, N), ublas::slice (0, 1, N));
            test_with (ms1, ms2, ms3);
#endif
        }
#endif
    }
};

// Test matrix
void test_matrix () {
    std::cout << "test_matrix" << std::endl;

#ifdef USE_BOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, bounded_array" << std::endl;
    test_my_matrix<ublas::triangular_matrix<float, ublas::lower, ublas::row_major, ublas::bounded_array<float, 3 * 3> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, bounded_array" << std::endl;
    test_my_matrix<ublas::triangular_matrix<double, ublas::lower, ublas::row_major, ublas::bounded_array<double, 3 * 3> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, bounded_array" << std::endl;
    test_my_matrix<ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, ublas::bounded_array<std::complex<float>, 3 * 3> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, bounded_array" << std::endl;
    test_my_matrix<ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, ublas::bounded_array<std::complex<double>, 3 * 3> >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_UNBOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, unbounded_array" << std::endl;
    test_my_matrix<ublas::triangular_matrix<float, ublas::lower, ublas::row_major, ublas::unbounded_array<float> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, unbounded_array" << std::endl;
    test_my_matrix<ublas::triangular_matrix<double, ublas::lower, ublas::row_major, ublas::unbounded_array<double> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, unbounded_array" << std::endl;
    test_my_matrix<ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, ublas::unbounded_array<std::complex<float> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, unbounded_array" << std::endl;
    test_my_matrix<ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, ublas::unbounded_array<std::complex<double> > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_STD_VECTOR
#ifdef USE_FLOAT
    std::cout << "float, std::vector" << std::endl;
    test_my_matrix<ublas::triangular_matrix<float, ublas::lower, ublas::row_major, std::vector<float> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, std::vector" << std::endl;
    test_my_matrix<ublas::triangular_matrix<double, ublas::lower, ublas::row_major, std::vector<double> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, std::vector" << std::endl;
    test_my_matrix<ublas::triangular_matrix<std::complex<float>, ublas::lower, ublas::row_major, std::vector<std::complex<float> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
std::cout << "std::complex<double>, std::vector" << std::endl;
    test_my_matrix<ublas::triangular_matrix<std::complex<double>, ublas::lower, ublas::row_major, std::vector<std::complex<double> > >, 3 > () ();
#endif
#endif
#endif
}
