/*
 *
 * Copyright (c) 2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <cassert>
#include <boost/test/execution_monitor.hpp>
#include "regex_comparison.hpp"


void test_match(const std::string& re, const std::string& text, const std::string& description, bool icase)
{
   double time;
   results r(re, description);

   std::cout << "Testing: \"" << re << "\" against \"" << description << "\"" << std::endl;

#ifdef BOOST_HAS_GRETA
   if(time_greta == true)
   {
      time = g::time_match(re, text, icase);
      r.greta_time = time;
      std::cout << "\tGRETA regex: " << time << "s\n";
   }
   if(time_safe_greta == true)
   {
      time = gs::time_match(re, text, icase);
      r.safe_greta_time = time;
      std::cout << "\tSafe GRETA regex: " << time << "s\n";
   }
#endif
   if(time_boost == true)
   {
      time = b::time_match(re, text, icase);
      r.boost_time = time;
      std::cout << "\tBoost regex: " << time << "s\n";
   }
   if(time_localised_boost == true)
   {
      time = bl::time_match(re, text, icase);
      r.localised_boost_time = time;
      std::cout << "\tBoost regex (C++ locale): " << time << "s\n";
   }
#ifdef BOOST_HAS_POSIX
   if(time_posix == true)
   {
      time = posix::time_match(re, text, icase);
      r.posix_time = time;
      std::cout << "\tPOSIX regex: " << time << "s\n";
   }
#endif
#ifdef BOOST_HAS_PCRE
   if(time_pcre == true)
   {
      time = pcr::time_match(re, text, icase);
      r.pcre_time = time;
      std::cout << "\tPCRE regex: " << time << "s\n";
   }
#endif
#ifdef BOOST_HAS_XPRESSIVE
   if(time_xpressive == true)
   {
      time = dxpr::time_match(re, text, icase);
      r.xpressive_time = time;
      std::cout << "\txpressive regex: " << time << "s\n";
   }
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
   if(time_std == true)
   {
      time = stdr::time_match(re, text, icase);
      r.std_time = time;
      std::cout << "\tstd::regex: " << time << "s\n";
   }
#endif
   r.finalise();
   result_list.push_back(r);
}

void test_find_all(const std::string& re, const std::string& text, const std::string& description, bool icase)
{
   std::cout << "Testing: " << re << std::endl;

   double time;
   results r(re, description);

#ifdef BOOST_HAS_GRETA
   if(time_greta == true)
   {
      time = g::time_find_all(re, text, icase);
      r.greta_time = time;
      std::cout << "\tGRETA regex: " << time << "s\n";
   }
   if(time_safe_greta == true)
   {
      time = gs::time_find_all(re, text, icase);
      r.safe_greta_time = time;
      std::cout << "\tSafe GRETA regex: " << time << "s\n";
   }
#endif
   if(time_boost == true)
   {
      time = b::time_find_all(re, text, icase);
      r.boost_time = time;
      std::cout << "\tBoost regex: " << time << "s\n";
   }
   if(time_localised_boost == true)
   {
      time = bl::time_find_all(re, text, icase);
      r.localised_boost_time = time;
      std::cout << "\tBoost regex (C++ locale): " << time << "s\n";
   }
#ifdef BOOST_HAS_POSIX
   if(time_posix == true)
   {
      time = posix::time_find_all(re, text, icase);
      r.posix_time = time;
      std::cout << "\tPOSIX regex: " << time << "s\n";
   }
#endif
#ifdef BOOST_HAS_PCRE
   if(time_pcre == true)
   {
      time = pcr::time_find_all(re, text, icase);
      r.pcre_time = time;
      std::cout << "\tPCRE regex: " << time << "s\n";
   }
#endif
#ifdef BOOST_HAS_XPRESSIVE
   if(time_xpressive == true)
   {
      time = dxpr::time_find_all(re, text, icase);
      r.xpressive_time = time;
      std::cout << "\txpressive regex: " << time << "s\n";
   }
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
   if(time_std == true)
   {
      time = stdr::time_find_all(re, text, icase);
      r.std_time = time;
      std::cout << "\tstd::regex: " << time << "s\n";
   }
#endif
   r.finalise();
   result_list.push_back(r);
}

int cpp_main(int argc, char * argv[])
{
   // start by processing the command line args:
   if(argc < 2)
      return show_usage();
   int result = 0;
   for(int c = 1; c < argc; ++c)
   {
      result += handle_argument(argv[c]);
   }
   if(result)
      return result;

   if(test_matches)
   {
      // start with a simple test, this is basically a measure of the minimal overhead
      // involved in calling a regex matcher:
      test_match("abc", "abc");
      // these are from the regex docs:
      test_match("^([0-9]+)(\\-| |$)(.*)$", "100- this is a line of ftp response which contains a message string");
      test_match("([[:digit:]]{4}[- ]){3}[[:digit:]]{3,4}", "1234-5678-1234-456");
      // these are from http://www.regxlib.com/
      test_match("^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$", "john@johnmaddock.co.uk");
      test_match("^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$", "foo12@foo.edu");
      test_match("^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$", "bob.smith@foo.tv");
      test_match("^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$", "EH10 2QQ");
      test_match("^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$", "G1 1AA");
      test_match("^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$", "SW1 1ZZ");
      test_match("^[[:digit:]]{1,2}/[[:digit:]]{1,2}/[[:digit:]]{4}$", "4/1/2001");
      test_match("^[[:digit:]]{1,2}/[[:digit:]]{1,2}/[[:digit:]]{4}$", "12/12/2001");
      test_match("^[-+]?[[:digit:]]*\\.?[[:digit:]]*$", "123");
      test_match("^[-+]?[[:digit:]]*\\.?[[:digit:]]*$", "+3.14159");
      test_match("^[-+]?[[:digit:]]*\\.?[[:digit:]]*$", "-3.14159");
   }
   output_html_results(true, "%short_matches%");

   std::string file_contents;

   if(test_code)
   {
      load_file(file_contents, "../../../boost/crc.hpp");

      const char* highlight_expression = // preprocessor directives: index 1
                              "(^[ \t]*#(?:[^\\\\\\n]|\\\\[^\\n_[:punct:][:alnum:]]*[\\n[:punct:][:word:]])*)|"
                              // comment: index 2
                              "(//[^\\n]*|/\\*.*?\\*/)|"
                              // literals: index 3
                              "\\<([+-]?(?:(?:0x[[:xdigit:]]+)|(?:(?:[[:digit:]]*\\.)?[[:digit:]]+(?:[eE][+-]?[[:digit:]]+)?))u?(?:(?:int(?:8|16|32|64))|L)?)\\>|"
                              // string literals: index 4
                              "('(?:[^\\\\']|\\\\.)*'|\"(?:[^\\\\\"]|\\\\.)*\")|"
                              // keywords: index 5
                              "\\<(__asm|__cdecl|__declspec|__export|__far16|__fastcall|__fortran|__import"
                              "|__pascal|__rtti|__stdcall|_asm|_cdecl|__except|_export|_far16|_fastcall"
                              "|__finally|_fortran|_import|_pascal|_stdcall|__thread|__try|asm|auto|bool"
                              "|break|case|catch|cdecl|char|class|const|const_cast|continue|default|delete"
                              "|do|double|dynamic_cast|else|enum|explicit|extern|false|float|for|friend|goto"
                              "|if|inline|int|long|mutable|namespace|new|operator|pascal|private|protected"
                              "|public|register|reinterpret_cast|return|short|signed|sizeof|static|static_cast"
                              "|struct|switch|template|this|throw|true|try|typedef|typeid|typename|union|unsigned"
                              "|using|virtual|void|volatile|wchar_t|while)\\>"
                              ;

      const char* class_expression = "^(template[[:space:]]*<[^;:{]+>[[:space:]]*)?" 
                   "(class|struct)[[:space:]]*(\\<\\w+\\>([ \t]*\\([^)]*\\))?" 
                   "[[:space:]]*)*(\\<\\w*\\>)[[:space:]]*(<[^;:{]+>[[:space:]]*)?" 
                   "(\\{|:[^;\\{()]*\\{)";

      const char* include_expression = "^[ \t]*#[ \t]*include[ \t]+(\"[^\"]+\"|<[^>]+>)";
      const char* boost_include_expression = "^[ \t]*#[ \t]*include[ \t]+(\"boost/[^\"]+\"|<boost/[^>]+>)";


      test_find_all(class_expression, file_contents);
      test_find_all(highlight_expression, file_contents);
      test_find_all(include_expression, file_contents);
      test_find_all(boost_include_expression, file_contents);
   }
   output_html_results(false, "%code_search%");

   if(test_html)
   {
      load_file(file_contents, "../../../libs/libraries.htm");
      test_find_all("beman|john|dave", file_contents, true);
      test_find_all("<p>.*?</p>", file_contents, true);
      test_find_all("<a[^>]+href=(\"[^\"]*\"|[^[:space:]]+)[^>]*>", file_contents, true);
      test_find_all("<h[12345678][^>]*>.*?</h[12345678]>", file_contents, true);
      test_find_all("<img[^>]+src=(\"[^\"]*\"|[^[:space:]]+)[^>]*>", file_contents, true);
      test_find_all("<font[^>]+face=(\"[^\"]*\"|[^[:space:]]+)[^>]*>.*?</font>", file_contents, true);
   }
   output_html_results(false, "%html_search%");

   if(test_short_twain)
   {
      load_file(file_contents, "short_twain.txt");

      test_find_all("Twain", file_contents);
      test_find_all("Huck[[:alpha:]]+", file_contents);
      test_find_all("[[:alpha:]]+ing", file_contents);
      test_find_all("^[^\n]*?Twain", file_contents);
      test_find_all("Tom|Sawyer|Huckleberry|Finn", file_contents);
      test_find_all("(Tom|Sawyer|Huckleberry|Finn).{0,30}river|river.{0,30}(Tom|Sawyer|Huckleberry|Finn)", file_contents);
   }
   output_html_results(false, "%short_twain_search%");

   if(test_long_twain)
   {
      load_file(file_contents, "mtent13.txt");

      test_find_all("Twain", file_contents);
      test_find_all("Huck[[:alpha:]]+", file_contents);
      test_find_all("[[:alpha:]]+ing", file_contents);
      test_find_all("^[^\n]*?Twain", file_contents);
      test_find_all("Tom|Sawyer|Huckleberry|Finn", file_contents);
      time_posix = false;
      test_find_all("(Tom|Sawyer|Huckleberry|Finn).{0,30}river|river.{0,30}(Tom|Sawyer|Huckleberry|Finn)", file_contents);
      time_posix = true;
   }   
   output_html_results(false, "%long_twain_search%");

   output_final_html();
   return 0;
}


