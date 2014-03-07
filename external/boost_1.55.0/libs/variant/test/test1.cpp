//-----------------------------------------------------------------------------
// boost-libs variant/test/test1.cpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Friedman, Itay Maman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "boost/config.hpp"

#ifdef BOOST_MSVC
#pragma warning(disable:4244) // conversion from const int to const short
#endif

#include "boost/test/minimal.hpp"
#include "boost/variant.hpp"

#include "class_a.h"
#include "jobs.h"

#include <iostream>
#include <string>
#include <vector>



void run()
{

   using boost::apply_visitor;
   using boost::variant;
   using std::string;
   using std::vector;
   using std::cout;
   using std::endl;

   typedef variant< char*, string, short > t_var0;
   typedef variant< int, string, double > t_var1;
   typedef variant< short, const char* > t_var2;
   typedef variant< string, char > t_var3;   
   typedef variant< unsigned short, const char* > t_var4;
   typedef variant< unsigned short, const char*, t_var2 > t_var5;
   typedef variant< unsigned short, const char*, t_var5 > t_var6;
   typedef variant< class_a, const void* > t_var7;
   typedef variant< t_var6, int > t_var8;
   typedef variant< t_var8, unsigned short > t_var9;
   typedef variant< char, unsigned char > t_var10;
   typedef variant< short, int, vector<int>, long> t_var11;

   t_var1 v1;
   t_var0 v0;
   t_var2 v2;
   t_var3 v3;
   t_var4 v4;
   t_var5 v5;
   t_var6 v6;
   t_var7 v7;
   t_var8 v8;
   t_var9 v9;
   t_var10 v10;
   t_var11 v11;


   //
   // Check assignment rules 
   //

   v2 = 4;
   v4 = v2;
   verify(v4, spec<unsigned short>());

   v2 = "abc";
   v4 = v2;
   verify(v4, spec<const char*>(), "[V] abc");

   v5 = "def";
   verify(v5, spec<const char*>(), "[V] def");

   v5 = v2;
   verify(v5, spec<t_var2>(), "[V] [V] abc");

   v6 = 58;
   verify(v6, spec<unsigned short>(), "[V] 58");

   v6 = v5;
   verify(v6, spec<t_var5>(), "[V] [V] [V] abc");

   v8 = v2;
   verify(v8, spec<t_var6>(), "[V] [V] abc");

   v8 = v6;
   verify(v8, spec<t_var6>(), "[V] [V] [V] [V] abc");

   v7 = v2;
   verify(v7, spec<const void*>());

   v7 = 199;
   verify(v7, spec<class_a>(), "[V] class_a(199)");

   v2 = 200;
   v7 = v2;
   verify(v7, spec<class_a>(), "[V] class_a(200)");



   //
   // Check sizes of held values
   //
   total_sizeof ts;

   v1 = 5.9;
   apply_visitor(ts, v1);

   v1 = 'B';
   apply_visitor(ts, v1);

   v1 = 3.4f;
   apply_visitor(ts, v1);

   BOOST_CHECK(ts.result() == sizeof(int) + sizeof(double)*2);

   v11 = 5;
   string res_s = apply_visitor(int_printer(), v11);
   BOOST_CHECK(res_s == "5");

   //
   // A variant object holding an std::vector 
   //
   vector<int> int_vec_1;
   int_vec_1.push_back(512);
   int_vec_1.push_back(256);
   int_vec_1.push_back(128);
   int_vec_1.push_back(64);

   v11 = int_vec_1;
   res_s = apply_visitor(int_printer(), v11);
   BOOST_CHECK(res_s == ",512,256,128,64");
}



int test_main(int , char* [])
{
   run();
   return 0;
}
