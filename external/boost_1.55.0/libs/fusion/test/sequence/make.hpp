/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <string>

#if !defined(FUSION_AT)
#define FUSION_AT at_c
#endif

#if !defined(FUSION_MAKE)
#define FUSION_MAKE BOOST_PP_CAT(make_, FUSION_SEQUENCE)
#endif

namespace test_detail
{
    // something to prevent warnings for unused variables
    template<class T> void dummy(const T&) {}

    class A {};
    class B {};
}

void make_tuple_test() {}

void
test()
{
    using namespace boost::fusion;
    using namespace test_detail;

    {
        FUSION_SEQUENCE<int, char> t1 = FUSION_MAKE(5, 'a');
        BOOST_TEST(FUSION_AT<0>(t1) == 5);
        BOOST_TEST(FUSION_AT<1>(t1) == 'a');

        FUSION_SEQUENCE<int, std::string> t2;
        t2 = FUSION_MAKE((short int)2, std::string("Hi"));
        BOOST_TEST(FUSION_AT<0>(t2) == 2);
        BOOST_TEST(FUSION_AT<1>(t2) == "Hi");
    }

    {   // This test was previously disallowed for non-PTS compilers.
        A a = A(); B b;
        const A ca = a;
        FUSION_MAKE(boost::cref(a), b);
        FUSION_MAKE(boost::ref(a), b);
        FUSION_MAKE(boost::ref(a), boost::cref(b));
        FUSION_MAKE(boost::ref(ca));
    }

    {   //  the result of make_xxx is assignable: 
        BOOST_TEST(FUSION_MAKE(2, 4, 6) ==
            (FUSION_MAKE(1, 2, 3) = FUSION_MAKE(2, 4, 6)));
    }

    {   // This test was previously disallowed for non-PTS compilers.
        FUSION_MAKE("Donald", "Daisy"); // should work;
    //  std::make_pair("Doesn't","Work"); // fails
    }

    {
        // You can store a reference to a function in a sequence
        FUSION_SEQUENCE<void(&)()> adf(make_tuple_test);
        dummy(adf); // avoid warning for unused variable
    }

#if defined(FUSION_TEST_FAIL)
    {
        //  But make_xxx doesn't work
        //  with function references, since it creates a const
        //  qualified function type

        FUSION_MAKE(make_tuple_test);
    }
#endif
    
    {
        // With function pointers, make_xxx works just fine
        FUSION_MAKE(&make_tuple_test);
    }
}
