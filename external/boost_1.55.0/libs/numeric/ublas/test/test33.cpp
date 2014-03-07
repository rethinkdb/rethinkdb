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

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
            // Project range and slice
            initialize_matrix (m1);
            initialize_matrix (m2);
            project (m1, ublas::range(0,1),ublas::range(0,1)) = project (m2, ublas::range(0,1),ublas::range(0,1));
            project (m1, ublas::range(0,1),ublas::range(0,1)) = project (m2, ublas::slice(0,1,1),ublas::slice(0,1,1));
            project (m1, ublas::slice(2,-1,2),ublas::slice(2,-1,2)) = project (m2, ublas::slice(0,1,2),ublas::slice(0,1,2));
            project (m1, ublas::slice(2,-1,2),ublas::slice(2,-1,2)) = project (m2, ublas::range(0,2),ublas::range(0,2));
            std::cout << "m1 = range/slice " << m1 << std::endl;
#endif

            // Unary matrix operations resulting in a matrix
            initialize_matrix (m1);
            m2 = - m1;
            std::cout << "- m1 = " << m2 << std::endl;
            m2 = ublas::conj (m1);
            std::cout << "conj (m1) = " << m2 << std::endl;

            // Binary matrix operations resulting in a matrix
            initialize_matrix (m1);
            initialize_matrix (m2);
            initialize_matrix (m3);
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
            m2 = ublas::trans (m1);
            std::cout << "trans (m1) = " << m2 << std::endl;

            // Hermitean
            initialize_matrix (m1);
            m2 = ublas::herm (m1);
            std::cout << "herm (m1) = " << m2 << std::endl;

            // Matrix multiplication
            initialize_matrix (m1);
            initialize_matrix (m2);
            m3 = ublas::prod (m1, m2);
            std::cout << "prod (m1, m2) = " << m3 << std::endl;
        }
    }
    void operator () () const {
        {
            M m1 (N, N, N * N), m2 (N, N, N * N), m3 (N, N, N * N);
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
    }
};

