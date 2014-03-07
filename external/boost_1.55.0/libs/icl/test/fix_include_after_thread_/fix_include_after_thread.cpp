/*-----------------------------------------------------------------------------+    
Copyright (c) 2011-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
//JODO REMOVE THIS TESTCASE

#define BOOST_TEST_MODULE icl::fix_include_after_thread unit test
#include <boost/config.hpp>
#include <boost/test/unit_test.hpp>

//Problem: If <boost/thread.hpp> is included before this
//example code, it influences compilation: Code that has
//compiled well, produces a syntax error error C2059 under 
//msvc-9/10. This can be fixed by enclosing subexpressions
//like some_attribute<Type>::value in parentheses
//  ->(some_attribute<Type>::value)
//The problem does not occur for gcc compilers.

// #include <boost/thread.hpp> MEMO: The problem occured when using thread.hpp
#include <boost/bind.hpp>    // but is also triggered from bind.hpp alone
    // while the cause of it is an error in the msvc-7.1 to 10.0 compilers.
    // A minimal example is provided by test case 'cmp_msvc_value_born_error'

//--- included code ---------------------------------------
template <class Type> struct some_attribute
{ 
    BOOST_STATIC_CONSTANT(int, value = 0); 
};

template <class Type> struct some_predicate
{ 
    BOOST_STATIC_CONSTANT(bool, 
        value = ((some_attribute<Type>::value) < 0) 
    //  value = ( some_attribute<Type>::value  < 0) 
                //error C2059: syntax error : ')' ONLY
                //IF <boost/thread.hpp> is included before
        ); 
};
//--- end of included code --------------------------------

BOOST_AUTO_TEST_CASE(dummy)
{
    BOOST_CHECK(true);
}

