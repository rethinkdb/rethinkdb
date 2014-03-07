//-----------------------------------------------------------------------------
// boost-libs variant/test/test2.cpp header file
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
#pragma warning(disable:4244) // conversion from 'const int' to 'const short'
#endif

#include "boost/test/minimal.hpp"
#include "boost/variant.hpp"

#include "jobs.h"

#include <cassert>
#include <iostream>
#include <algorithm>
#include <cstring>

using boost::apply_visitor;

struct short_string
{
   BOOST_STATIC_CONSTANT(size_t, e_limit = 101);

   short_string() : len_(0) 
   { 
      buffer_[0] = '\0';
   }

   short_string(const char* src) 
   {
#ifndef BOOST_NO_STDC_NAMESPACE
      using std::strlen;
#endif // BOOST_NO_STDC_NAMESPACE

      size_t limit = this->e_limit; // avoid warnings on some compilers
      size_t src_len = strlen(src);
      
      len_ = (std::min)(src_len, limit-1);
      std::copy(src, src + len_, buffer_);
      buffer_[len_] = '\0';
   }

   short_string(const short_string& other) : len_(other.len_)
   {
      std::copy(other.buffer_, other.buffer_ + e_limit, buffer_);
   }

   void swap(short_string& other)
   {
      char temp[e_limit];

      std::copy(buffer_, buffer_ + e_limit, temp);
      std::copy(other.buffer_, other.buffer_ + e_limit, buffer_);
      std::copy(temp, temp + e_limit, other.buffer_);

      std::swap(len_, other.len_);
   }

   short_string& operator=(const short_string& rhs)
   {
      short_string temp(rhs);
      swap(temp);

      return *this;
   }

   operator const char*() const
   {
      return buffer_;
   }


private:
   char buffer_[e_limit];
   size_t len_;
}; //short_string


std::ostream& operator<<(std::ostream& out, const short_string& s)
{
   out << static_cast<const char*>(s);
   return out;
}



void run()
{   
   using boost::variant;

   variant<short, short_string> v0;
   variant<char, const char*> v1;
   variant<short_string, char > v2;

   //
   // Default construction
   //
   verify(v0, spec<short>());
   verify(v1, spec<char>());
   verify(v2, spec<short_string>());

   //
   // Implicit conversion to bounded type
   //
   v1 = "I am v1";
   verify(v1, spec<const char*>(), "[V] I am v1");

   v2 = "I am v2";
   verify(v2, spec<short_string>(), "[V] I am v2");

   //
   // Variant-to-variant assignment
   //

   v0 = v1;
   verify(v0, spec<short_string>(), "[V] I am v1");

   v1 = v0;
   verify(v1, spec<const char*>(), "[V] I am v1");

   const int n0 = 88;
   v1 = n0;
   v0 = v1;

   //
   // Implicit conversion to bounded type
   //
   verify(v0, spec<short>(), "[V] 88");
   verify(v1, spec<char>(), "[V] X");
}


int test_main(int , char* [])
{
   run();
   return 0;
}

