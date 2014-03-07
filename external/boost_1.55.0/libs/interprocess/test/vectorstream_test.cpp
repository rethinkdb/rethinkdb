//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <sstream>
#include <cstring>
#include <vector>
#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <stdio.h>

namespace boost {
namespace interprocess {

//Force instantiations to catch compile-time errors
typedef basic_string<char> my_string;
typedef basic_vectorstream<my_string > my_stringstream_t;
typedef vector<char> my_vector;
typedef basic_vectorstream<my_vector>  my_vectorstream_t;
template class basic_vectorstream<my_string>;
template class basic_vectorstream<std::vector<char> >;

}}

using namespace boost::interprocess;

static int vectorstream_test()
{
   {  //Test high watermarking initialization
      my_stringstream_t my_stringstream;
      int a (0);
      my_stringstream << 11;
      my_stringstream >> a;
      if(a != 11)
         return 1;
   }
   {  //Test high watermarking initialization
      my_vectorstream_t my_stringstream;
      int a (0);
      my_stringstream << 13;
      my_stringstream >> a;
      if(a != 13)
         return 1;
   }

   //Pre-reserved string
   {
      my_stringstream_t my_stringstream;
      std::stringstream std_stringstream;
      std::string str1, str2, str3("testline:");
      int number1, number2;

      my_stringstream.reserve(10000);
      for(int i = 0; i < 100; ++i){
         my_stringstream  << "testline: " << i << std::endl;
         std_stringstream << "testline: " << i << std::endl;
      }

      if(std::strcmp(my_stringstream.vector().c_str(), std_stringstream.str().c_str()) != 0){
         return 1;
      }

      for(int i = 0; i < 100; ++i){
         my_stringstream  >> str1 >> number1;
         std_stringstream >> str2 >> number2;
         if((str1 != str2) || (str1 != str3)){
            assert(0); return 1;
         }
         if((number1 != number2) || (number1 != i)){
            assert(0); return 1;
         }
      }
   }
   //Pre-reserved vector
   {
      basic_vectorstream<std::vector<char> > my_vectorstream;
      std::vector<char> myvector;
      std::stringstream std_stringstream;
      std::string str1, str2, str3("testline:");
      int number1, number2;

      my_vectorstream.reserve(10000);
      for(int i = 0; i < 100; ++i){
         my_vectorstream  << "testline: " << i << std::endl;
         std_stringstream << "testline: " << i << std::endl;
      }
      //Add final null to form a c string
      myvector.push_back(0);
      if(std::strcmp(&(my_vectorstream.vector()[0]), std_stringstream.str().c_str()) != 0){
         return 1;
      }
      myvector.pop_back();
      for(int i = 0; i < 100; ++i){
         my_vectorstream  >> str1 >> number1;
         std_stringstream >> str2 >> number2;
         if((str1 != str2) || (str1 != str3)){
            assert(0); return 1;
         }
         if((number1 != number2) || (number1 != i)){
            assert(0); return 1;
         }
      }
   }

   //No pre-reserved or pre-reserved string
   {
      my_stringstream_t my_stringstream;
      std::stringstream std_stringstream;
      std::string str1, str2, str3("testline:");
      int number1, number2;

      for(int i = 0; i < 100; ++i){
         my_stringstream  << "testline: " << i << std::endl;
         std_stringstream << "testline: " << i << std::endl;
      }
      if(std::strcmp(my_stringstream.vector().c_str(), std_stringstream.str().c_str()) != 0){
         assert(0);   return 1;
      }
      for(int i = 0; i < 100; ++i){
         my_stringstream  >> str1 >> number1;
         std_stringstream >> str2 >> number2;
         if((str1 != str2) || (str1 != str3)){
            assert(0); return 1;
         }
         if((number1 != number2) || (number1 != i)){
            assert(0); return 1;
         }
      }
   }

   //Test seek
   {
      my_stringstream_t my_stringstream;
      my_stringstream << "ABCDEFGHIJKLM";
      my_stringstream.seekp(0);
      my_stringstream << "PQRST";
      string s("PQRSTFGHIJKLM");
      if(s != my_stringstream.vector()){
         return 1;
      }
      my_stringstream.seekp(0, std::ios_base::end);
      my_stringstream << "NOPQRST";
      s ="PQRSTFGHIJKLMNOPQRST";
      if(s != my_stringstream.vector()){
         return 1;
      }
      int size = static_cast<int>(my_stringstream.vector().size());
      my_stringstream.seekp(-size, std::ios_base::cur);
      my_stringstream << "ABCDE";
      s ="ABCDEFGHIJKLMNOPQRST";
      if(s != my_stringstream.vector()){
         return 1;
      }
   }
   return 0;
}

int main ()
{
   if(vectorstream_test()){
      return 1;
   }
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
