///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include "test.hpp"
#include <boost/multiprecision/cpp_dec_float.hpp>

void proc_that_throws()
{
   throw std::runtime_error("");
}

int main()
{
   boost::multiprecision::cpp_dec_float_50 a1(1), a2(1), b(3), c(-2);

   BOOST_WARN(a1);
   BOOST_WARN(a1 == b);
   BOOST_CHECK(a1);
   BOOST_CHECK(a1 == b);
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   boost::multiprecision::cpp_dec_float_50 a_tol = a1 + a1 * 100 * std::numeric_limits<boost::multiprecision::cpp_dec_float_50>::epsilon();
   boost::multiprecision::cpp_dec_float_50 tol = 100 * std::numeric_limits<boost::multiprecision::cpp_dec_float_50>::epsilon();

   BOOST_CHECK_CLOSE(a1, a_tol, tol * 102);  // Passes
   BOOST_WARN_CLOSE(a1, a_tol, tol * 98);    // fails
   BOOST_CHECK_CLOSE(a1, a_tol, tol * 98);    // fails
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   BOOST_CHECK_CLOSE_FRACTION(a1, a_tol, tol * 1.02);  // Passes
   BOOST_WARN_CLOSE_FRACTION(a1, a_tol, tol * 0.98);    // fails
   BOOST_CHECK_CLOSE_FRACTION(a1, a_tol, tol * 0.98);    // fails
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   BOOST_CHECK_EQUAL(a1, a2);
   BOOST_WARN_EQUAL(a1, b);
   BOOST_CHECK_EQUAL(a1, b);
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   BOOST_CHECK_NE(a1, b);
   BOOST_WARN_NE(a1, a2);
   BOOST_CHECK_NE(a1, a2);
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   BOOST_CHECK_GT(a1, c);
   BOOST_WARN_GT(a1, a2);
   BOOST_CHECK_GT(a1, a2);
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   BOOST_CHECK_LT(a1, b);
   BOOST_WARN_LT(a1, a2);
   BOOST_CHECK_LT(a1, a2);
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   BOOST_CHECK_GE(a1, a2);
   BOOST_CHECK_GE(a1, c);
   BOOST_WARN_GE(a1, b);
   BOOST_CHECK_GE(a1, b);
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   BOOST_CHECK_LE(a1, a2);
   BOOST_CHECK_LE(a1, b);
   BOOST_WARN_LE(a1, c);
   BOOST_CHECK_LE(a1, c);
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   BOOST_CHECK_THROW(proc_that_throws(), std::runtime_error);
   BOOST_CHECK_THROW(a1+b+c, std::runtime_error);
   BOOST_CHECK(boost::detail::test_errors() == 1);
   --boost::detail::test_errors();

   return boost::report_errors();
}

