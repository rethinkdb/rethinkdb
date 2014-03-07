//  Copyright (c) 2000-2002
//  Joerg Walter, Mathias Koch
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.

#include "test1.hpp"

// Test vector expression templates
template<class V, int N>
struct test_my_vector {
    typedef typename V::value_type value_type;
    typedef typename V::size_type size_type;
    typedef typename ublas::type_traits<value_type>::real_type real_type;

    template<class VP>
    void test_container_with (VP &v1) const {
        // Container type tests in addition to expression types
        // Insert and erase
        v1.insert_element (0, 55);
        v1.erase_element (1);
        v1.clear ();
    }
    
    template<class VP>
    void test_expression_with (VP &v1, VP &v2, VP &v3) const {
        // Expression type tests
        value_type t;
        size_type i;
        real_type n;

        // Default Construct
        default_construct<VP>::test ();
        
        // Copy and swap
        initialize_vector (v1);
        initialize_vector (v2);
        v1 = v2;
        std::cout << "v1 = v2 = " << v1 << std::endl;
        v1.assign_temporary (v2);
        std::cout << "v1.assign_temporary (v2) = " << v1 << std::endl;
        v1.swap (v2);
        std::cout << "v1.swap (v2) = " << v1 << " " << v2 << std::endl;
        
        // Zero assignment
        v1 = ublas::zero_vector<> (v1.size ());
        std::cout << "v1.zero_vector = " << v1 << std::endl;
        v1 = v2;

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
            // Project range and slice
        initialize_vector (v1);
        initialize_vector (v2);
        project (v1, ublas::range(0,1)) = project (v2, ublas::range(0,1));
        project (v1, ublas::range(0,1)) = project (v2, ublas::slice(0,1,1));
        project (v1, ublas::slice(2,-1,2)) = project (v2, ublas::slice(0,1,2));
        project (v1, ublas::slice(2,-1,2)) = project (v2, ublas::range(0,2));
        std::cout << "v1 = range/slice " << v1 << std::endl;
#endif

            // Unary vector operations resulting in a vector
        initialize_vector (v1);
        v2 = - v1;
        std::cout << "- v1 = " << v2 << std::endl;
        v2 = ublas::conj (v1);
        std::cout << "conj (v1) = " << v2 << std::endl;

        // Binary vector operations resulting in a vector
        initialize_vector (v1);
        initialize_vector (v2);
        v3 = v1 + v2;
        std::cout << "v1 + v2 = " << v3 << std::endl;
        v3 = v1 - v2;
        std::cout << "v1 - v2 = " << v3 << std::endl;
        v3 = ublas::element_prod (v1, v2);
        std::cout << "element_prod (v1, v2) = " << v3 << std::endl;

        // Scaling a vector
        t = N;
        initialize_vector (v1);
        v2 = value_type (1.) * v1;
        std::cout << "1. * v1 = " << v2 << std::endl;
        v2 = t * v1;
        std::cout << "N * v1 = " << v2 << std::endl;
        initialize_vector (v1);
        v2 = v1 * value_type (1.);
        std::cout << "v1 * 1. = " << v2 << std::endl;
        v2 = v1 * t;
   std::cout << "v1 * value_type(N) = " << v2 << std::endl;
   // test interop with integer
   v2 = v1 * N;

        std::cout << "v1 * N = " << v2 << std::endl;

        // Some assignments
        initialize_vector (v1);
        initialize_vector (v2);
        v2 += v1;
        std::cout << "v2 += v1 = " << v2 << std::endl;
        v2 -= v1;
        std::cout << "v2 -= v1 = " << v2 << std::endl;
        v2 = v2 + v1;
        std::cout << "v2 = v2 + v1 = " << v2 << std::endl;
        v2 = v2 - v1;
        std::cout << "v2 = v2 - v1 = " << v2 << std::endl;
        v1 *= value_type (1.);
        std::cout << "v1 *= 1. = " << v1 << std::endl;
        v1 *= t;
        std::cout << "v1 *= value_type(N) = " << v1 << std::endl;
        // test interop with integer
        v1 *= N;
        std::cout << "v1 *= N = " << v1 << std::endl;

        // Unary vector operations resulting in a scalar
        initialize_vector (v1);
        t = ublas::sum (v1);
        std::cout << "sum (v1) = " << t << std::endl;
        n = ublas::norm_1 (v1);
        std::cout << "norm_1 (v1) = " << n << std::endl;
        n = ublas::norm_2 (v1);
        std::cout << "norm_2 (v1) = " << n << std::endl;
        n = ublas::norm_inf (v1);
        std::cout << "norm_inf (v1) = " << n << std::endl;

        i = ublas::index_norm_inf (v1);
        std::cout << "index_norm_inf (v1) = " << i << std::endl;

        // Binary vector operations resulting in a scalar
        initialize_vector (v1);
        initialize_vector (v2);
        t = ublas::inner_prod (v1, v2);
        std::cout << "inner_prod (v1, v2) = " << t << std::endl;

        // Scalar and Binary vector expression resulting in a vector
        initialize_vector (v1);
        initialize_vector (v2);
        v1 = v1 * ublas::inner_prod (v1, v2);
        std::cout << "v1 * inner_prod (v1, v2) = " << v1 << std::endl;
    }

