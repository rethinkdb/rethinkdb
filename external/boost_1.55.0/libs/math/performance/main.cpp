//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"

#include <map>
#include <set>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <boost/math/tools/config.hpp>
#include <boost/regex.hpp>
#include <boost/type_traits/is_same.hpp>
#include "performance_measure.hpp"
#include <boost/math/policies/policy.hpp>

#ifdef TEST_GSL
#include <gsl/gsl_errno.h>
#include <gsl/gsl_message.h>
#endif


extern void reference_evaluate();

std::map<std::string, double> times;
std::set<test_info> tests;
double total = 0;
int call_count = 0;
bool compiler_prefix = false;

std::set<test_info>& all_tests()
{
   static std::set<test_info> i;
   return i;
}

void add_new_test(test_info i)
{
   all_tests().insert(i);
}

void set_call_count(int i)
{
   call_count = i;
}

void show_help()
{
   std::cout << "Specify on the command line which functions to test.\n"
      "Available options are:\n";
   std::set<test_info>::const_iterator i(all_tests().begin()), j(all_tests().end());
   while(i != j)
   {
      std::cout << "   --" << (*i).name << std::endl;
      ++i;
   }
   std::cout << "Or use --all to test everything." << std::endl;
   std::cout << "You can also specify what to test as a regular expression,\n"
      " for example --dist.* to test all the distributions." << std::endl;
}

void run_tests()
{
   // Get time for empty proceedure:
   double reference_time = performance_measure(reference_evaluate);

   std::set<test_info>::const_iterator i, j;
   for(i = tests.begin(), j = tests.end(); i != j; ++i)
   {
      std::string name;
      if(compiler_prefix)
      {
#if defined(BOOST_MSVC) && !defined(_DEBUG) && !defined(__ICL)
         name = "msvc-";
#elif defined(BOOST_MSVC) && defined(_DEBUG) && !defined(__ICL)
         name = "msvc-debug-";
#elif defined(__GNUC__)
         name = "gcc-";
#elif defined(__ICL)
         name = "intel-";
#elif defined(__ICC)
         name = "intel-linux-";
#endif
      }
      name += i->name;
      set_call_count(1);
      std::cout << "Testing " << std::left << std::setw(50) << name << std::flush;
      double time = performance_measure(i->proc) - reference_time;
      time /= call_count;
      std::cout << std::setprecision(3) << std::scientific << time << std::endl;
   }
}

bool add_named_test(const char* name)
{
   bool found = false;
   boost::regex e(name);
   std::set<test_info>::const_iterator a(all_tests().begin()), b(all_tests().end());
   while(a != b)
   {
      if(regex_match((*a).name, e))
      {
         found = true;
         tests.insert(*a);
      }
      ++a;
   }
   return found;
}

