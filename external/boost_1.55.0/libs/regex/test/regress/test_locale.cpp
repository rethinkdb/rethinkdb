/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "test.hpp"
#include <clocale>
#if defined(BOOST_WINDOWS) && !defined(BOOST_DISABLE_WIN32)
#include <boost/scoped_array.hpp>
#include <windows.h>
#endif

#ifdef BOOST_MSVC
#pragma warning(disable:4127)
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{ using ::setlocale; }
#endif

test_locale::test_locale(const char* c_name, boost::uint32_t lcid)
{
#ifndef UNDER_CE
   // store the name:
   m_old_name = m_name;
   m_name = c_name;
   // back up C locale and then set it's new name:
   const char* pl = std::setlocale(LC_ALL, 0);
   m_old_c_locale = pl ? pl : "";
   m_old_c_state = s_c_locale;
   if(std::setlocale(LC_ALL, c_name))
   {
      s_c_locale = test_with_locale;
      std::cout << "Testing the global C locale: " << c_name << std::endl;
   }
   else
   {
      s_c_locale = no_test;
      std::cout << "The global C locale: " << c_name << " is not available and will not be tested." << std::endl;
   }
#else
  s_c_locale = no_test;
#endif
#ifndef BOOST_NO_STD_LOCALE
   // back up the C++ locale and create the new one:
   m_old_cpp_locale = s_cpp_locale_inst;
   m_old_cpp_state = s_cpp_locale;
   try{
      s_cpp_locale_inst = std::locale(c_name);
      s_cpp_locale = test_with_locale;
      std::cout << "Testing the C++ locale: " << c_name << std::endl;
   }catch(std::runtime_error const &)
   {
      s_cpp_locale = no_test;
      std::cout << "The C++ locale: " << c_name << " is not available and will not be tested." << std::endl;
   }
#else
   s_cpp_locale = no_test;
#endif

   // back up win locale and create the new one:
   m_old_win_locale = s_win_locale_inst;
   m_old_win_state = s_win_locale;
   s_win_locale_inst = lcid;
#if defined(BOOST_WINDOWS) && !defined(BOOST_DISABLE_WIN32)
   //
   // Start by geting the printable name of the locale.
   // We use this for debugging purposes only:
   //
#ifndef BOOST_NO_ANSI_APIS
   boost::scoped_array<char> p;
   int r = ::GetLocaleInfoA(
               lcid,               // locale identifier
               LOCALE_SCOUNTRY,    // information type
               0,                  // information buffer
               0                   // size of buffer
            );
   p.reset(new char[r+1]);
   r = ::GetLocaleInfoA(
               lcid,               // locale identifier
               LOCALE_SCOUNTRY,    // information type
               p.get(),            // information buffer
               r+1                 // size of buffer
            );
#else
   WCHAR code_page_string[7];
   int r = ::GetLocaleInfoW(
               lcid, 
               LOCALE_IDEFAULTANSICODEPAGE, 
               code_page_string, 
               7);
   BOOST_ASSERT(r != 0);

   UINT code_page = static_cast<UINT>(_wtol(code_page_string));

   boost::scoped_array<wchar_t> wp;
   r = ::GetLocaleInfoW(
               lcid,               // locale identifier
               LOCALE_SCOUNTRY,    // information type
               0,                  // information buffer
               0                   // size of buffer
            );
   wp.reset(new wchar_t[r+1]);
   r = ::GetLocaleInfoW(
               lcid,               // locale identifier
               LOCALE_SCOUNTRY,    // information type
               wp.get(),            // information buffer
               r+1                 // size of buffer
            );

   int name_size = (r+1) * 2;
   boost::scoped_array<char> p(new char[name_size]);
   int conv_r = ::WideCharToMultiByte(
               code_page, 
               0,  
               wp.get(), r,  
               p.get(), name_size,  
               NULL, NULL
            );
   BOOST_ASSERT(conv_r != 0);
#endif
   //
   // now see if locale is installed and behave accordingly:
   //
   if(::IsValidLocale(lcid, LCID_INSTALLED))
   {
      s_win_locale = test_with_locale;
      std::cout << "Testing the Win32 locale: \"" << p.get() << "\" (0x" << std::hex << lcid << ")" << std::endl;
   }
   else
   {
      s_win_locale = no_test;
      std::cout << "The Win32 locale: \"" << p.get() << "\" (0x" << std::hex << lcid << ") is not available and will not be tested." << std::endl;
   }
#else
   s_win_locale = no_test;
#endif
}