// Test matrix
void test_matrix () {
    std::cout << "test_matrix" << std::endl;

#ifdef USE_SPARSE_MATRIX
#ifdef USE_MAP_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, mapped_matrix map_array" << std::endl;
    test_my_matrix<ublas::mapped_matrix<float, ublas::row_major, ublas::map_array<std::size_t, float> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, mapped_matrix map_array" << std::endl;
    test_my_matrix<ublas::mapped_matrix<double, ublas::row_major, ublas::map_array<std::size_t, double> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, mapped_matrix map_array" << std::endl;
    test_my_matrix<ublas::mapped_matrix<std::complex<float>, ublas::row_major, ublas::map_array<std::size_t, std::complex<float> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, mapped_matrix map_array" << std::endl;
    test_my_matrix<ublas::mapped_matrix<std::complex<double>, ublas::row_major, ublas::map_array<std::size_t, std::complex<double> > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_STD_MAP
#ifdef USE_FLOAT
    std::cout << "float, mapped_matrix std::map" << std::endl;
    test_my_matrix<ublas::mapped_matrix<float, ublas::row_major, std::map<std::size_t, float> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, mapped_matrix std::map" << std::endl;
    test_my_matrix<ublas::mapped_matrix<double, ublas::row_major, std::map<std::size_t, double> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, mapped_matrix std::map" << std::endl;
    test_my_matrix<ublas::mapped_matrix<std::complex<float>, ublas::row_major, std::map<std::size_t, std::complex<float> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, mapped_matrix std::map" << std::endl;
    test_my_matrix<ublas::mapped_matrix<std::complex<double>, ublas::row_major, std::map<std::size_t, std::complex<double> > >, 3 > () ();
#endif
#endif
#endif
#endif

#ifdef USE_SPARSE_VECTOR_OF_SPARSE_VECTOR
#ifdef USE_MAP_ARRAY
#ifdef USE_FLOAT
    std::cout << "float, mapped_vector_of_mapped_vector map_array" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<float, ublas::row_major, ublas::map_array<std::size_t, ublas::map_array<std::size_t, float> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, mapped_vector_of_mapped_vector map_array" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<double, ublas::row_major, ublas::map_array<std::size_t, ublas::map_array<std::size_t, double> > >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, mapped_vector_of_mapped_vector map_array" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<std::complex<float>, ublas::row_major, ublas::map_array<std::size_t, ublas::map_array<std::size_t, std::complex<float> > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, mapped_vector_of_mapped_vectormap_array" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<std::complex<double>, ublas::row_major, ublas::map_array<std::size_t, ublas::map_array<std::size_t, std::complex<double> > > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_STD_MAP
#ifdef USE_FLOAT
    std::cout << "float, mapped_vector_of_mapped_vector std::map" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<float, ublas::row_major, std::map<std::size_t, std::map<std::size_t, float> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, mapped_vector_of_mapped_vector std::map" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<double, ublas::row_major, std::map<std::size_t, std::map<std::size_t, double> > >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, mapped_vector_of_mapped_vector std::map" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<std::complex<float>, ublas::row_major, std::map<std::size_t, std::map<std::size_t, std::complex<float> > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, mapped_vector_of_mapped_vector std::map" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<std::complex<double>, ublas::row_major, std::map<std::size_t, std::map<std::size_t, std::complex<double> > > >, 3 > () ();
#endif
#endif
#endif
#endif

#ifdef USE_GENERALIZED_VECTOR_OF_VECTOR
#ifdef USE_MAP_ARRAY
#ifdef USE_FLOAT
    std::cout << "float,generalized_vector_of_vector map_array" << std::endl;
    test_my_matrix<ublas::generalized_vector_of_vector<float, ublas::row_major, ublas::vector<ublas::mapped_vector<float, ublas::map_array<std::size_t, float> > > >, 3 > () ();
    test_my_matrix<ublas::generalized_vector_of_vector<float, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<float, ublas::map_array<std::size_t, float> >, ublas::map_array<std::size_t, ublas::mapped_vector<float, ublas::map_array<std::size_t, float> > > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, generalized_vector_of_vector map_array" << std::endl;
    test_my_matrix<ublas::generalized_vector_of_vector<double, ublas::row_major, ublas::vector<ublas::mapped_vector<double, ublas::map_array<std::size_t, double> > > >, 3 > () ();
    test_my_matrix<ublas::generalized_vector_of_vector<double, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<double, ublas::map_array<std::size_t, double> >, ublas::map_array<std::size_t, ublas::mapped_vector<double, ublas::map_array<std::size_t, double> > > > >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, generalized_vector_of_vector map_array" << std::endl;
    test_my_matrix<ublas::generalized_vector_of_vector<std::complex<float>, ublas::row_major, ublas::vector<ublas::mapped_vector<std::complex<float>, ublas::map_array<std::size_t, std::complex<float> > > > >, 3 > () ();
    test_my_matrix<ublas::generalized_vector_of_vector<std::complex<float>, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<std::complex<float>, ublas::map_array<std::size_t, std::complex<float> > >, ublas::map_array<std::size_t, ublas::mapped_vector<std::complex<float>, ublas::map_array<std::size_t, std::complex<float> > > > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, generalized_vector_of_vector map_array" << std::endl;
    test_my_matrix<ublas::generalized_vector_of_vector<std::complex<double>, ublas::row_major, ublas::vector<ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > > > >, 3 > () ();
    test_my_matrix<ublas::generalized_vector_of_vector<std::complex<double>, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > >, ublas::map_array<std::size_t, ublas::mapped_vector<std::complex<double>, ublas::map_array<std::size_t, std::complex<double> > > > > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_STD_MAP
#ifdef USE_FLOAT
    std::cout << "float, generalized_vector_of_vector std::map" << std::endl;
    test_my_matrix<ublas::generalized_vector_of_vector<float, ublas::row_major, ublas::vector<ublas::mapped_vector<float, std::map<std::size_t, float> > > >, 3 > () ();
    test_my_matrix<ublas::generalized_vector_of_vector<float, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<float, std::map<std::size_t, float> >, std::map<std::size_t, ublas::mapped_vector<float, std::map<std::size_t, float> > > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, generalized_vector_of_vector std::map" << std::endl;
    test_my_matrix<ublas::generalized_vector_of_vector<double, ublas::row_major, ublas::vector<ublas::mapped_vector<double, std::map<std::size_t, double> > > >, 3 > () ();
    test_my_matrix<ublas::generalized_vector_of_vector<double, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<double, std::map<std::size_t, double> >, std::map<std::size_t, ublas::mapped_vector<double, std::map<std::size_t, double> > > > >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float>, generalized_vector_of_vector std::map" << std::endl;
    test_my_matrix<ublas::generalized_vector_of_vector<std::complex<float>, ublas::row_major, ublas::vector<ublas::mapped_vector<std::complex<float>, std::map<std::size_t, std::complex<float> > > > >, 3 > () ();
    test_my_matrix<ublas::generalized_vector_of_vector<std::complex<float>, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<std::complex<float>, std::map<std::size_t, std::complex<float> > >, std::map<std::size_t, ublas::mapped_vector<std::complex<float>, std::map<std::size_t, std::complex<float> > > > > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, generalized_vector_of_vector std::map" << std::endl;
    test_my_matrix<ublas::generalized_vector_of_vector<std::complex<double>, ublas::row_major, ublas::vector<ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > > > >, 3 > () ();
    test_my_matrix<ublas::generalized_vector_of_vector<std::complex<double>, ublas::row_major, ublas::mapped_vector<ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > >, std::map<std::size_t, ublas::mapped_vector<std::complex<double>, std::map<std::size_t, std::complex<double> > > > > >, 3 > () ();
#endif
#endif
#endif
#endif

#ifdef USE_COMPRESSED_MATRIX
#ifdef USE_FLOAT
    std::cout << "float compressed_matrix" << std::endl;
    test_my_matrix<ublas::compressed_matrix<float>, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double compressed_matrix" << std::endl;
    test_my_matrix<ublas::compressed_matrix<double>, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float> compressed_matrix" << std::endl;
    test_my_matrix<ublas::compressed_matrix<std::complex<float> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double> compressed_matrix" << std::endl;
    test_my_matrix<ublas::compressed_matrix<std::complex<double> >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_COORDINATE_MATRIX
#ifdef USE_FLOAT
    std::cout << "float coordinate_matrix" << std::endl;
    test_my_matrix<ublas::coordinate_matrix<float>, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double coordinate_matrix" << std::endl;
    test_my_matrix<ublas::coordinate_matrix<double>, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float> coordinate_matrix" << std::endl;
    test_my_matrix<ublas::coordinate_matrix<std::complex<float> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double> coordinate_matrix" << std::endl;
    test_my_matrix<ublas::coordinate_matrix<std::complex<double> >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_MAPPED_VECTOR_OF_MAPPED_VECTOR
#ifdef USE_FLOAT
    std::cout << "float mapped_vector_of_mapped_vector" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<float>, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double mapped_vector_of_mapped_vector" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<double>, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<float> mapped_vector_of_mapped_vector" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<std::complex<float> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double> mapped_vector_of_mapped_vector" << std::endl;
    test_my_matrix<ublas::mapped_vector_of_mapped_vector<std::complex<double> >, 3 > () ();
#endif
#endif
#endif
}
