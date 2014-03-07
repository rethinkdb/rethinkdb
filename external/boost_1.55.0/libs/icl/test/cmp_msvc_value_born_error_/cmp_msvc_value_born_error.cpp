/*-----------------------------------------------------------------------------+    
Copyright (c) 2011-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::cmp_msvc_value_born_error unit test
#include <boost/config.hpp>
#include "../unit_test_unwarned.hpp"

namespace unhelpful{
    // This declaration of a class template will cause
    // the compilation of line 17 to fail with syntax error C2059
    template<class T> class value{};
}

//--- affected code ---------------------------------------
template <class Type> struct meta_attribute
{ 
    BOOST_STATIC_CONSTANT(int, value = 0); 
};

template <class Type> struct meta_predicate
{ 
    BOOST_STATIC_CONSTANT(bool, value = 
        ( meta_attribute<Type>::value  < 1)
        //error C2059: syntax error : ')'
        //IF class template value declared before
    //  ((meta_attribute<Type>::value) < 1) // Remedy#1 enclose into ()
    //  ( meta_attribute<Type>::value  <=0) // Remedy#2 use operator <= 
        );
};

BOOST_AUTO_TEST_CASE(dummy)
{
    BOOST_CHECK( meta_predicate<int>::value );
}

