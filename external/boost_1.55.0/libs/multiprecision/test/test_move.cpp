// Copyright John Maddock 2012.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/config.hpp>

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

#if !defined(TEST_GMP) && !defined(TEST_MPFR) && !defined(TEST_TOMMATH) && !defined(TEST_CPP_INT)
#  define TEST_GMP
#  define TEST_MPFR
#  define TEST_TOMMATH
#  define TEST_CPP_INT

#ifdef _MSC_VER
#pragma message("CAUTION!!: No backend type specified so testing everything.... this will take some time!!")
#endif
#ifdef __GNUC__
#pragma warning "CAUTION!!: No backend type specified so testing everything.... this will take some time!!"
#endif

#endif

#if defined(TEST_GMP)
#include <boost/multiprecision/gmp.hpp>
#endif
#if defined(TEST_MPFR)
#include <boost/multiprecision/mpfr.hpp>
#endif
#ifdef TEST_TOMMATH
#include <boost/multiprecision/tommath.hpp>
#endif
#ifdef TEST_CPP_INT
#include <boost/multiprecision/cpp_int.hpp>
#endif

#include "test.hpp"

unsigned allocation_count = 0;

void *(*alloc_func_ptr) (size_t);
void *(*realloc_func_ptr) (void *, size_t, size_t);
void (*free_func_ptr) (void *, size_t);

void *alloc_func(size_t n)
{
   ++allocation_count;
   return (*alloc_func_ptr)(n);
}

void free_func(void * p, size_t n)
{
   (*free_func_ptr)(p, n);
}

void * realloc_func(void * p, size_t old, size_t n)
{
   ++allocation_count;
   return (*realloc_func_ptr)(p, old, n);
}

template <class T>
void do_something(const T&)
{
}

template <class T>
void test_std_lib()
{
   std::vector<T> v;
   for(unsigned i = 0; i < 100; ++i)
      v.insert(v.begin(), i);

   T a(2), b(3);
   std::swap(a, b);
   BOOST_TEST(a == 3);
   BOOST_TEST(b == 2);
}

template <class T, class A>
void test_move_and_assign(T x, A val)
{
   // move away from x, then assign val to x.
   T z(x);
   T y(std::move(x));
   x.assign(val);
   BOOST_CHECK_EQUAL(x, T(val));
   BOOST_CHECK_EQUAL(z, y);
}

template <class T>
void test_move_and_assign()
{
   T x(23);
   test_move_and_assign(x, static_cast<short>(2));
   test_move_and_assign(x, static_cast<int>(2));
   test_move_and_assign(x, static_cast<long>(2));
   test_move_and_assign(x, static_cast<long long>(2));
   test_move_and_assign(x, static_cast<unsigned short>(2));
   test_move_and_assign(x, static_cast<unsigned int>(2));
   test_move_and_assign(x, static_cast<unsigned long>(2));
   test_move_and_assign(x, static_cast<unsigned long long>(2));
   test_move_and_assign(x, static_cast<float>(2));
   test_move_and_assign(x, static_cast<double>(2));
   test_move_and_assign(x, static_cast<long double>(2));
   test_move_and_assign(x, x);
   test_move_and_assign(x, "23");
}


