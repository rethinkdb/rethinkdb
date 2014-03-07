//  license_check implementation  --------------------------------------------//

//  Copyright Beman Dawes 2002-2003.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include "boost/regex.hpp"
#include "license_check.hpp"

namespace
{
  boost::regex license_regex(
    //~ The next two lines change the regex so that it detects when the license
    //~ doesn't follow the prefered statement. Disabled because it currently
    //~ generates a large number of issues.
    //~ "Distributed[\\s\\W]+"
    //~ "under[\\s\\W]+the[\\s\\W]+"
    "boost[\\s\\W]+software[\\s\\W]+license",
    boost::regbase::normal | boost::regbase::icase);

} // unnamed namespace

namespace boost
{
  namespace inspect
  {
   license_check::license_check() : m_files_with_errors(0)
   {
   }

   void license_check::inspect(
      const string & library_name,
      const path & full_path,   // example: c:/foo/boost/filesystem/path.hpp
      const string & contents )     // contents of file to be inspected
    {
      if (contents.find( "boostinspect:" "nolicense" ) != string::npos) return;

      if ( !boost::regex_search( contents, license_regex ) )
      {
        ++m_files_with_errors;
        error( library_name, full_path, name() );
      }
    }
  } // namespace inspect
} // namespace boost


