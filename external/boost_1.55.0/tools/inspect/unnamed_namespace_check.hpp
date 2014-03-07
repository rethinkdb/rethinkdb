//  unnamed_namespace_check -----------------------------------------//

//  Copyright Gennaro Prota 2006.

//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNNAMED_NAMESPACE_CHECK_HPP_GP_20060718
#define BOOST_UNNAMED_NAMESPACE_CHECK_HPP_GP_20060718

#include "inspector.hpp"

namespace boost
{
  namespace inspect
  {
    class unnamed_namespace_check : public inspector
    {
      long m_errors;
    public:

      unnamed_namespace_check();
      virtual const char * name() const { return "*U*"; }
      virtual const char * desc() const { return "unnamed namespace in header"; }

      virtual void inspect(
        const std::string & library_name,
        const path & full_path,
        const std::string & contents );

      virtual ~unnamed_namespace_check()
        { std::cout << "  " << m_errors << " usages of unnamed namespaces in headers (including .ipp files)" << line_break(); }
    };
  }
}


#endif // include guard
