///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void basic_tests()
{

   std::cout << "Allocation Counts for Horner Evaluation:\n";
#ifdef TEST_MPFR
   basic_allocation_test("mpfr_float_50", mpfr_float_50(2));
   basic_allocation_test("mpfr_float_50 - no expression templates", number<mpfr_float_backend<50>, et_off>(2));
#endif
#ifdef TEST_MPFR_CLASS
   basic_allocation_test("mpfr_class", mpfr_class(2));
#endif
#ifdef TEST_MPREAL
   basic_allocation_test("mpfr::mpreal", mpfr::mpreal(2));
#endif

   std::cout << "Allocation Counts for boost::math::tools::evaluate_polynomial:\n";
#ifdef TEST_MPFR
   poly_allocation_test("mpfr_float_50", mpfr_float_50(2));
   poly_allocation_test("mpfr_float_50 - no expression templates", number<mpfr_float_backend<50>, et_off>(2));
#endif
#ifdef TEST_MPFR_CLASS
   poly_allocation_test("mpfr_class", mpfr_class(2));
#endif
#ifdef TEST_MPREAL
   poly_allocation_test("mpfr::mpreal", mpfr::mpreal(2));
#endif

   //
   // Comparison for builtin floats:
   //
#ifdef TEST_FLOAT
   time_proc("Bessel Functions - double", test_bessel<double>);
   time_proc("Bessel Functions - real_concept", test_bessel<boost::math::concepts::real_concept>);
   time_proc("Bessel Functions - arithmetic_backend<double>", test_bessel<number<arithmetic_backend<double>, et_on> >);
   time_proc("Bessel Functions - arithmetic_backend<double> - no expression templates", test_bessel<number<arithmetic_backend<double>, et_off> >);

   time_proc("Non-central T - double", test_nct<double>);
   time_proc("Non-central T - real_concept", test_nct<boost::math::concepts::real_concept>);
   time_proc("Non-central T - arithmetic_backend<double>", test_nct<number<arithmetic_backend<double>, et_on> >);
   time_proc("Non-central T - arithmetic_backend<double> - no expression templates", test_nct<number<arithmetic_backend<double>, et_off> >);
#endif
}
