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


#ifndef REGEX_COMPARISON_HPP
#define REGEX_COMPARISON_HPP

#include <string>
#include <list>
#include <boost/limits.hpp>

//
// globals:
//
extern bool time_boost;
extern bool time_localised_boost;
extern bool time_greta;
extern bool time_safe_greta;
extern bool time_posix;
extern bool time_pcre;
extern bool time_xpressive;
extern bool time_std;

extern bool test_matches;
extern bool test_short_twain;
extern bool test_long_twain;
extern bool test_code;
extern bool test_html;

extern std::string html_template_file;
extern std::string html_out_file;
extern std::string html_contents;


int handle_argument(const std::string& what);
int show_usage();
void load_file(std::string& text, const char* file);
void output_html_results(bool show_description, const std::string& tagname);
void output_final_html();


struct results
{
   double boost_time;
   double localised_boost_time;
   double greta_time;
   double safe_greta_time;
   double posix_time;
   double pcre_time;
   double xpressive_time;
   double std_time;
   double factor;
   std::string expression;
   std::string description;
   results(const std::string& ex, const std::string& desc)
      : boost_time(-1),
        localised_boost_time(-1),
        greta_time(-1),
        safe_greta_time(-1),
        posix_time(-1),
        pcre_time(-1),
        xpressive_time(-1),
		std_time(-1),
        factor((std::numeric_limits<double>::max)()),
        expression(ex), 
        description(desc)
   {}
   void finalise()
   {
      if((boost_time >= 0) && (boost_time < factor))
         factor = boost_time;
      if((localised_boost_time >= 0) && (localised_boost_time < factor))
         factor = localised_boost_time;
      if((greta_time >= 0) && (greta_time < factor))
         factor = greta_time;
      if((safe_greta_time >= 0) && (safe_greta_time < factor))
         factor = safe_greta_time;
      if((posix_time >= 0) && (posix_time < factor))
         factor = posix_time;
      if((pcre_time >= 0) && (pcre_time < factor))
         factor = pcre_time;
      if((xpressive_time >= 0) && (xpressive_time < factor))
         factor = xpressive_time;
      if((std_time >= 0) && (std_time < factor))
         factor = std_time;
   }
};

extern std::list<results> result_list;


namespace b {
// boost tests:
double time_match(const std::string& re, const std::string& text, bool icase);
double time_find_all(const std::string& re, const std::string& text, bool icase);

}
namespace bl {
// localised boost tests:
double time_match(const std::string& re, const std::string& text, bool icase);
double time_find_all(const std::string& re, const std::string& text, bool icase);

}
namespace pcr {
// pcre tests:
double time_match(const std::string& re, const std::string& text, bool icase);
double time_find_all(const std::string& re, const std::string& text, bool icase);

}
namespace g {
// greta tests:
double time_match(const std::string& re, const std::string& text, bool icase);
double time_find_all(const std::string& re, const std::string& text, bool icase);

}
namespace gs {
// safe greta tests:
double time_match(const std::string& re, const std::string& text, bool icase);
double time_find_all(const std::string& re, const std::string& text, bool icase);

}
namespace posix {
// safe greta tests:
double time_match(const std::string& re, const std::string& text, bool icase);
double time_find_all(const std::string& re, const std::string& text, bool icase);

}
namespace dxpr {
// xpressive tests:
double time_match(const std::string& re, const std::string& text, bool icase);
double time_find_all(const std::string& re, const std::string& text, bool icase);
}
namespace stdr {
// xpressive tests:
double time_match(const std::string& re, const std::string& text, bool icase);
double time_find_all(const std::string& re, const std::string& text, bool icase);
}

void test_match(const std::string& re, const std::string& text, const std::string& description, bool icase = false);
void test_find_all(const std::string& re, const std::string& text, const std::string& description, bool icase = false);
inline void test_match(const std::string& re, const std::string& text, bool icase = false)
{ test_match(re, text, text, icase); }
inline void test_find_all(const std::string& re, const std::string& text, bool icase = false)
{ test_find_all(re, text, "", icase); }


#define REPEAT_COUNT 10

#endif

