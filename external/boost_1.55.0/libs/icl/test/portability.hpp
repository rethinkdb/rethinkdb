/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_PORTABILITY_HPP_JOFA_101111
#define BOOST_ICL_TEST_PORTABILITY_HPP_JOFA_101111

#include <boost/icl/detail/design_config.hpp>

// This file contains auxiliary macros to help with portability problems
// It is not designed to for general use but only in the context of test
// code for the ICL. There will be a number of specific assumptions about
// e.g. template parameter names that are only valid for icl tests.

//PORT: msvc-7.1: For local template template parameters, msvc-7.1 does not 
// accept a subsequent instantiation of that parameter using default arguments e.g.:
// test_functions.hpp(37) : error C2976: 'IntervalMap' : too few template arguments

//ASSUMPTION: Fixed name IntervalMap
#define ICL_PORT_msvc_7_1_IntervalMap(tp_T, tp_U, tp_Trt) \
IntervalMap<tp_T, tp_U, tp_Trt \
           ,ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, tp_T) \
           ,ICL_COMBINE_INSTANCE(icl::inplace_plus, tp_U) \
           ,ICL_SECTION_INSTANCE(icl::inter_section, tp_U) \
           ,ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, tp_T, ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, tp_T)) \
           ,std::allocator>

//ASSUMPTION: Fixed name IntervalSet
#define ICL_PORT_msvc_7_1_IntervalSet(tp_T) \
IntervalSet<tp_T \
           ,ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, tp_T) \
           ,ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, tp_T, ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, tp_T)) \
           ,std::allocator>


//------------------------------------------------------------------------------
// Signature Macros: Help reducing redundancies in template headers
//------------------------------------------------------------------------------
#define ICL_IntervalMap_TEMPLATE(tp_T, tp_U, tp_Traits, tp_Trt) \
template<class tp_T, class tp_U, \
       class tp_Traits = tp_Trt, \
       ICL_COMPARE Compare = ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, tp_T), \
       ICL_COMBINE Combine = ICL_COMBINE_INSTANCE(icl::inplace_plus, tp_U), \
       ICL_SECTION Section = ICL_SECTION_INSTANCE(icl::inter_section, tp_U), \
       ICL_INTERVAL(ICL_COMPARE)  Interval = ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, tp_T, Compare), \
       ICL_ALLOC   Alloc   = std::allocator>class


#ifndef ICL_INTERVAL_BITSET_IMPL

#   define ICL_IntervalSet_TEMPLATE(tp_T) \
    template<class tp_T, \
        ICL_COMPARE Compare = ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, tp_T), \
        ICL_INTERVAL(ICL_COMPARE)  Interval = ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, tp_T, Compare), \
        ICL_ALLOC   Alloc   = std::allocator>class 

#else

#   define ICL_IntervalSet_TEMPLATE(tp_T) \
    template<class tp_T, \
        class BitSetT = icl::bits<unsigned long>, \
        ICL_COMPARE Compare = ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, tp_T), \
        ICL_INTERVAL(ICL_COMPARE)  Interval = ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, tp_T, Compare), \
        ICL_ALLOC   Alloc   = std::allocator>class 

#endif //ICL_INTERVAL_BITSET_IMPL

#endif // BOOST_ICL_TEST_PORTABILITY_HPP_JOFA_101111
