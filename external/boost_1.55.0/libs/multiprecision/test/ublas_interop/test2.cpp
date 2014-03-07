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

#include "test2.hpp"

int main () {
#ifdef USE_FLOAT
    std::cout << "mp_test_type" << std::endl;
    test_blas_1<ublas::vector<mp_test_type>, 3> ().test ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double" << std::endl;
    test_blas_1<ublas::vector<double>, 3> ().test ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>" << std::endl;
    test_blas_1<ublas::vector<std::complex<mp_test_type> >, 3> ().test ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>" << std::endl;
    test_blas_1<ublas::vector<std::complex<double> >, 3> ().test ();
#endif
#endif

    std::cout << "test_blas_2" << std::endl;

#ifdef USE_FLOAT
    std::cout << "mp_test_type" << std::endl;
    test_blas_2<ublas::vector<mp_test_type>, ublas::matrix<mp_test_type>, 3> ().test ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double" << std::endl;
    test_blas_2<ublas::vector<double>, ublas::matrix<double>, 3> ().test ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>" << std::endl;
    test_blas_2<ublas::vector<std::complex<mp_test_type> >, ublas::matrix<std::complex<mp_test_type> >, 3> ().test ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>" << std::endl;
    test_blas_2<ublas::vector<std::complex<double> >, ublas::matrix<std::complex<double> >, 3> ().test ();
#endif
#endif

    std::cout << "test_blas_3" << std::endl;

#ifdef USE_FLOAT
    std::cout << "mp_test_type" << std::endl;
    test_blas_3<ublas::matrix<mp_test_type>, 3> ().test ();
#endif

#ifdef USE_DOUBLE
    std::cout << "double" << std::endl;
    test_blas_3<ublas::matrix<double>, 3> ().test ();
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    std::cout << "std::complex<mp_test_type>" << std::endl;
    test_blas_3<ublas::matrix<std::complex<mp_test_type> >, 3> ().test ();
#endif

#ifdef USE_DOUBLE
    std::cout << "std::complex<double>" << std::endl;
    test_blas_3<ublas::matrix<std::complex<double> >, 3> ().test ();
#endif
#endif

    return 0;
}
