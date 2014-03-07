//  apple_macro_check implementation  ------------------------------------------------//

//  Copyright Marshall Clow 2007.
//  Based on the tab-check checker by Beman Dawes
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include "apple_macro_check.hpp"
#include <functional>
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/filesystem/operations.hpp"

namespace fs = boost::filesystem;

namespace
{
  boost::regex apple_macro_regex(
    "("
    "^\\s*#\\s*undef\\s*" // # undef
    "\\b(check|verify|require|check_error)\\b"     // followed by apple macro name, whole word
    ")"
    "|"                   // or (ignored)
    "("
    "//[^\\n]*"           // single line comments (//)
    "|"
    "/\\*.*?\\*/"         // multi line comments (/**/)
    "|"
    "\"(?:\\\\\\\\|\\\\\"|[^\"])*\"" // string literals
    ")"
    "|"                   // or
    "("
    "\\b(check|verify|require|check_error)\\b" // apple macro name, whole word
    "\\s*\\("         // followed by 0 or more spaces and an opening paren
    ")"
    , boost::regex::normal);

} // unnamed namespace


namespace boost
{
  namespace inspect
  {
   apple_macro_check::apple_macro_check() : m_files_with_errors(0)
   {
     register_signature( ".c" );
     register_signature( ".cpp" );
     register_signature( ".cxx" );
     register_signature( ".h" );
     register_signature( ".hpp" );
     register_signature( ".hxx" );
     register_signature( ".ipp" );
   }

   void apple_macro_check::inspect(
      const string & library_name,
      const path & full_path,   // example: c:/foo/boost/filesystem/path.hpp
      const string & contents )     // contents of file to be inspected
    {
      if (contents.find( "boostinspect:" "naapple_macros" ) != string::npos) return;

      // Only check files in the boost directory, as we can avoid including the
      // apple test headers elsewhere.
      path relative( relative_to( full_path, fs::initial_path() ) );
      if ( relative.empty() || *relative.begin() != "boost") return;

      boost::sregex_iterator cur(contents.begin(), contents.end(), apple_macro_regex), end;

      long errors = 0;

      for( ; cur != end; ++cur /*, ++m_files_with_errors*/ )
      {

        if(!(*cur)[3].matched)
        {
          string::const_iterator it = contents.begin();
          string::const_iterator match_it = (*cur)[0].first;

          string::const_iterator line_start = it;

          string::size_type line_number = 1;
          for ( ; it != match_it; ++it) {
              if (string::traits_type::eq(*it, '\n')) {
                  ++line_number;
                  line_start = it + 1; // could be end()
              }
          }

          ++errors;
          error( library_name, full_path, 
            "Apple macro clash: " + std::string((*cur)[0].first, (*cur)[0].second-1),
            line_number );
        }
      }
      if(errors > 0) {
        ++m_files_with_errors;
      }
    }
  } // namespace inspect
} // namespace boost


