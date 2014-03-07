//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_REGEX_TEST_STD

#ifdef TEST_STD_HEADERS
#include <regex>
#else
#include <boost/tr1/regex.hpp>
#endif
#define BOOST_TEST_TR1_REGEX
#include <boost/regex/concepts.hpp>

int main()
{
   boost::function_requires<
      boost::RegexTraitsConcept<
         std::tr1::regex_traits<char>
      >
   >();

   boost::function_requires<
      boost::RegexConcept<
         std::tr1::basic_regex<char>
      >
   >();

   boost::function_requires<
      boost::RegexConcept<
         std::tr1::basic_regex<wchar_t>
      >
   >();

   //
   // now test the regex_traits concepts:
   //
   typedef std::tr1::basic_regex<char, boost::regex_traits_architype<char> > regex_traits_tester_type1;
   boost::function_requires<
      boost::RegexConcept<
         regex_traits_tester_type1
      >
   >();
   return 0;
}