void print_current_config()
{
   std::cout << "Currently, polynomial evaluation uses method " << BOOST_MATH_POLY_METHOD << std::endl;
   std::cout << "Currently, rational function evaluation uses method " << BOOST_MATH_RATIONAL_METHOD << std::endl;
   if(BOOST_MATH_POLY_METHOD + BOOST_MATH_RATIONAL_METHOD > 0)
      std::cout << "Currently, the largest order of polynomial or rational function"
         " that uses a method other than 0, is " << BOOST_MATH_MAX_POLY_ORDER << std::endl;
   bool uses_mixed_tables = boost::is_same<BOOST_MATH_INT_TABLE_TYPE(double, int), int>::value;
   if(uses_mixed_tables)
      std::cout << "Currently, rational functions with integer coefficients are evaluated using mixed integer/real arithmetic" << std::endl;
   else
      std::cout << "Currently, rational functions with integer coefficients are evaluated using all real arithmetic (integer coefficients are actually stored as reals)" << std::endl << std::endl;
   std::cout << "Policies are currently set as follows:\n\n";
   std::cout << "Policy                                  Value\n";
   std::cout << "BOOST_MATH_DOMAIN_ERROR_POLICY          " << BOOST_STRINGIZE(BOOST_MATH_DOMAIN_ERROR_POLICY) << std::endl;
   std::cout << "BOOST_MATH_POLE_ERROR_POLICY            " << BOOST_STRINGIZE(BOOST_MATH_POLE_ERROR_POLICY) << std::endl;
   std::cout << "BOOST_MATH_OVERFLOW_ERROR_POLICY        " << BOOST_STRINGIZE(BOOST_MATH_OVERFLOW_ERROR_POLICY) << std::endl;
   std::cout << "BOOST_MATH_UNDERFLOW_ERROR_POLICY       " << BOOST_STRINGIZE(BOOST_MATH_UNDERFLOW_ERROR_POLICY) << std::endl;
   std::cout << "BOOST_MATH_DENORM_ERROR_POLICY          " << BOOST_STRINGIZE(BOOST_MATH_DENORM_ERROR_POLICY) << std::endl;
   std::cout << "BOOST_MATH_EVALUATION_ERROR_POLICY      " << BOOST_STRINGIZE(BOOST_MATH_EVALUATION_ERROR_POLICY) << std::endl;
   std::cout << "BOOST_MATH_DIGITS10_POLICY              " << BOOST_STRINGIZE(BOOST_MATH_DIGITS10_POLICY) << std::endl;
   std::cout << "BOOST_MATH_PROMOTE_FLOAT_POLICY         " << BOOST_STRINGIZE(BOOST_MATH_PROMOTE_FLOAT_POLICY) << std::endl;
   std::cout << "BOOST_MATH_PROMOTE_DOUBLE_POLICY        " << BOOST_STRINGIZE(BOOST_MATH_PROMOTE_DOUBLE_POLICY) << std::endl;
   std::cout << "BOOST_MATH_DISCRETE_QUANTILE_POLICY     " << BOOST_STRINGIZE(BOOST_MATH_DISCRETE_QUANTILE_POLICY) << std::endl;
   std::cout << "BOOST_MATH_ASSERT_UNDEFINED_POLICY      " << BOOST_STRINGIZE(BOOST_MATH_ASSERT_UNDEFINED_POLICY) << std::endl;
   std::cout << "BOOST_MATH_MAX_SERIES_ITERATION_POLICY  " << BOOST_STRINGIZE(BOOST_MATH_MAX_SERIES_ITERATION_POLICY) << std::endl;
   std::cout << "BOOST_MATH_MAX_ROOT_ITERATION_POLICY    " << BOOST_STRINGIZE(BOOST_MATH_MAX_ROOT_ITERATION_POLICY) << std::endl;
}

extern void sanity_check();

int main(int argc, const char** argv)
{
   //sanity_check();
   try{

#ifdef TEST_GSL
   gsl_set_error_handler_off();
#endif

   if(argc >= 2)
   {
      for(int i = 1; i < argc; ++i)
      {
         if(std::strcmp(argv[i], "--help") == 0)
         {
            show_help();
         }
         else if(std::strcmp(argv[i], "--tune") == 0)
         {
            print_current_config();
            add_named_test("Polynomial-method-0");
            add_named_test("Polynomial-method-1");
            add_named_test("Polynomial-method-2");
            add_named_test("Polynomial-method-3");
            add_named_test("Polynomial-mixed-method-0");
            add_named_test("Polynomial-mixed-method-1");
            add_named_test("Polynomial-mixed-method-2");
            add_named_test("Polynomial-mixed-method-3");
            add_named_test("Rational-method-0");
            add_named_test("Rational-method-1");
            add_named_test("Rational-method-2");
            add_named_test("Rational-method-3");
            add_named_test("Rational-mixed-method-0");
            add_named_test("Rational-mixed-method-1");
            add_named_test("Rational-mixed-method-2");
            add_named_test("Rational-mixed-method-3");
         }
         else if(std::strcmp(argv[i], "--all") == 0)
         {
            print_current_config();
            std::set<test_info>::const_iterator a(all_tests().begin()), b(all_tests().end());
            while(a != b)
            {
               tests.insert(*a);
               ++a;
            }
         }
         else if(std::strcmp(argv[i], "--compiler-prefix") == 0)
         {
            compiler_prefix = true;
         }
         else
         {
            bool found = false;
            if((argv[i][0] == '-') && (argv[i][1] == '-'))
            {
               found = add_named_test(argv[i] + 2);
            }
            if(!found)
            {
               std::cerr << "Unknown option: " << argv[i] << std::endl;
               show_help();
               return 1;
            }
         }
      }
      run_tests();
   }
   else
   {
      show_help();
   }
   //
   // This is just to confuse the optimisers:
   //
   if(argc == 100000)
   {
      std::cerr << total << std::endl;
   }

   }
   catch(const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }

   return 0;
}

void consume_result(double x)
{
   // Do nothing proceedure, don't let whole program optimisation
   // optimise this away - doing so may cause false readings....
   total += x;
}