test_locale::~test_locale()
{
   // restore to previous state:
#ifndef UNDER_CE
   std::setlocale(LC_ALL, m_old_c_locale.c_str());
   s_c_locale = m_old_c_state;
#endif
#ifndef BOOST_NO_STD_LOCALE
   s_cpp_locale_inst = m_old_cpp_locale;
#endif
   s_cpp_locale = m_old_cpp_state;
   s_win_locale_inst = m_old_win_locale;
   s_win_locale = m_old_win_state;
   m_name = m_old_name;
}

int test_locale::s_c_locale = test_no_locale;
int test_locale::s_cpp_locale = test_no_locale;
int test_locale::s_win_locale = test_no_locale;
#ifndef BOOST_NO_STD_LOCALE
std::locale test_locale::s_cpp_locale_inst;
#endif
boost::uint32_t test_locale::s_win_locale_inst = 0;
std::string test_locale::m_name;


void test_en_locale(const char* name, boost::uint32_t lcid)
{
   using namespace boost::regex_constants;
   errors_as_warnings w;
   test_locale l(name, lcid);
   TEST_REGEX_SEARCH_L("[[:lower:]]+", perl, "\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xf7", match_default, make_array(1, 32, -2, -2));
   TEST_REGEX_SEARCH_L("[[:upper:]]+", perl, "\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf", match_default, make_array(1, 31, -2, -2));
//   TEST_REGEX_SEARCH_L("[[:punct:]]+", perl, "\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0", match_default, make_array(0, 31, -2, -2));
   TEST_REGEX_SEARCH_L("[[:print:]]+", perl, "\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe", match_default, make_array(0, 93, -2, -2));
   TEST_REGEX_SEARCH_L("[[:graph:]]+", perl, "\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe", match_default, make_array(0, 93, -2, -2));
   TEST_REGEX_SEARCH_L("[[:word:]]+", perl, "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf8\xf9\xfa\xfb\xfc\xfd\xfe", match_default, make_array(0, 61, -2, -2));
   // collation sensitive ranges:
#if !BOOST_WORKAROUND(__BORLANDC__, < 0x600)
   // these tests are disabled for Borland C++: a bug in std::collate<wchar_t>
   // causes these tests to crash (pointer overrun in std::collate<wchar_t>::do_transform).
   TEST_REGEX_SEARCH_L("[a-z]+", perl|::boost::regex_constants::collate, "\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf8\xf9\xfa\xfb\xfc", match_default, make_array(0, 28, -2, -2));
   TEST_REGEX_SEARCH_L("[a-z]+", perl|::boost::regex_constants::collate, "\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd8\xd9\xda\xdb\xdc", match_default, make_array(1, 28, -2, -2));
   // and equivalence classes:
   TEST_REGEX_SEARCH_L("[[=a=]]+", perl, "aA\xe0\xe1\xe2\xe3\xe4\xe5\xc0\xc1\xc2\xc3\xc4\xc5", match_default, make_array(0, 14, -2, -2));
   // case mapping:
   TEST_REGEX_SEARCH_L("[A-Z]+", perl|icase|::boost::regex_constants::collate, "\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf8\xf9\xfa\xfb\xfc", match_default, make_array(0, 28, -2, -2));
   TEST_REGEX_SEARCH_L("[a-z]+", perl|icase|::boost::regex_constants::collate, "\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd8\xd9\xda\xdb\xdc", match_default, make_array(1, 28, -2, -2));
   TEST_REGEX_SEARCH_L("\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd8\xd9\xda\xdb\xdc\xdd", perl|icase, "\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf8\xf9\xfa\xfb\xfc\xfd\xfe", match_default, make_array(1, 30, -2, -2));
#endif
}

void test_en_locale()
{
   // VC6 seems to have problems with std::setlocale, I've never
   // gotten to the bottem of this as the program runs fine under the
   // debugger, but hangs when run from bjam:
#if !BOOST_WORKAROUND(BOOST_MSVC, <1300) && !(defined(__ICL) && defined(_MSC_VER) && (_MSC_VER == 1200))
   test_en_locale("en_US", 0x09 | 0x01 << 10);
   test_en_locale("en_UK", 0x09 | 0x02 << 10);
   test_en_locale("en", 0x09);
#endif
}


