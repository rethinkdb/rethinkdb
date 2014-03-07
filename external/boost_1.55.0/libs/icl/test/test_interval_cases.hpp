/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef __test_IntervalT_hpp_JOFA_100401__
#define __test_IntervalT_hpp_JOFA_100401__

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_ctor_4_ordered_types, T, ORDERED_TYPES)
{         interval_ctor_4_ordered_types<T, std::less, INTERVAL>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_ctor_4_bicremental_types, T, BICREMENTAL_TYPES)
{         interval_ctor_4_bicremental_types<T, std::less, INTERVAL>();}

#endif // __test_IntervalT_hpp_JOFA_100401__
