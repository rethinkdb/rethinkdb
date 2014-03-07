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

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         info.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Error handling for test cases.
  */

#ifndef BOOST_REGEX_REGRESS_INFO_HPP
#define BOOST_REGEX_REGRESS_INFO_HPP
#include <iostream>
#include <string>
#include <boost/regex.hpp>

#ifdef TEST_THREADS
#include <boost/thread/once.hpp>
#include <boost/thread.hpp>
#endif
//
// class test info, 
// store information about the test we are about to conduct:
//
template <class charT>
class test_info_base
{
public:
   typedef std::basic_string<charT> string_type;
private:
   struct data_type
   {
      std::string file;
      int line;
      string_type expression;
      boost::regex_constants::syntax_option_type options;
      string_type search_text;
      boost::regex_constants::match_flag_type match_options;
      const int* answer_table;
      string_type format_string;
      string_type result_string;
      bool need_to_print;
      std::string expression_type_name;
   };
#ifdef TEST_THREADS
   static data_type& do_get_data()
   {
      static boost::thread_specific_ptr<data_type> pd;
      if(pd.get() == 0)
         pd.reset(new data_type());
      return *(pd.get());
   }
   static void init_data()
   {
      do_get_data();
   }
#endif
   static data_type& data()
   {
#ifdef TEST_THREADS
      static boost::once_flag f = BOOST_ONCE_INIT;
      boost::call_once(f,&init_data);
      return do_get_data();
#else
      static data_type d;
      return d;
#endif
   }
public:
   test_info_base(){};
   static void set_info(
      const char* file,
      int line,
      const string_type& ex,
      boost::regex_constants::syntax_option_type opt,
      const string_type& search_text = string_type(),
      boost::regex_constants::match_flag_type match_options = boost::match_default,
      const int* answer_table = 0,
      const string_type& format_string = string_type(),
      const string_type& result_string = string_type())
   {
      data_type& dat = data();
      dat.file = file;
      dat.line = line;
      dat.expression = ex;
      dat.options = opt;
      dat.search_text = search_text;
      dat.match_options = match_options;
      dat.answer_table = answer_table;
      dat.format_string = format_string;
      dat.result_string = result_string;
      dat.need_to_print = true;
   }
   static void set_typename(const std::string& n)
   {
      data().expression_type_name = n;
   }

   static const string_type& expression()
   {
      return data().expression;
   }
   static boost::regex_constants::syntax_option_type syntax_options()
   {
      return data().options;
   }
   static const string_type& search_text()
   {
      return data().search_text;
   }
   static boost::regex_constants::match_flag_type match_options()
   {
      return data().match_options;
   }
   static const int* answer_table()
   {
      return data().answer_table;
   }
   static const string_type& format_string()
   {
      return data().format_string;
   }
   static const string_type& result_string()
   {
      return data().result_string;
   }  
   static bool need_to_print()
   {
      return data().need_to_print;
   }
   static const std::string& file()
   {
      return data().file;
   }
   static int line()
   {
      return data().line;
   }
   static void clear()
   {
      data().need_to_print = false;
   }
   static std::string& expression_typename()
   {
      return data().expression_type_name;
   }
};

template <class T>
struct test_info
   : public test_info_base<wchar_t>
{};

template<>
struct test_info<char>
   : public test_info_base<char>
{};

#if BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042))

// Some template instantiation modes (namely local, implicit local, and weak) of
// this compiler need an explicit instantiation because otherwise we end up with
// multiple copies of the static variable defined in this method. This explicit
// instantiation generates the static variable with common linkage, which makes
// the linker choose only one of the available definitions. For more details,
// see "man ld".

template test_info_base<wchar_t>::data_type & test_info_base<wchar_t>::data();
template test_info_base<char>::data_type & test_info_base<char>::data();

#endif

template <class charT>
std::ostream& operator<<(std::ostream& os, const test_info<charT>&)
{
   if(test_info<charT>::need_to_print())
   {
      os << test_info<charT>::file() << ":" << test_info<charT>::line() << ": Error in test here:" << std::endl;
      test_info<charT>::clear();
   }
   return os;
}
//
// define some test macros:
//
extern int error_count;

#define BOOST_REGEX_TEST_ERROR(msg, charT)\
   ++error_count;\
   std::cerr << test_info<charT>();\
   std::cerr << "  " << __FILE__ << ":" << __LINE__ << ":" << msg \
             << " (While testing " << test_info<charT>::expression_typename() << ")" << std::endl

class errors_as_warnings
{
public:
   errors_as_warnings()
   {
      m_saved_error_count = error_count;
   }
   ~errors_as_warnings()
   {
      if(m_saved_error_count != error_count)
      {
         std::cerr << "<note>The above " << (error_count - m_saved_error_count) << " errors are treated as warnings only.</note>" << std::endl;
         error_count = m_saved_error_count;
      }
   }
private:
   int m_saved_error_count;
};

#endif

