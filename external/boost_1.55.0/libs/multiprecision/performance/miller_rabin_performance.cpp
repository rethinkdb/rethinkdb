///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#define BOOST_CHRONO_HEADER_ONLY

#if !defined(TEST_MPZ) && !defined(TEST_TOMMATH) && !defined(TEST_CPP_INT)
#  define TEST_MPZ
#  define TEST_TOMMATH
#  define TEST_CPP_INT
#endif

#ifdef TEST_MPZ
#include <boost/multiprecision/gmp.hpp>
#endif
#ifdef TEST_TOMMATH
#include <boost/multiprecision/tommath.hpp>
#endif
#ifdef TEST_CPP_INT
#include <boost/multiprecision/cpp_int.hpp>
#endif
#include <boost/multiprecision/miller_rabin.hpp>
#include <boost/chrono.hpp>
#include <map>

template <class Clock>
struct stopwatch
{
   typedef typename Clock::duration duration;
   stopwatch()
   {
      m_start = Clock::now();
   }
   duration elapsed()
   {
      return Clock::now() - m_start;
   }
   void reset()
   {
      m_start = Clock::now();
   }

private:
   typename Clock::time_point m_start;
};

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

#ifdef TEST_MPZ
boost::chrono::duration<double> test_miller_rabin_gmp()
{
   using namespace boost::random;
   using namespace boost::multiprecision;

   stopwatch<boost::chrono::high_resolution_clock> c;

   independent_bits_engine<mt11213b, 256, mpz_int> gen;

   for(unsigned i = 0; i < 1000; ++i)
   {
      mpz_int n = gen();
      mpz_probab_prime_p(n.backend().data(), 25);
   }
   return c.elapsed();
}
#endif

std::map<std::string, double> results;
double min_time = (std::numeric_limits<double>::max)();

template <class IntType>
boost::chrono::duration<double> test_miller_rabin(const char* name)
{
   using namespace boost::random;

   stopwatch<boost::chrono::high_resolution_clock> c;

   independent_bits_engine<mt11213b, 256, IntType> gen;
   //
   // We must use a different generator for the tests and number generation, otherwise
   // we get false positives.
   //
   mt19937 gen2;
   unsigned result_count = 0;

   for(unsigned i = 0; i < 1000; ++i)
   {
      IntType n = gen();
      if(boost::multiprecision::miller_rabin_test(n, 25, gen2))
         ++result_count;
   }
   boost::chrono::duration<double> t = c.elapsed();
   double d = t.count();
   if(d < min_time)
      min_time = d;
   results[name] = d;
   std::cout << "Time for " << std::setw(30) << std::left << name << " = " << d << std::endl;
   std::cout << "Number of primes found = " << result_count << std::endl;
   return t;
}

void generate_quickbook()
{
   std::cout << "[table\n[[Integer Type][Relative Performance (Actual time in parenthesis)]]\n";

   std::map<std::string, double>::const_iterator i(results.begin()), j(results.end());

   while(i != j)
   {
      double rel = i->second / min_time;
      std::cout << "[[" << i->first << "][" << rel << "(" << i->second << "s)]]\n";
      ++i;
   }
   
   std::cout << "]\n";
}

int main()
{
   using namespace boost::multiprecision;
#ifdef TEST_CPP_INT
   test_miller_rabin<number<cpp_int_backend<>, et_off> >("cpp_int (no Expression templates)");
   test_miller_rabin<cpp_int>("cpp_int");
   test_miller_rabin<number<cpp_int_backend<128> > >("cpp_int (128-bit cache)");
   test_miller_rabin<number<cpp_int_backend<256> > >("cpp_int (256-bit cache)");
   test_miller_rabin<number<cpp_int_backend<512> > >("cpp_int (512-bit cache)");
   test_miller_rabin<number<cpp_int_backend<1024> > >("cpp_int (1024-bit cache)");
   test_miller_rabin<int1024_t>("int1024_t");
   test_miller_rabin<checked_int1024_t>("checked_int1024_t");
#endif
#ifdef TEST_MPZ
   test_miller_rabin<number<gmp_int, et_off> >("mpz_int (no Expression templates)");
   test_miller_rabin<mpz_int>("mpz_int");
   std::cout << "Time for mpz_int (native Miller Rabin Test) = " << test_miller_rabin_gmp() << std::endl;
#endif
#ifdef TEST_TOMMATH
   test_miller_rabin<number<boost::multiprecision::tommath_int, et_off> >("tom_int (no Expression templates)");
   test_miller_rabin<boost::multiprecision::tom_int>("tom_int");
#endif

   generate_quickbook();

   return 0;
}

