//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#define BOOST_CONTAINER_USER_DEFINED_THROW_CALLBACKS

#include <boost/container/detail/config_begin.hpp>

#include <boost/container/throw_exception.hpp>
#include <boost/detail/lightweight_test.hpp>

using namespace boost::container;

static bool bad_alloc_called     = false;
static bool out_of_range_called  = false;
static bool length_error_called  = false;
static bool logic_error_called   = false;
static bool runtime_error_called = false;

//User defined throw implementations
namespace boost {
namespace container {

   void throw_bad_alloc()
   {  bad_alloc_called = true;   }

   void throw_out_of_range(const char* str)
   {  (void)str; out_of_range_called = true;   }

   void throw_length_error(const char* str)
   {  (void)str; length_error_called = true;   }

   void throw_logic_error(const char* str)
   {  (void)str; logic_error_called = true; }

   void throw_runtime_error(const char* str)
   {  (void)str; runtime_error_called = true;  }

}} //boost::container

int main()
{
   //Check user-defined throw callbacks are called
   throw_bad_alloc();
   BOOST_TEST(bad_alloc_called == true);
   throw_out_of_range("dummy");
   BOOST_TEST(out_of_range_called == true);
   throw_length_error("dummy");
   BOOST_TEST(length_error_called == true);
   throw_logic_error("dummy");
   BOOST_TEST(logic_error_called == true);
   throw_runtime_error("dummy");
   BOOST_TEST(runtime_error_called == true);
   return ::boost::report_errors();
}

#include <boost/container/detail/config_end.hpp>
