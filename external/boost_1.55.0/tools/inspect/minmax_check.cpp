//  minmax_check implementation  --------------------------------------------//

//  Copyright Beman Dawes   2002.
//  Copyright Gennaro Prota 2006.
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


#include <algorithm>

#include "minmax_check.hpp"
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"

namespace
{
  boost::regex minmax_regex(
    "("
    "^\\s*#\\s*undef\\s*" // # undef
    "\\b(min|max)\\b"     // followed by min or max, whole word
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
    "\\b(min|max)\\b" // min or max, whole word
    "\\s*\\("         // followed by 0 or more spaces and an opening paren
    ")"
    , boost::regex::normal);

} // unnamed namespace

namespace boost
{
  namespace inspect
  {

    //  minmax_check constructor  -------------------------------------------//

    minmax_check::minmax_check()
      : m_errors(0)
    {
      // C/C++ source code...
      register_signature( ".c" );
      register_signature( ".cpp" );
      register_signature( ".cxx" );
      register_signature( ".h" );
      register_signature( ".hpp" );
      register_signature( ".hxx" );
      register_signature( ".inc" );
      register_signature( ".ipp" );
    }

    //  inspect ( C++ source files )  ---------------------------------------//

    void minmax_check::inspect(
      const string & library_name,
      const path & full_path,      // example: c:/foo/boost/filesystem/path.hpp
      const string & contents)     // contents of file to be inspected
    {
      if (contents.find( "boostinspect:" "nominmax" ) != string::npos) return;

      boost::sregex_iterator cur(contents.begin(), contents.end(), minmax_regex), end;

      for( ; cur != end; ++cur /*, ++m_errors*/ )
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

          ++m_errors;
          error( library_name, full_path, string(name())
              + " violation of Boost min/max guidelines on line "
              + boost::lexical_cast<string>( line_number ) );
        }

      }
    }

  } // namespace inspect
} // namespace boost

