//-----------------------------------------------------------------------------
// boost-libs variant/test/test8.cpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Friedman, Itay Maman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "boost/test/minimal.hpp"
#include "boost/variant.hpp"

#include <iostream>
#include <vector>
#include <string>

using namespace std;
using namespace boost;

typedef variant<float, std::string, int, std::vector<std::string> > t_var1;

struct int_sum : static_visitor<>
{
   int_sum() : result_(0) { }

   void operator()(int t) 
   {
      result_ += t;
   }

   result_type operator()(float ) { }
   result_type operator()(const std::string& ) { }
   result_type operator()(const std::vector<std::string>& ) { }

   int result_;
}; 

template <typename T, typename Variant>
T& check_pass(Variant& v, T value)
{
    BOOST_CHECK(get<T>(&v));

    try
    {
        T& r = get<T>(v);
        BOOST_CHECK(r == value);
        return r;
    }
    catch(boost::bad_get&)
    {
        throw; // must never reach
    }
}

template <typename T, typename Variant>
void check_fail(Variant& v)
{
    BOOST_CHECK(!get<T>(&v));

    try
    {
        T& r = get<T>(v);
        (void)r; // suppress warning about r not being used
        BOOST_CHECK(false && &r); // should never reach
    }
    catch(boost::bad_get&)
    {
        // (do nothing here)
    }
}

int test_main(int , char* [])
{
   int_sum acc;
   t_var1 v1 = 800;

   // check get on non-const variant
   {
      int& r1 = check_pass<int>(v1, 800);
      const int& cr1 = check_pass<const int>(v1, 800);

      check_fail<float>(v1);
      check_fail<const float>(v1);
      check_fail<short>(v1);
      check_fail<const short>(v1);

      apply_visitor(acc, v1);
      BOOST_CHECK(acc.result_ == 800);

      r1 = 920; // NOTE: modifies content of v1
      apply_visitor(acc, v1);
      BOOST_CHECK(cr1 == 920);
      BOOST_CHECK(acc.result_ == 800 + 920);
   }

   // check const correctness:
   {
      const t_var1& c = v1;

      check_pass<const int>(c, 920);

      //check_fail<int>(c);
      check_fail<const float>(c);
      //check_fail<float>(c);
      check_fail<const short>(c);
      //check_fail<short>(c);
   }

   return boost::exit_success;
}

