//  deprecated_macro_check header  --------------------------------------------------------//

//  Copyright Eric Niebler 2010.
//  Based on the apple_macro_check checker by Marshall Clow
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DEPRECATED_MACRO_CHECK_HPP
#define BOOST_DEPRECATED_MACRO_CHECK_HPP

#include "inspector.hpp"


namespace boost
{
  namespace inspect
  {
    class deprecated_macro_check : public inspector
    {
      long m_files_with_errors;
      bool m_from_boost_root;
    public:

      deprecated_macro_check();
      virtual const char * name() const { return "*DEPRECATED-MACROS*"; }
      virtual const char * desc() const { return "presence of deprecated BOOST macro in file (see docs for replacements)"; }

      virtual void inspect(
        const std::string & library_name,
        const path & full_path,
        const std::string & contents );

      virtual ~deprecated_macro_check()
        { std::cout << "  " << m_files_with_errors << " files with a deprecated BOOST macro" << line_break(); }
    };
  }
}

#endif // BOOST_DEPRECATED_MACRO_CHECK_HPP
