//  end_check header  ---------------------------------------------------------//

//  Copyright Beman Dawes 2002.
//  Copyright Rene Rivera 2004.
//  Copyright Daniel James 2009.
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_END_CHECK_HPP
#define BOOST_END_CHECK_HPP

#include "inspector.hpp"

namespace boost
{
  namespace inspect
  {
    class end_check : public inspector
    {
      long m_files_with_errors;
    public:

      end_check();
      virtual const char * name() const { return "*END*"; }
      virtual const char * desc() const { return "file doesn't end with a newline"; }

      virtual void inspect(
        const std::string & library_name,
        const path & full_path,
        const std::string & contents );

      virtual ~end_check()
        { std::cout << "  " << m_files_with_errors << " files that don't end with a newline" << line_break(); }
    };
  }
}

#endif // BOOST_END_CHECK_HPP
