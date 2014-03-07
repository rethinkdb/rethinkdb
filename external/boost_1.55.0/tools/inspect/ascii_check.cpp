//  ascii_check implementation  ------------------------------------------------//

//  Copyright Marshall Clow 2007.
//  Based on the tab-check checker by Beman Dawes
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//  √ -- this is a test.

#include "ascii_check.hpp"
#include <functional>

namespace boost
{
  namespace inspect
  {

    static const string gPunct ( "$_{}[]#()<>%:;.?*+-/ˆ&|~!=,\\\"'@^`" );
    
   // Legal characters for a source file are defined in section 2.2 of the standard
   // I have added '@', '^', and '`' to the "legal" chars because they are commonly
   //    used in comments, and they are strictly ASCII.
   struct non_ascii : public std::unary_function<char, bool> {
   public:
      non_ascii () {}
      ~non_ascii () {}
      bool operator () ( char c ) const
      {
         if ( c == ' ' ) return false;
         if ( c >= 'a' && c <= 'z' ) return false;
         if ( c >= 'A' && c <= 'Z' ) return false;
         if ( c >= '0' && c <= '9' ) return false;
      // Horizontal/Vertical tab, newline, and form feed
         if ( c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f' ) return false;
         return gPunct.find ( c ) == string::npos;
      }
   };

   struct is_CRLF : public std::unary_function<char, bool> {
   public:
      is_CRLF () {}
      ~is_CRLF () {}
      bool operator () ( char c ) const
      {
         return c == '\015' || c == '\012';
      }
   };

   const char *kCRLF = "\012\015";
   
// Given a position in the file, extract and return the line
   std::string find_line ( const std::string &contents, std::string::const_iterator iter_pos )
   {
      std::size_t pos = iter_pos - contents.begin ();
      
   // Search backwards for a CR or LR
      std::size_t start_pos = contents.find_last_of ( kCRLF, pos );
      std::string::const_iterator line_start = contents.begin () + ( start_pos == std::string::npos ? 0 : start_pos + 1 );
      

   // Search forwards for a CR or LF
      std::size_t end_pos = contents.find_first_of ( kCRLF, pos + 1 );
      std::string::const_iterator line_end;
      if ( end_pos == std::string::npos )
         line_end = contents.end ();
      else
         line_end = contents.begin () + end_pos - 1;

      return std::string ( line_start, line_end );
   }

   ascii_check::ascii_check() : m_files_with_errors(0)
   {
     register_signature( ".c" );
     register_signature( ".cpp" );
     register_signature( ".cxx" );
     register_signature( ".h" );
     register_signature( ".hpp" );
     register_signature( ".hxx" );
     register_signature( ".ipp" );
   }

   void ascii_check::inspect(
      const string & library_name,
      const path & full_path,   // example: c:/foo/boost/filesystem/path.hpp
      const string & contents )     // contents of file to be inspected
    {
      if (contents.find( "boostinspect:" "noascii" ) != string::npos) return;
      string::const_iterator bad_char = std::find_if ( contents.begin (), contents.end (), non_ascii ());
      if ( bad_char != contents.end ())
      {
        ++m_files_with_errors;
        int ln = std::count( contents.begin(), bad_char, '\n' ) + 1;
        string the_line = find_line ( contents, bad_char );
        error( library_name, full_path, "Non-ASCII: " + the_line, ln );
      }
    }
  } // namespace inspect
} // namespace boost


