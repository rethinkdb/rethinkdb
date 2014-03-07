//  ascii_check header  --------------------------------------------------------//

//  Copyright Marshall Clow 2007.
//  Based on the tab-check checker by Beman Dawes
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASCII_CHECK_HPP
#define BOOST_ASCII_CHECK_HPP

#include "inspector.hpp"

namespace boost
{
  namespace inspect
  {
    class ascii_check : public inspector
    {
      long m_files_with_errors;
    public:

      ascii_check();
      virtual const char * name() const { return "*ASCII*"; }
      virtual const char * desc() const { return "non-ASCII chars in file"; }

      virtual void inspect(
        const std::string & library_name,
        const path & full_path,
        const std::string & contents );

      virtual ~ascii_check()
        { std::cout << "  " << m_files_with_errors << " files with non-ASCII chars" << line_break(); }
    };
  }
}

#endif // BOOST_TAB_CHECK_HPP
