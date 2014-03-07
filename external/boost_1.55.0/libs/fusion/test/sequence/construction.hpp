/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/container/list/cons.hpp>

#if !defined(FUSION_AT)
#define FUSION_AT at_c
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

    // another class without a public default constructor
    class no_def_constructor
    {
        no_def_constructor() {}

    public:

        no_def_constructor(std::string) {}
    };
}

inline void
test()
{
    using namespace boost::fusion;
    using namespace test_detail;

    nil empty;

    FUSION_SEQUENCE<> empty0;

#ifndef NO_CONSTRUCT_FROM_NIL
    FUSION_SEQUENCE<> empty1(empty);
#endif

    FUSION_SEQUENCE<int> t1;
    BOOST_TEST(FUSION_AT<0>(t1) == int());

    FUSION_SEQUENCE<float> t2(5.5f);
    BOOST_TEST(FUSION_AT<0>(t2) > 5.4f && FUSION_AT<0>(t2) < 5.6f);

    FUSION_SEQUENCE<foo> t3(foo(12));
    BOOST_TEST(FUSION_AT<0>(t3) == foo(12));

    FUSION_SEQUENCE<double> t4(t2);
    BOOST_TEST(FUSION_AT<0>(t4) > 5.4 && FUSION_AT<0>(t4) < 5.6);

    FUSION_SEQUENCE<int, float> t5;
    BOOST_TEST(FUSION_AT<0>(t5) == int());
    BOOST_TEST(FUSION_AT<1>(t5) == float());

    FUSION_SEQUENCE<int, float> t6(12, 5.5f);
    BOOST_TEST(FUSION_AT<0>(t6) == 12);
    BOOST_TEST(FUSION_AT<1>(t6) > 5.4f && FUSION_AT<1>(t6) < 5.6f);

    FUSION_SEQUENCE<int, float> t7(t6);
    BOOST_TEST(FUSION_AT<0>(t7) == 12);
    BOOST_TEST(FUSION_AT<1>(t7) > 5.4f && FUSION_AT<1>(t7) < 5.6f);

    FUSION_SEQUENCE<long, double> t8(t6);
    BOOST_TEST(FUSION_AT<0>(t8) == 12);
    BOOST_TEST(FUSION_AT<1>(t8) > 5.4f && FUSION_AT<1>(t8) < 5.6f);

    dummy
    (
        FUSION_SEQUENCE<no_def_constructor, no_def_constructor, no_def_constructor>(
            std::string("Jaba"),        // ok, since the default
            std::string("Daba"),        // constructor is not used
            std::string("Doo")
        )
    );

    dummy(FUSION_SEQUENCE<int, double>());
    dummy(FUSION_SEQUENCE<int, double>(1,3.14));

#if defined(FUSION_TEST_FAIL)
    dummy(FUSION_SEQUENCE<double&>());          // should fail, no defaults for references
    dummy(FUSION_SEQUENCE<const double&>());    // likewise
#endif

    {
        double dd = 5;
        dummy(FUSION_SEQUENCE<double&>(dd)); // ok
        dummy(FUSION_SEQUENCE<const double&>(dd+3.14)); // ok, but dangerous
    }

#if defined(FUSION_TEST_FAIL)
    dummy(FUSION_SEQUENCE<double&>(dd+3.14));   // should fail,
                                                // temporary to non-const reference
#endif
}
