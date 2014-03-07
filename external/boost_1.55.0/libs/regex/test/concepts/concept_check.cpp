/*
 *
 * Copyright (c) 2003
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <boost/config.hpp>

#if defined(BOOST_MSVC)
// this lets us compile at warning level 4 without seeing concept-check related warnings
#  pragma warning(disable:4100)
#endif
#ifdef __BORLANDC__
#pragma option -w-8019 -w-8004 -w-8008
#endif
#ifdef BOOST_INTEL
#pragma warning(disable:1418 981 983 595 383)
#endif

#include <boost/regex.hpp>
#include <boost/detail/workaround.hpp>
#if !BOOST_WORKAROUND(_MSC_VER, < 1310) && !defined(BOOST_NO_MEMBER_TEMPLATES) && !defined(__IBMCPP__) && !BOOST_WORKAROUND(__GNUC__, < 3)
#include <boost/regex/concepts.hpp>
#endif


int main()
{
   // VC6 and VC7 can't cope with the iterator architypes, 
   // don't bother testing as it doesn't work:
#if !BOOST_WORKAROUND(_MSC_VER, < 1310) && !defined(BOOST_NO_MEMBER_TEMPLATES) && !defined(__IBMCPP__) && !BOOST_WORKAROUND(__GNUC__, < 3)
   boost::function_requires<
      boost::RegexTraitsConcept<
         boost::regex_traits<char>
      >
   >();
#ifndef BOOST_NO_STD_LOCALE
   boost::function_requires<
      boost::BoostRegexConcept<
         boost::basic_regex<char, boost::cpp_regex_traits<char> >
      >
   >();
#ifndef BOOST_NO_WREGEX
   boost::function_requires<
      boost::BoostRegexConcept<
         boost::basic_regex<wchar_t, boost::cpp_regex_traits<wchar_t> >
      >
   >();
#endif
#endif
#if !BOOST_WORKAROUND(__BORLANDC__, < 0x560)
   boost::function_requires<
      boost::BoostRegexConcept<
         boost::basic_regex<char, boost::c_regex_traits<char> >
      >
   >();
#ifndef BOOST_NO_WREGEX
   boost::function_requires<
      boost::BoostRegexConcept<
         boost::basic_regex<wchar_t, boost::c_regex_traits<wchar_t> >
      >
   >();
#endif
#endif
#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
   boost::function_requires<
      boost::BoostRegexConcept<
         boost::basic_regex<char, boost::w32_regex_traits<char> >
      >
   >();
#ifndef BOOST_NO_WREGEX
   boost::function_requires<
      boost::BoostRegexConcept<
         boost::basic_regex<wchar_t, boost::w32_regex_traits<wchar_t> >
      >
   >();
#endif
#endif
   //
   // now test the regex_traits concepts:
   //
   typedef boost::basic_regex<char, boost::regex_traits_architype<char> > regex_traits_tester_type1;
   boost::function_requires<
      boost::BoostRegexConcept<
         regex_traits_tester_type1
      >
   >();
#if !defined(__MWERKS__) && !defined(__SUNPRO_CC) // MWCW tries to instantiate std::basic_string<boost::char_architype>, not sure whose bug this is....
   typedef boost::basic_regex<boost::char_architype, boost::regex_traits_architype<boost::char_architype> > regex_traits_tester_type2;
   boost::function_requires<
      boost::BaseRegexConcept<
         regex_traits_tester_type2
      >
   >();
#endif // __MWERKS__
#endif   
   return 0;
}


