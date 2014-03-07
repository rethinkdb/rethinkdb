/*-----------------------------------------------------------------------------+    
Copyright (c) 2011-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::cmp_clang_ttp_passing unit test
#include <libs/icl/test/disable_test_warnings.hpp>
#include <boost/config.hpp>
#include "../unit_test_unwarned.hpp"


namespace sep
{
    template<class T>class less{};

    template
    <
        class T, 
        template<class>class Less = sep::less
    >
    class interv
    {
    public:
        typedef interv<T,Less> type;
    };

    template
    <
        class T, 
        template<class>class Less = sep::less,
        class I = typename sep::interv<T,Less>::type
    >
    class cont
    {
    public:
        bool test()const { return true; }
    };
}//namespace sep

template
<
    template
    <
        class _T, 
        template<class>class _Less = sep::less,
        class I = typename sep::interv<_T,_Less>::type
    >
    class Cont,
    class T 
>
bool test_ttp()
{
    typedef Cont<T> cont_type; 
    cont_type test_cont;
    return test_cont.test();
};

BOOST_AUTO_TEST_CASE(dummy)
{
    bool result = test_ttp<sep::cont, int>();
    BOOST_CHECK( result );
}

