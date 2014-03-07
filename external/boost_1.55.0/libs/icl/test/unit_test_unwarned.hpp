/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_UNIT_TEST_UNWARNED_HPP_JOFA_091204
#define BOOST_ICL_TEST_UNIT_TEST_UNWARNED_HPP_JOFA_091204

#include <boost/icl/detail/boost_config.hpp>
#include <boost/detail/workaround.hpp>

#ifdef BOOST_MSVC 
#pragma warning(push)
#pragma warning(disable:4389) // boost/test/test_tools.hpp(509) : warning C4389: '==' : signed/unsigned mismatch
#pragma warning(disable:4996) // 'std::_Traits_helper::copy_s': Function call with parameters that may be unsafe
#endif                        

#include <boost/test/unit_test.hpp>

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


#endif // BOOST_ICL_TEST_UNIT_TEST_UNWARNED_HPP_JOFA_091204

