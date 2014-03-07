/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------+
Auxiliary functions to reduce redundancies in test case code.
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_FUNCTIONS_H_JOFA_091003
#define BOOST_ICL_TEST_FUNCTIONS_H_JOFA_091003

#include <boost/icl/detail/notate.hpp>
#include <boost/icl/type_traits/interval_type_default.hpp>
#include <boost/icl/discrete_interval.hpp>

#include <boost/icl/type_traits/identity_element.hpp>
#include <boost/icl/functors.hpp>
#include "portability.hpp"


namespace boost{namespace icl
{

template <class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
          ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap,
#else
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap,
#endif
          class SequenceT
>
void itl_map_copy(const SequenceT& segments, 
                  ICL_PORT_msvc_7_1_IntervalMap(T,U,Trt)& destination) 
{
    ICL_const_FORALL(typename SequenceT, segment_, segments)
        destination.insert(*segment_);
}


template <class T, class U, class Trt,
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
          ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap,
#else
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap,
#endif
          class SequenceT
>
void test_interval_map_copy_via_inserter(const SequenceT& segments, 
                                         ICL_PORT_msvc_7_1_IntervalMap(T,U,Trt)& std_copied_map)
{
    // The second parameter (std_copied_map) could be omitted and is only held as a 
    // local variable. It is there to help gcc-3.4.4 resolving the function template type.
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    IntervalMapT looped_copied_map;
    std_copied_map.clear();
    itl_map_copy(segments, looped_copied_map);
    std::copy(segments.begin(), segments.end(), std::inserter(std_copied_map, std_copied_map.end()));
    BOOST_CHECK_EQUAL( looped_copied_map, std_copied_map );
}

}} // namespace icl boost

#endif // BOOST_ICL_TEST_FUNCTIONS_H_JOFA_091003

