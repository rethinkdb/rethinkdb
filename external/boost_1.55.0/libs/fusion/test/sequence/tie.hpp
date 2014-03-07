/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>

#if !defined(FUSION_AT)
#define FUSION_AT at_c
#endif

#if !defined(FUSION_MAKE)
#define FUSION_MAKE BOOST_PP_CAT(make_, FUSION_SEQUENCE)
#endif

#if !defined(FUSION_TIE)
#define FUSION_TIE BOOST_PP_CAT(FUSION_SEQUENCE, _tie)
#endif

namespace test_detail
{
    // something to prevent warnings for unused variables
    template<class T> void dummy(const T&) {}

    // no public default constructor
    class foo
    {
    public:

        explicit foo(int v) : val(v) {}

        bool operator==(const foo& other) const
        {
            return val == other.val;
        }

    private:

        foo() {}
        int val;
    };
}

void
test()
{
    using namespace boost::fusion;
    using namespace test_detail;

    int a;
    char b;
    foo c(5);

    FUSION_TIE(a, b, c) = FUSION_MAKE(2, 'a', foo(3));
    BOOST_TEST(a == 2);
    BOOST_TEST(b == 'a');
    BOOST_TEST(c == foo(3));

    FUSION_TIE(a, ignore, c) = FUSION_MAKE((short int)5, false, foo(5));
    BOOST_TEST(a == 5);
    BOOST_TEST(b == 'a');
    BOOST_TEST(c == foo(5));

    int i, j;
    FUSION_TIE(i, j) = FUSION_MAKE(1, 2);
    BOOST_TEST(i == 1 && j == 2);

    FUSION_SEQUENCE<int, int, float> ta;

#if defined(FUSION_TEST_FAIL)
    ta = std::FUSION_MAKE(1, 2); // should fail, tuple is of length 3, not 2
#endif

    dummy(ta);
    
    // ties cannot be rebound
    int d = 3;
    FUSION_SEQUENCE<int&> ti(a);
    BOOST_TEST(&FUSION_AT<0>(ti) == &a);
    ti = FUSION_SEQUENCE<int&>(d);
    BOOST_TEST(&FUSION_AT<0>(ti) == &a);
}