    void operator () () const {
        V v1 (N), v2 (N), v3 (N);
        test_expression_with (v1, v2, v3);
        test_container_with (v1);

#ifdef USE_RANGE
        ublas::vector_range<V> vr1 (v1, ublas::range (0, N)),
                               vr2 (v2, ublas::range (0, N)),
                               vr3 (v3, ublas::range (0, N));
        test_expression_with (vr1, vr2, vr3);
#endif

#ifdef USE_SLICE
        ublas::vector_slice<V> vs1 (v1, ublas::slice (0, 1, N)),
                               vs2 (v2, ublas::slice (0, 1, N)),
                               vs3 (v3, ublas::slice (0, 1, N));
        test_expression_with (vs1, vs2, vs3);
#endif
    }
};

// Test vector
void test_vector () {
    std::cout << "test_vector" << std::endl;

#ifdef USE_BOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "mp_test_type, bounded_array" << std::endl;
    test_my_vector<ublas::vector<mp_test_type, ublas::bounded_array<mp_test_type, 3> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, bounded_array" << std::endl;
    test_my_vector<ublas::vector<double, ublas::bounded_array<double, 3> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, bounded_array" << std::endl;
    test_my_vector<ublas::vector<std::complex<mp_test_type>, ublas::bounded_array<std::complex<mp_test_type>, 3> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, bounded_array" << std::endl;
    test_my_vector<ublas::vector<std::complex<double>, ublas::bounded_array<std::complex<double>, 3> >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_UNBOUNDED_ARRAY
#ifdef USE_FLOAT
    std::cout << "mp_test_type, unbounded_array" << std::endl;
    test_my_vector<ublas::vector<mp_test_type, ublas::unbounded_array<mp_test_type> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, unbounded_array" << std::endl;
    test_my_vector<ublas::vector<double, ublas::unbounded_array<double> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, unbounded_array" << std::endl;
    test_my_vector<ublas::vector<std::complex<mp_test_type>, ublas::unbounded_array<std::complex<mp_test_type> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, unbounded_array" << std::endl;
    test_my_vector<ublas::vector<std::complex<double>, ublas::unbounded_array<std::complex<double> > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_STD_VECTOR
#ifdef USE_FLOAT
    std::cout << "mp_test_type, std::vector" << std::endl;
    test_my_vector<ublas::vector<mp_test_type, std::vector<mp_test_type> >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, std::vector" << std::endl;
    test_my_vector<ublas::vector<double, std::vector<double> >, 3 > () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, std::vector" << std::endl;
    test_my_vector<ublas::vector<std::complex<mp_test_type>, std::vector<std::complex<mp_test_type> > >, 3 > () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, std::vector" << std::endl;
    test_my_vector<ublas::vector<std::complex<double>, std::vector<std::complex<double> > >, 3 > () ();
#endif
#endif
#endif

#ifdef USE_BOUNDED_VECTOR
#ifdef USE_FLOAT
    std::cout << "mp_test_type, bounded" << std::endl;
    test_my_vector<ublas::bounded_vector<mp_test_type, 3>, 3> () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double, bounded" << std::endl;
    test_my_vector<ublas::bounded_vector<double, 3>, 3> () ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>, bounded" << std::endl;
    test_my_vector<ublas::bounded_vector<std::complex<mp_test_type>, 3>, 3> () ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>, bounded" << std::endl;
    test_my_vector<ublas::bounded_vector<std::complex<double>, 3>, 3> () ();
#endif
#endif
#endif
}
