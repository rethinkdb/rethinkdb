//  end_check implementation  -------------------------------------------------//

//  Copyright Beman Dawes 2002.
//  Copyright Daniel James 2009.
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include "end_check.hpp"
#include <boost/next_prior.hpp>

namespace boost
{
  namespace inspect
  {
   end_check::end_check() : m_files_with_errors(0)
   {
     register_signature( ".c" );
     register_signature( ".cpp" );
     register_signature( ".cxx" );
     register_signature( ".h" );
     register_signature( ".hpp" );
     register_signature( ".hxx" );
     register_signature( ".ipp" );
   }

   void end_check::inspect(
      const string & library_name,
      const path & full_path,   // example: c:/foo/boost/filesystem/path.hpp
      const string & contents )     // contents of file to be inspected
    {
      if (contents.find( "boostinspect:" "noend" ) != string::npos) return;

      // this file deliberately contains errors
      const char test_file_name[] = "wrong_line_ends_test.cpp";

      char final_char = contents.begin() == contents.end() ? '\0'
        : *(boost::prior(contents.end()));

      bool failed = final_char != '\n' && final_char != '\r';

      if (failed && full_path.leaf() != test_file_name)
      {
        ++m_files_with_errors;
        error( library_name, full_path, string(name()) + ' ' + desc() );
      }

      if (!failed && full_path.leaf() == test_file_name)
      {
        ++m_files_with_errors;
        error( library_name, full_path, string(name()) + " should not end with a newline" );
      }
    }
  } // namespace inspect
} // namespace boost


