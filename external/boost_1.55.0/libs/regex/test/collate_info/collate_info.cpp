/*
 *
 * Copyright (c) 2005
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

// most of the workarounds and headers we need are already in here:
#include <boost/regex.hpp> 
#include <boost/regex/v4/primary_transform.hpp>
#include <assert.h>

#ifdef BOOST_INTEL
#pragma warning(disable:1418 981 983 2259)
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{
   using ::strxfrm;
#ifndef BOOST_NO_WREGEX
   using ::wcsxfrm;
#endif
}
#endif

#include <iostream>

template <class charT>
int make_int(charT c)
{
   return c;
}

int make_int(char c)
{
   return static_cast<unsigned char>(c);
}

template <class charT>
void print_string(const std::basic_string<charT>& s)
{
   typedef typename std::basic_string<charT>::size_type size_type;
   std::cout.put(static_cast<unsigned char>('"'));
   for(size_type i = 0; i < s.size(); ++i)
   {
      if((s[i] > ' ') && (s[i] <= 'z'))
      {
         std::cout.put(static_cast<unsigned char>(s[i]));
      }
      else
      {
         std::cout << "\\x" << std::hex << make_int(s[i]);
      }
   }
   std::cout.put(static_cast<unsigned char>('"'));
}

void print_c_char(char c)
{
   char buf[50];
   const char cbuf[2] = { c, 0, };
   std::size_t len = std::strxfrm(buf, cbuf, 50);
   std:: cout << len << "   ";
   std::string s(buf);
   print_string(s);
}
#ifndef BOOST_NO_WREGEX
void print_c_char(wchar_t c)
{
   wchar_t buf[50];
   const wchar_t cbuf[2] = { c, 0, };
   std::size_t len = std::wcsxfrm(buf, cbuf, 50);
   std:: cout << len << "   ";
   std::wstring s(buf);
   print_string(s);
}
#endif
template <class charT>
void print_c_info(charT, const char* name)
{
   std::cout << "Info for " << name << " C API's:" << std::endl;
   std::cout << "   \"a\"  :  ";
   print_c_char(charT('a'));
   std::cout << std::endl;
   std::cout << "   \"A\"  :  ";
   print_c_char(charT('A'));
   std::cout << std::endl;
   std::cout << "   \"z\"  :  ";
   print_c_char(charT('z'));
   std::cout << std::endl;
   std::cout << "   \"Z\"  :  ";
   print_c_char(charT('Z'));
   std::cout << std::endl;
   std::cout << "   \";\"  :  ";
   print_c_char(charT(';'));
   std::cout << std::endl;
   std::cout << "   \"{\"  :  ";
   print_c_char(charT('{'));
   std::cout << std::endl;
}

template <class charT>
void print_cpp_char(charT c)
{
#ifndef BOOST_NO_STD_LOCALE
   std::locale l;
   const std::collate<charT>& col = BOOST_USE_FACET(std::collate<charT>, l);
   std::basic_string<charT> result = col.transform(&c, &c+1);
   std::cout << result.size() << "   ";
   print_string(result);
   std::size_t n = result.find(charT(0));
   if(n != std::basic_string<charT>::npos)
   {
      std::cerr << "(Error in location of null, found: " << n << ")";
   }
#endif
}

template <class charT>
void print_cpp_info(charT, const char* name)
{
   std::cout << "Info for " << name << " C++ locale API's:" << std::endl;
   std::cout << "   \"a\"  :  ";
   print_cpp_char(charT('a'));
   std::cout << std::endl;
   std::cout << "   \"A\"  :  ";
   print_cpp_char(charT('A'));
   std::cout << std::endl;
   std::cout << "   \"z\"  :  ";
   print_cpp_char(charT('z'));
   std::cout << std::endl;
   std::cout << "   \"Z\"  :  ";
   print_cpp_char(charT('Z'));
   std::cout << std::endl;
   std::cout << "   \";\"  :  ";
   print_cpp_char(charT(';'));
   std::cout << std::endl;
   std::cout << "   \"{\"  :  ";
   print_cpp_char(charT('{'));
   std::cout << std::endl;
}

template <class traits>
void print_sort_syntax(const traits& pt, const char* name)
{
   std::cout << "Sort Key Syntax for type " << name << ":\n";
   typedef typename traits::char_type char_type;
   char_type delim;
   unsigned result = ::boost::re_detail::find_sort_syntax(&pt, &delim);
   std::cout << "   ";
   switch(result)
   {
   case boost::re_detail::sort_C:
      std::cout << "sort_C";
      break;
   case boost::re_detail::sort_fixed:
      std::cout << "sort_fixed" << "    " << static_cast<int>(delim);
      break;
   case boost::re_detail::sort_delim:
      {
         std::cout << "sort_delim" << "    ";
         std::basic_string<char_type> s(1, delim);
         print_string(s);
      }
      break;
   case boost::re_detail::sort_unknown:
      std::cout << "sort_unknown";
      break;
   default:
      std::cout << "bad_value";
      break;
   }
   std::cout << std::endl;

   typedef typename traits::string_type string_type;
   typedef typename traits::char_type char_type;

   char_type c[5] = { 'a', 'A', ';', '{', '}', };
   for(int i = 0; i < 5; ++i)
   {
      string_type s(1, c[i]);
      string_type sk = pt.transform(s.c_str(), s.c_str() + s.size());
      string_type skp = pt.transform_primary(s.c_str(), s.c_str() + s.size());
      print_string(s);
      std::cout << "   ";
      print_string(sk);
      std::cout << "   ";
      print_string(skp);
      std::cout << std::endl;
   }
}

#ifndef BOOST_NO_STD_LOCALE
template <class charT>
void print_ctype_info(charT, const char* name)
{
   std::locale l;
   const std::ctype<charT>& ct = BOOST_USE_FACET(std::ctype<charT>, l);
   typedef typename std::ctype<charT>::mask mask_type;
   mask_type m = static_cast<mask_type>(std::ctype<charT>::lower | std::ctype<charT>::upper);
   bool result = ct.is(m, static_cast<charT>('a')) && ct.is(m , static_cast<charT>('A'));
   std::cout << "Checking std::ctype<" << name << ">::is(mask, c):" << std::endl;
#ifdef BOOST_REGEX_BUGGY_CTYPE_FACET
   std::cout << "   Boost.Regex believes this facet to be buggy..." << std::endl;
#else
   std::cout << "   Boost.Regex believes this facet to be correct..." << std::endl;
#endif
   std::cout << "   Actual behavior, appears to be " << (result ? "correct." : "buggy.") << std::endl;
   assert(ct.is(std::ctype<charT>::alnum, 'a'));
   assert(ct.is(std::ctype<charT>::alnum, 'A'));
   assert(ct.is(std::ctype<charT>::alnum, '0'));
}
#endif

int cpp_main(int /*argc*/, char * /*argv*/[])
{
   print_c_info(char(0), "char");
#ifndef BOOST_NO_WREGEX
   print_c_info(wchar_t(0), "wchar_t");
#endif
   print_cpp_info(char(0), "char");
#ifndef BOOST_NO_WREGEX
   print_cpp_info(wchar_t(0), "wchar_t");
#endif

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x560)
   boost::c_regex_traits<char> a;
   print_sort_syntax(a, "boost::c_regex_traits<char>");
#ifndef BOOST_NO_WREGEX
   boost::c_regex_traits<wchar_t> b;
   print_sort_syntax(b, "boost::c_regex_traits<wchar_t>");
#endif
#endif
#ifndef BOOST_NO_STD_LOCALE
   boost::cpp_regex_traits<char> c;
   print_sort_syntax(c, "boost::cpp_regex_traits<char>");
#ifndef BOOST_NO_WREGEX
   boost::cpp_regex_traits<wchar_t> d;
   print_sort_syntax(d, "boost::cpp_regex_traits<wchar_t>");
#endif
   print_ctype_info(char(0), "char");
#ifndef BOOST_NO_WREGEX
   print_ctype_info(wchar_t(0), "wchar_t");
#endif
#endif
   return 0;
}

#include <boost/test/included/prg_exec_monitor.hpp>
