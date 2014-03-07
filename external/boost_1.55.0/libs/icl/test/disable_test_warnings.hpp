/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_DETAIL_DISABLE_TEST_WARNINGS_HPP_JOFA_101031
#define BOOST_ICL_DETAIL_DISABLE_TEST_WARNINGS_HPP_JOFA_101031

#include <boost/icl/detail/boost_config.hpp>
#include <boost/detail/workaround.hpp>

#ifdef BOOST_MSVC             // These warnings are disabled permanently for tests.
#pragma warning(disable:4996) // Function call with parameters that may be unsafe
#pragma warning(disable:4503) // decorated name length exceeded, name was truncated
#pragma warning(disable:4702) // unreachable code
#endif                        


#endif // BOOST_ICL_DETAIL_DISABLE_TEST_WARNINGS_HPP_JOFA_101031

