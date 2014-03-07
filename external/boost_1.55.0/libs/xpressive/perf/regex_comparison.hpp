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
extern bool time_greta;
extern bool time_safe_greta;
extern bool time_dynamic_xpressive;
extern bool time_static_xpressive;
//extern bool time_posix;
//extern bool time_pcre;

extern bool test_matches;
extern bool test_short_twain;
extern bool test_long_twain;

extern std::string xml_out_file;
extern std::string xml_contents;


int handle_argument(const std::string& what);
int show_usage();
void load_file(std::string& text, const char* file);
void output_xml_results(bool show_description, const std::string& title, const std::string& filename);

struct results
{
   double boost_time;
   double greta_time;
   double safe_greta_time;
   double dynamic_xpressive_time;
   double static_xpressive_time;
   //double posix_time;
   //double pcre_time;
   double factor;
   std::string expression;
   std::string description;
   results(const std::string& ex, const std::string& desc)
      : boost_time(-1), 
        greta_time(-1),
        safe_greta_time(-1),
        dynamic_xpressive_time(-1),
        static_xpressive_time(-1),
        //posix_time(-1),
        //pcre_time(-1),
        factor((std::numeric_limits<double>::max)()),
        expression(ex), 
        description(desc)
   {}
   void finalise()
   {
      if((boost_time >= 0) && (boost_time < factor))
         factor = boost_time;
      if((greta_time >= 0) && (greta_time < factor))
         factor = greta_time;
      if((safe_greta_time >= 0) && (safe_greta_time < factor))
          factor = safe_greta_time;
      if((dynamic_xpressive_time >= 0) && (dynamic_xpressive_time < factor))
          factor = dynamic_xpressive_time;
      if((static_xpressive_time >= 0) && (static_xpressive_time < factor))
          factor = static_xpressive_time;
      //if((posix_time >= 0) && (posix_time < factor))
      //   factor = posix_time;
      //if((pcre_time >= 0) && (pcre_time < factor))
      //   factor = pcre_time;
      if((factor >= 0) && (factor < factor))
         factor = factor;
   }
};

extern std::list<results> result_list;


namespace b {
// boost tests:
double time_match(const std::string& re, const std::string& text);
double time_find_all(const std::string& re, const std::string& text);
}
//namespace posix {
//// posix tests:
//double time_match(const std::string& re, const std::string& text);
//double time_find_all(const std::string& re, const std::string& text);
//
//}
//namespace pcr {
//// pcre tests:
//double time_match(const std::string& re, const std::string& text);
//double time_find_all(const std::string& re, const std::string& text);
//
//}
namespace g {
// greta tests:
double time_match(const std::string& re, const std::string& text);
double time_find_all(const std::string& re, const std::string& text);
}
namespace gs {
// safe greta tests:
double time_match(const std::string& re, const std::string& text);
double time_find_all(const std::string& re, const std::string& text);
}
namespace dxpr {
// dynamic xpressive tests:
double time_match(const std::string& re, const std::string& text);
double time_find_all(const std::string& re, const std::string& text);
}
namespace sxpr {
// static xpressive tests:
double time_match(const std::string& re, const std::string& text);
double time_find_all(const std::string& re, const std::string& text);
}
void test_match(const std::string& re, const std::string& text, const std::string& description);
void test_find_all(const std::string& re, const std::string& text, const std::string& description);
inline void test_match(const std::string& re, const std::string& text)
{ test_match(re, text, text); }
inline void test_find_all(const std::string& re, const std::string& text)
{ test_find_all(re, text, ""); }


#define REPEAT_COUNT 10

#endif
