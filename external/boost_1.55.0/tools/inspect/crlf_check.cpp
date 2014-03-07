//  crlf_check implementation  ------------------------------------------------//

//  Copyright Beman Dawes 2002.
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  Contributed by Joerg Walter

#include "crlf_check.hpp"

namespace boost
{
  namespace inspect
  {
   crlf_check::crlf_check() : m_files_with_errors(0)
   {
   }

   void crlf_check::inspect(
      const string & library_name,
      const path & full_path,   // example: c:/foo/boost/filesystem/path.hpp
      const string & contents )     // contents of file to be inspected
    {
      if (contents.find( "boostinspect:" "nocrlf" ) != string::npos) return;

      // this file deliberately contains errors
      const char test_file_name[] = "wrong_line_ends_test.cpp";

      bool failed = false;
      // The understanding on line endings, as I remember it, was that
      // either "\n" or "\r\n" is OK, and they can be mixed, but "\r" alone
      // is not acceptable. Mixed line endings are allowed because Boost files
      // are commonly edited in both Windows and UNIX environments, and editors
      // in those environments generally accept either ending. Even Mac people
      // agreed with this policy. --Beman

      // Joerg's original implementation is saved below,
      // in case we change our minds!

      for ( std::string::const_iterator itr ( contents.begin() );
        itr != contents.end(); ++itr )
      {
        if ( *itr == '\r' && ((itr+1) == contents.end() || *(itr+1) != '\n') )
        {
          failed = true;
          break;
        }
      }

      if (failed && full_path.leaf() != test_file_name)
      {
        ++m_files_with_errors;
        error( library_name, full_path, name() );
      }

      if (!failed && full_path.leaf() == test_file_name)
      {
        ++m_files_with_errors;
        error( library_name, full_path, string(name()) + " should have cr-only line endings" );
      }

/*
      size_t cr_count = 0;
      size_t lf_count = 0;
      size_t crlf_count = 0;
      bool had_cr = false;
      for ( size_t i = 0; i < contents.length(); ++i )
      {
        switch ( contents[i] )
        {
          case '\r':
            had_cr = true;
            ++cr_count;
            break;
          case '\n':
            ++lf_count;
            if ( had_cr )
              ++crlf_count;
            // fallthrough
          default:
            had_cr = false;
            break;
        }
      }
      if ( cr_count > 0 && lf_count != crlf_count )
      {
        ++m_files_with_errors;
        error( library_name, full_path, desc() );
      }
*/
    }
  } // namespace inspect
} // namespace boost


