//  Copyright (C) 2011 John Maddock
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX
//  TITLE:         C++0x unified initialization syntax unavailable
//  DESCRIPTION:   The compiler does not support C++0x unified initialization syntax: see http://en.wikipedia.org/wiki/C%2B%2B0x#Uniform_initialization

#include <string>

namespace boost_no_cxx11_unified_initialization_syntax {

struct BasicStruct
{
   int x;
   double y;
};

struct AltStruct
{
public:
   AltStruct(int x, double y) : x_{x}, y_{y} {}
private:
   int x_;
   double y_;
};

struct IdString
{
   std::string name;
   int identifier;
   bool operator == (const IdString& other)
   {
      return identifier == other.identifier && name == other.name;
   }
};

IdString get_string()
{
   return {"SomeName", 4}; //Note the lack of explicit type.
}

int test()
{
   BasicStruct var1{5, 3.2};
   AltStruct var2{2, 4.3};
  (void) var1;
  (void) var2;

   IdString id{"SomeName", 4};
   return id == get_string() ? 0 : 1;
}

}
