//  tab_check implementation  ------------------------------------------------//

//  Copyright Beman Dawes 2002.
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include "tab_check.hpp"

namespace boost
{
  namespace inspect
  {
   tab_check::tab_check() : m_files_with_errors(0)
   {
     register_signature( ".c" );
     register_signature( ".cpp" );
     register_signature( ".cxx" );
     register_signature( ".h" );
     register_signature( ".hpp" );
     register_signature( ".hxx" );
     register_signature( ".ipp" );
     register_signature( "Jamfile" );
     register_signature( ".py" );
   }

   void tab_check::inspect(
      const string & library_name,
      const path & full_path,   // example: c:/foo/boost/filesystem/path.hpp
      const string & contents )     // contents of file to be inspected
    {
      if (contents.find( "boostinspect:" "notab" ) != string::npos) return;

      if ( contents.find( '\t' ) != string::npos )
      {
        ++m_files_with_errors;
        error( library_name, full_path, name() );
      }
    }
  } // namespace inspect
} // namespace boost


