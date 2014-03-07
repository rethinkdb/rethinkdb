///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

//
// Compare arithmetic results using fixed_int to GMP results.
//

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/timer.hpp>
#include "test.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/exception/all.hpp>

template <class T>
T generate_random(unsigned bits_wanted)
{
   static boost::random::mt19937 gen;
   typedef boost::random::mt19937::result_type random_type;

   T max_val;
   unsigned digits;
   if(std::numeric_limits<T>::is_bounded && (bits_wanted == (unsigned)std::numeric_limits<T>::digits))
   {
      max_val = (std::numeric_limits<T>::max)();
      digits = std::numeric_limits<T>::digits;
   }
   else
   {
      max_val = T(1) << bits_wanted;
      digits = bits_wanted;
   }

   unsigned bits_per_r_val = std::numeric_limits<random_type>::digits - 1;
   while((random_type(1) << bits_per_r_val) > (gen.max)()) --bits_per_r_val;

   unsigned terms_needed = digits / bits_per_r_val + 1;

   T val = 0;
   for(unsigned i = 0; i < terms_needed; ++i)
   {
      val *= (gen.max)();
      val += gen();
   }
   val %= max_val;
   if(!val)
      val = 1;
   return val;
}

template <class T>
void test_neg(const T& x, const boost::mpl::true_&)
{
   T val = -x;
   try
   {
      std::stringstream ss;
      boost::archive::text_oarchive oa(ss);
      oa << static_cast<const T&>(val);
      boost::archive::text_iarchive ia(ss);
      T val2;
      ia >> val2;
      BOOST_CHECK_EQUAL(val, val2);

      ss.clear();
      boost::archive::binary_oarchive ob(ss);
      ob << static_cast<const T&>(val);
      boost::archive::binary_iarchive ib(ss);
      ib >> val2;
      BOOST_CHECK_EQUAL(val, val2);
   }
   catch(const boost::exception& e)
   {
      std::cout << "Caught boost::exception with:\n";
      std::cout << diagnostic_information(e);
   }
   catch(const std::exception& e)
   {
      std::cout << "Caught std::exception with:\n";
      std::cout << e.what() << std::endl;
   }
}
template <class T>
void test_neg(const T& , const boost::mpl::false_&){}

template <class T>
void test()
{
   using namespace boost::multiprecision;

   boost::random::mt19937 gen;
   boost::uniform_int<> d(3, std::numeric_limits<T>::is_bounded ? std::numeric_limits<T>::digits : 3000);
   boost::timer tim;

   while(true)
   {
      T val(generate_random<typename component_type<T>::type>(d(gen)), generate_random<typename component_type<T>::type>(d(gen)));
      try
      {
         std::stringstream ss;
         boost::archive::text_oarchive oa(ss);
         oa << static_cast<const T&>(val);
         boost::archive::text_iarchive ia(ss);
         T val2;
         ia >> val2;
         BOOST_CHECK_EQUAL(val, val2);

         ss.clear();
         boost::archive::binary_oarchive ob(ss);
         ob << static_cast<const T&>(val);
         boost::archive::binary_iarchive ib(ss);
         ib >> val2;
         BOOST_CHECK_EQUAL(val, val2);
      }
      catch(const boost::exception& e)
      {
         std::cout << "Caught boost::exception with:\n";
         std::cout << diagnostic_information(e);
      }
      catch(const std::exception& e)
      {
         std::cout << "Caught std::exception with:\n";
         std::cout << e.what() << std::endl;
      }
      
      test_neg(val, boost::mpl::bool_<std::numeric_limits<T>::is_signed>());

      //
      // Check to see if test is taking too long.
      // Tests run on the compiler farm time out after 300 seconds,
      // so don't get too close to that:
      //
      if(tim.elapsed() > 150)
      {
         std::cout << "Timeout reached, aborting tests now....\n";
         break;
      }
   }
}

int main()
{
   using namespace boost::multiprecision;
   test<cpp_rational>();
   return boost::report_errors();
}



