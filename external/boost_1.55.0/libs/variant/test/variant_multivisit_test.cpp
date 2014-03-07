//-----------------------------------------------------------------------------
// boost-libs variant/test/variant_multivisit_test.cpp source file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013
// Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "boost/config.hpp"
#define BOOST_VARAINT_MAX_MULTIVIZITOR_PARAMS 5
#include "boost/variant/multivisitors.hpp"

#include "boost/test/minimal.hpp"

typedef boost::variant<char, unsigned char, signed char, unsigned short, int, unsigned int>         variant6_t;

struct test_visitor: boost::static_visitor<> {
    // operators that shall not be called
    template <class  T1, class  T2, class  T3>
    void operator()(T1, T2, T3) const 
    {
        BOOST_CHECK(false);
    }

    template <class  T1, class  T2, class  T3, class  T4>
    void operator()(T1, T2, T3, T4) const 
    {
        BOOST_CHECK(false);
    }

    template <class  T1, class  T2, class  T3, class  T4, class  T5>
    void operator()(T1, T2, T3, T4, T5) const 
    {
        BOOST_CHECK(false);
    }

    // operators that are OK to call
    void operator()(char v0, unsigned char v1, signed char v2) const 
    {
        BOOST_CHECK(v0 == 0);
        BOOST_CHECK(v1 == 1);
        BOOST_CHECK(v2 == 2);
    }

    void operator()(char v0, unsigned char v1, signed char v2, unsigned short v3) const 
    {
        BOOST_CHECK(v0 == 0);
        BOOST_CHECK(v1 == 1);
        BOOST_CHECK(v2 == 2);
        BOOST_CHECK(v3 == 3);
    }

    void operator()(char v0, unsigned char v1, signed char v2, unsigned short v3, int v4) const 
    {
        BOOST_CHECK(v0 == 0);
        BOOST_CHECK(v1 == 1);
        BOOST_CHECK(v2 == 2);
        BOOST_CHECK(v3 == 3);
        BOOST_CHECK(v4 == 4);
    }
};

typedef boost::variant<int, double, bool> bool_like_t;
typedef boost::variant<int, double> arithmetics_t;

struct if_visitor: public boost::static_visitor<arithmetics_t> {
    template <class T1, class T2>
    arithmetics_t operator()(bool b, T1 v1, T2 v2) const {
        if (b) {
            return v1;
        } else {
            return v2;
        }
    }
};


int test_main(int , char* [])
{
    test_visitor v;

    variant6_t v_array6[6];
    v_array6[0] = char(0);
    v_array6[1] = static_cast<unsigned char>(1);
    v_array6[2] = static_cast<signed char>(2);
    v_array6[3] = static_cast<unsigned short>(3);
    v_array6[4] = static_cast<int>(4);
    v_array6[5] = static_cast<unsigned int>(5);

    boost::apply_visitor(v, v_array6[0], v_array6[1], v_array6[2]);
    boost::apply_visitor(test_visitor(), v_array6[0], v_array6[1], v_array6[2]);

// Following test also pass, but requires many Gigabytes of RAM for compilation and compile for about 15 minutes
//#define BOOST_VARIANT_MULTIVISITORS_TEST_VERY_EXTREME
#ifdef BOOST_VARIANT_MULTIVISITORS_TEST_VERY_EXTREME    
    boost::apply_visitor(v, v_array6[0], v_array6[1], v_array6[2], v_array6[3]);
    boost::apply_visitor(test_visitor(), v_array6[0], v_array6[1], v_array6[2], v_array6[3]);

    boost::apply_visitor(v, v_array6[0], v_array6[1], v_array6[2], v_array6[3], v_array6[4]);
    boost::apply_visitor(test_visitor(), v_array6[0], v_array6[1], v_array6[2], v_array6[3], v_array6[4]);
#endif

    bool_like_t v0(1), v1(true), v2(1.0);

    BOOST_CHECK(
        boost::apply_visitor(if_visitor(), v0, v1, v2)
        ==
        arithmetics_t(true)
    );

    /* Delayed multi visitation is not implemented
    if_visitor if_vis;
    BOOST_CHECK(
        boost::apply_visitor(if_vis)(v0, v1, v2)
        ==
        arithmetics_t(true)
    );
    */
    return boost::exit_success;
}
