/*
 *
 * Copyright (c) 2002-2003
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <iterator>
#include <boost/regex.hpp>
#include <boost/version.hpp>
#include "regex_comparison.hpp"

#ifdef BOOST_HAS_PCRE
#include "pcre.h" // for pcre version number
#endif

//
// globals:
//
bool time_boost = false;
bool time_localised_boost = false;
bool time_greta = false;
bool time_safe_greta = false;
bool time_posix = false;
bool time_pcre = false;
bool time_xpressive = false;
bool time_std = false;

bool test_matches = false;
bool test_code = false;
bool test_html = false;
bool test_short_twain = false;
bool test_long_twain = false;


std::string html_template_file;
std::string html_out_file;
std::string html_contents;
std::list<results> result_list;

// the following let us compute averages:
double greta_total = 0;
double safe_greta_total = 0;
double boost_total = 0;
double locale_boost_total = 0;
double posix_total = 0;
double pcre_total = 0;
double xpressive_total = 0;
double std_total = 0;
unsigned greta_test_count = 0;
unsigned safe_greta_test_count = 0;
unsigned boost_test_count = 0;
unsigned locale_boost_test_count = 0;
unsigned posix_test_count = 0;
unsigned pcre_test_count = 0;
unsigned xpressive_test_count = 0;
unsigned std_test_count = 0;

int handle_argument(const std::string& what)
{
   if(what == "-b")
      time_boost = true;
   else if(what == "-bl")
      time_localised_boost = true;
#ifdef BOOST_HAS_GRETA
   else if(what == "-g")
      time_greta = true;
   else if(what == "-gs")
      time_safe_greta = true;
#endif
#ifdef BOOST_HAS_POSIX
   else if(what == "-posix")
      time_posix = true;
#endif
#ifdef BOOST_HAS_PCRE
   else if(what == "-pcre")
      time_pcre = true;
#endif
#ifdef BOOST_HAS_XPRESSIVE
   else if(what == "-xpressive" || what == "-dxpr")
      time_xpressive = true;
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
   else if(what == "-std")
      time_std = true;
#endif
   else if(what == "-all")
   {
      time_boost = true;
      time_localised_boost = true;
#ifdef BOOST_HAS_GRETA
      time_greta = true;
      time_safe_greta = true;
#endif
#ifdef BOOST_HAS_POSIX
      time_posix = true;
#endif
#ifdef BOOST_HAS_PCRE
      time_pcre = true;
#endif
#ifdef BOOST_HAS_XPRESSIVE
      time_xpressive = true;
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
      time_std = true;
#endif
   }
   else if(what == "-test-matches")
      test_matches = true;
   else if(what == "-test-code")
      test_code = true;
   else if(what == "-test-html")
      test_html = true;
   else if(what == "-test-short-twain")
      test_short_twain = true;
   else if(what == "-test-long-twain")
      test_long_twain = true;
   else if(what == "-test-all")
   {
      test_matches = true;
      test_code = true;
      test_html = true;
      test_short_twain = true;
      test_long_twain = true;
   }
   else if((what == "-h") || (what == "--help"))
      return show_usage();
   else if((what[0] == '-') || (what[0] == '/'))
   {
      std::cerr << "Unknown argument: \"" << what << "\"" << std::endl;
      return 1;
   }
   else if(html_template_file.size() == 0)
   {
      html_template_file = what;
      load_file(html_contents, what.c_str());
   }
   else if(html_out_file.size() == 0)
      html_out_file = what;
   else
   {
      std::cerr << "Unexpected argument: \"" << what << "\"" << std::endl;
      return 1;
   }
   return 0;
}

int show_usage()
{
   std::cout <<
      "Usage\n"
      "regex_comparison [-h] [library options] [test options] [html_template html_output_file]\n"
      "   -h        Show help\n\n"
      "   library options:\n"
      "      -b     Apply tests to boost library\n"
      "      -bl    Apply tests to boost library with C++ locale\n"
#ifdef BOOST_HAS_GRETA
      "      -g     Apply tests to GRETA library\n"
      "      -gs    Apply tests to GRETA library (in non-recursive mode)\n"
#endif
#ifdef BOOST_HAS_POSIX
      "      -posix Apply tests to POSIX library\n"
#endif
#ifdef BOOST_HAS_PCRE
      "      -pcre  Apply tests to PCRE library\n"
#endif
#ifdef BOOST_HAS_XPRESSIVE
      "      -dxpr  Apply tests to dynamic xpressive library\n"
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
	  "      -std  Apply tests to std::regex.\n"
#endif
      "      -all   Apply tests to all libraries\n\n"
      "   test options:\n"
      "      -test-matches      Test short matches\n"
      "      -test-code         Test c++ code examples\n"
      "      -test-html         Test c++ code examples\n"
      "      -test-short-twain  Test short searches\n"
      "      -test-long-twain   Test long searches\n"
      "      -test-all          Test everthing\n";
   return 1;
}

void load_file(std::string& text, const char* file)
{
   std::deque<char> temp_copy;
   std::ifstream is(file);
   if(!is.good())
   {
      std::string msg("Unable to open file: \"");
      msg.append(file);
      msg.append("\"");
      throw std::runtime_error(msg);
   }
   is.seekg(0, std::ios_base::end);
   std::istream::pos_type pos = is.tellg();
   is.seekg(0, std::ios_base::beg);
   text.erase();
   text.reserve(pos);
   std::istreambuf_iterator<char> it(is);
   std::copy(it, std::istreambuf_iterator<char>(), std::back_inserter(text));
}

void print_result(std::ostream& os, double time, double best)
{
   static const char* suffixes[] = {"s", "ms", "us", "ns", "ps", };

   if(time < 0)
   {
      os << "<td>NA</td>";
      return;
   }
   double rel = time / best;
   bool highlight = ((rel > 0) && (rel < 1.1));
   unsigned suffix = 0;
   while(time < 0)
   {
      time *= 1000;
      ++suffix;
   }
   os << "<td>";
   if(highlight)
      os << "<font color=\"#008000\">";
   if(rel <= 1000)
      os << std::setprecision(3) << rel;
   else
      os << (int)rel;
   os << "<BR>(";
   if(time <= 1000)
      os << std::setprecision(3) << time;
   else
      os << (int)time;
   os << suffixes[suffix] << ")";
   if(highlight)
      os << "</font>";
   os << "</td>";
}

std::string html_quote(const std::string& in)
{
   static const boost::regex e("(<)|(>)|(&)|(\")");
   static const std::string format("(?1&lt;)(?2&gt;)(?3&amp;)(?4&quot;)");
   return regex_replace(in, e, format, boost::match_default | boost::format_all);
}

void output_html_results(bool show_description, const std::string& tagname)
{
   std::stringstream os;
   if(result_list.size())
   {
      //
      // start by outputting the table header:
      //
      os << "<table border=\"1\" cellspacing=\"1\">\n";
      os << "<tr><td><strong>Expression</strong></td>";
      if(show_description)
         os << "<td><strong>Text</strong></td>";
#if defined(BOOST_HAS_GRETA)
      if(time_greta == true)
         os << "<td><strong>GRETA</strong></td>";
      if(time_safe_greta == true)
         os << "<td><strong>GRETA<BR>(non-recursive mode)</strong></td>";
#endif
      if(time_boost == true)
         os << "<td><strong>Boost</strong></td>";
      if(time_localised_boost == true)
         os << "<td><strong>Boost + C++ locale</strong></td>";
#if defined(BOOST_HAS_POSIX)
      if(time_posix == true)
         os << "<td><strong>POSIX</strong></td>";
#endif
#ifdef BOOST_HAS_PCRE
      if(time_pcre == true)
         os << "<td><strong>PCRE</strong></td>";
#endif
#ifdef BOOST_HAS_XPRESSIVE
      if(time_xpressive == true)
         os << "<td><strong>Dynamic Xpressive</strong></td>";
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
      if(time_std == true)
         os << "<td><strong>std::regex</strong></td>";
#endif
      os << "</tr>\n";

      //
      // Now enumerate through all the test results:
      //
      std::list<results>::const_iterator first, last;
      first = result_list.begin();
      last = result_list.end();
      while(first != last)
      {
         os << "<tr><td><code>" << html_quote(first->expression) << "</code></td>";
         if(show_description)
            os << "<td>" << html_quote(first->description) << "</td>";
#if defined(BOOST_HAS_GRETA)
         if(time_greta == true)
         {
            print_result(os, first->greta_time, first->factor);
            if(first->greta_time > 0)
            {
               greta_total += first->greta_time / first->factor;
               ++greta_test_count;
            }
         }
         if(time_safe_greta == true)
         {
            print_result(os, first->safe_greta_time, first->factor);
            if(first->safe_greta_time > 0)
            {
               safe_greta_total += first->safe_greta_time / first->factor;
               ++safe_greta_test_count;
            }
         }
#endif
         if(time_boost == true)
         {
            print_result(os, first->boost_time, first->factor);
            if(first->boost_time > 0)
            {
               boost_total += first->boost_time / first->factor;
               ++boost_test_count;
            }
         }
         if(time_localised_boost == true)
         {
            print_result(os, first->localised_boost_time, first->factor);
            if(first->localised_boost_time > 0)
            {
               locale_boost_total += first->localised_boost_time / first->factor;
               ++locale_boost_test_count;
            }
         }
         if(time_posix == true)
         {
            print_result(os, first->posix_time, first->factor);
            if(first->posix_time > 0)
            {
               posix_total += first->posix_time / first->factor;
               ++posix_test_count;
            }
         }
#if defined(BOOST_HAS_PCRE)
         if(time_pcre == true)
         {
            print_result(os, first->pcre_time, first->factor);
            if(first->pcre_time > 0)
            {
               pcre_total += first->pcre_time / first->factor;
               ++pcre_test_count;
            }
         }
#endif
#if defined(BOOST_HAS_XPRESSIVE)
         if(time_xpressive == true)
         {
            print_result(os, first->xpressive_time, first->factor);
            if(first->xpressive_time > 0)
            {
               xpressive_total += first->xpressive_time / first->factor;
               ++xpressive_test_count;
            }
         }
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
         if(time_std == true)
         {
            print_result(os, first->std_time, first->factor);
            if(first->std_time > 0)
            {
               std_total += first->std_time / first->factor;
               ++std_test_count;
            }
         }
#endif
         os << "</tr>\n";
         ++first;
      }
      os << "</table>\n";
      result_list.clear();
   }
   else
   {
      os << "<P><I>Results not available...</I></P>\n";
   }

   std::string result = os.str();

   std::string::size_type pos = html_contents.find(tagname);
   if(pos != std::string::npos)
   {
      html_contents.replace(pos, tagname.size(), result);
   }
}

std::string get_boost_version()
{
   std::stringstream os;
   os << (BOOST_VERSION / 100000) << '.' << ((BOOST_VERSION / 100) % 1000) << '.' << (BOOST_VERSION % 100);
   return os.str();
}

std::string get_averages_table()
{
   std::stringstream os;
   //
   // start by outputting the table header:
   //
   os << "<table border=\"1\" cellspacing=\"1\">\n";
   os << "<tr>";
#if defined(BOOST_HAS_GRETA)
   if(time_greta == true)
   {
      os << "<td><strong>GRETA</strong></td>";
   }
   if(time_safe_greta == true)
   {
      os << "<td><strong>GRETA<BR>(non-recursive mode)</strong></td>";
   }

#endif
   if(time_boost == true)
   {
      os << "<td><strong>Boost</strong></td>";
   }
   if(time_localised_boost == true)
   {
      os << "<td><strong>Boost + C++ locale</strong></td>";
   }
#if defined(BOOST_HAS_POSIX)
   if(time_posix == true)
   {
      os << "<td><strong>POSIX</strong></td>";
   }
#endif
#ifdef BOOST_HAS_PCRE
   if(time_pcre == true)
   {
      os << "<td><strong>PCRE</strong></td>";
   }
#endif
#ifdef BOOST_HAS_XPRESSIVE
   if(time_xpressive == true)
   {
      os << "<td><strong>Dynamic Xpressive</strong></td>";
   }
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
   if(time_std == true)
   {
      os << "<td><strong>std::regex</strong></td>";
   }
#endif
   os << "</tr>\n";

   //
   // Now enumerate through all averages:
   //
   os << "<tr>";
#if defined(BOOST_HAS_GRETA)
   if(time_greta == true)
      os << "<td>" << (greta_total / greta_test_count) << "</td>\n";
   if(time_safe_greta == true)
      os << "<td>" << (safe_greta_total / safe_greta_test_count) << "</td>\n";
#endif
#if defined(BOOST_HAS_POSIX)
   if(time_boost == true)
      os << "<td>" << (boost_total / boost_test_count) << "</td>\n";
#endif
   if(time_boost == true)
      os << "<td>" << (boost_total / boost_test_count) << "</td>\n";
   if(time_localised_boost == true)
      os << "<td>" << (locale_boost_total / locale_boost_test_count) << "</td>\n";
   if(time_posix == true)
      os << "<td>" << (posix_total / posix_test_count) << "</td>\n";
#if defined(BOOST_HAS_PCRE)
   if(time_pcre == true)
      os << "<td>" << (pcre_total / pcre_test_count) << "</td>\n";
#endif
#if defined(BOOST_HAS_XPRESSIVE)
   if(time_xpressive == true)
      os << "<td>" << (xpressive_total / xpressive_test_count) << "</td>\n";
#endif
#ifndef BOOST_NO_CXX11_HDR_REGEX
   if(time_std == true)
      os << "<td>" << (std_total / std_test_count) << "</td>\n";
#endif
   os << "</tr>\n";
   os << "</table>\n";
   return os.str();
}

void output_final_html()
{
   if(html_out_file.size())
   {
      //
      // start with search and replace ops:
      //
      std::string::size_type pos;
      pos = html_contents.find("%compiler%");
      if(pos != std::string::npos)
      {
         html_contents.replace(pos, 10, BOOST_COMPILER);
      }
      pos = html_contents.find("%library%");
      if(pos != std::string::npos)
      {
         html_contents.replace(pos, 9, BOOST_STDLIB);
      }
      pos = html_contents.find("%os%");
      if(pos != std::string::npos)
      {
         html_contents.replace(pos, 4, BOOST_PLATFORM);
      }
      pos = html_contents.find("%boost%");
      if(pos != std::string::npos)
      {
         html_contents.replace(pos, 7, get_boost_version());
      }
      pos = html_contents.find("%pcre%");
      if(pos != std::string::npos)
      {
#ifdef PCRE_MINOR
         html_contents.replace(pos, 6, BOOST_STRINGIZE(PCRE_MAJOR.PCRE_MINOR));
#else
         html_contents.replace(pos, 6, "N/A");
#endif
      }
      pos = html_contents.find("%averages%");
      if(pos != std::string::npos)
      {
         html_contents.replace(pos, 10, get_averages_table());
      }
     //
      // now right the output to file:
      //
      std::ofstream os(html_out_file.c_str());
      os << html_contents;
   }
   else
   {
      std::cout << html_contents;
   }
}