int main()
{
#if defined(TEST_MPFR) || defined(TEST_GMP)
   mp_get_memory_functions(&alloc_func_ptr, &realloc_func_ptr, &free_func_ptr);
   mp_set_memory_functions(&alloc_func, &realloc_func, &free_func);
#endif

   using namespace boost::multiprecision;

#ifdef TEST_MPFR
   {
      test_std_lib<mpfr_float_50>();
      mpfr_float_50 a = 2;
      BOOST_TEST(allocation_count); // sanity check that we are tracking allocations
      allocation_count = 0;
      mpfr_float_50 b = std::move(a);
      BOOST_TEST(allocation_count == 0);
      //
      // Move assign - we rely on knowledge of the internals to make this test work!!
      //
      mpfr_float_50 c(3);
      do_something(b);
      do_something(c);
      const void* p = b.backend().data()[0]._mpfr_d;
      BOOST_TEST(c.backend().data()[0]._mpfr_d != p);
      c = std::move(b);
      BOOST_TEST(c.backend().data()[0]._mpfr_d == p);
      BOOST_TEST(b.backend().data()[0]._mpfr_d != p);
      //
      // Again with variable precision, this we can test more easily:
      //
      mpfr_float d, e;
      d.precision(100);
      e.precision(1000);
      d = 2;
      e = 3;
      allocation_count = 0;
      BOOST_TEST(d == 2);
      d = std::move(e);
      BOOST_TEST(allocation_count == 0);
      BOOST_TEST(d == 3);
      e = 2;
      BOOST_TEST(e == 2);
      d = std::move(e);
      e = d;
      BOOST_TEST(e == d);

      test_move_and_assign<mpfr_float>();
      test_move_and_assign<mpfr_float_50>();
   }
#endif
#ifdef TEST_GMP
   {
      test_std_lib<mpf_float_50>();
      mpf_float_50 a = 2;
      BOOST_TEST(allocation_count); // sanity check that we are tracking allocations
      allocation_count = 0;
      mpf_float_50 b = std::move(a);
      BOOST_TEST(allocation_count == 0);
      //
      // Move assign: this requires knowledge of the internals to test!!
      //
      mpf_float_50 c(3);
      do_something(b);
      do_something(c);
      const void* p = b.backend().data()[0]._mp_d;
      BOOST_TEST(c.backend().data()[0]._mp_d != p);
      c = std::move(b);
      BOOST_TEST(c.backend().data()[0]._mp_d == p);
      BOOST_TEST(b.backend().data()[0]._mp_d != p);
      //
      // Again with variable precision, this we can test more easily:
      //
      mpf_float d, e;
      d.precision(100);
      e.precision(1000);
      d = 2;
      e = 3;
      allocation_count = 0;
      BOOST_TEST(d == 2);
      d = std::move(e);
      BOOST_TEST(allocation_count == 0);
      BOOST_TEST(d == 3);
      e = 2;
      BOOST_TEST(e == 2);
      d = std::move(e);
      e = d;
      BOOST_TEST(e == d);

      test_move_and_assign<mpf_float>();
      test_move_and_assign<mpf_float_50>();
   }
   {
      test_std_lib<mpz_int>();
      mpz_int a = 2;
      BOOST_TEST(allocation_count); // sanity check that we are tracking allocations
      allocation_count = 0;
      mpz_int b = std::move(a);
      BOOST_TEST(allocation_count == 0);

      //
      // Move assign:
      //
      mpz_int d, e;
      d = 2;
      d <<= 1000;
      e = 3;
      allocation_count = 0;
      e = std::move(d);
      BOOST_TEST(allocation_count == 0);
      e = 2;
      BOOST_TEST(e == 2);
      d = std::move(e);
      e = d;
      BOOST_TEST(e == d);

      test_move_and_assign<mpz_int>();
   }
   {
      test_std_lib<mpq_rational>();
      mpq_rational a = 2;
      BOOST_TEST(allocation_count); // sanity check that we are tracking allocations
      allocation_count = 0;
      mpq_rational b = std::move(a);
      BOOST_TEST(allocation_count == 0);

      //
      // Move assign:
      //
      mpq_rational d, e;
      d = mpz_int(2) << 1000;
      e = 3;
      allocation_count = 0;
      e = std::move(d);
      BOOST_TEST(allocation_count == 0);
      d = 2;
      BOOST_TEST(d == 2);
      d = std::move(e);
      e = d;
      BOOST_TEST(e == d);

      test_move_and_assign<mpq_rational>();
   }
#endif
#ifdef TEST_TOMMATH
   {
      test_std_lib<tom_int>();
      tom_int a = 2;
      void const* p = a.backend().data().dp;
      tom_int b = std::move(a);
      BOOST_TEST(b.backend().data().dp == p);
      // We can't test this, as it will assert inside data():
      //BOOST_TEST(a.backend().data().dp == 0);

      //
      // Move assign:
      //
      tom_int d, e;
      d = 2;
      d <<= 1000;
      e = 3;
      p = d.backend().data().dp;
      BOOST_TEST(p != e.backend().data().dp);
      e = std::move(d);
      BOOST_TEST(e.backend().data().dp == p);
      d = 2;
      BOOST_TEST(d == 2);
      d = std::move(e);
      e = d;
      BOOST_TEST(e == d);

      test_move_and_assign<tom_int>();
   }
#endif
#ifdef TEST_CPP_INT
   {
      test_std_lib<cpp_int>();
      cpp_int a = 2;
      a <<= 1000;  // Force dynamic allocation.
      void const* p = a.backend().limbs();
      cpp_int b = std::move(a);
      BOOST_TEST(b.backend().limbs() == p);

      //
      // Move assign:
      //
      cpp_int d, e;
      d = 2;
      d <<= 1000;
      e = 3;
      e <<= 1000;
      p = d.backend().limbs();
      BOOST_TEST(p != e.backend().limbs());
      e = std::move(d);
      BOOST_TEST(e.backend().limbs() == p);
      d = 2;
      BOOST_TEST(d == 2);
      d = std::move(e);
      e = d;
      BOOST_TEST(e == d);

      test_move_and_assign<cpp_int>();
      test_move_and_assign<int512_t>();
   }
#endif
   return boost::report_errors();
}

#else
//
// No rvalue refs, nothing to test:
//
int main()
{
   return 0;
}

#endif

