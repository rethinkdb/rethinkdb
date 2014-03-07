/*
Copyright Redshift Software Inc 2011-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#include <exception>
#include <vector>
#include <string>
#include <iostream>

namespace
{
    struct test_info
    {
        std::string value;
        bool passed;

        test_info(std::string const & v, bool p) : value(v), passed(p) {}
        test_info(test_info const & o) : value(o.value), passed(o.passed) {}
    };

    std::vector<test_info> test_results;
}

#define PREDEF_CHECK(X) test_results.push_back(test_info(#X,(X)))

void test_BOOST_VERSION_NUMBER()
{
    PREDEF_CHECK(BOOST_PREDEF_MAKE_0X_VRP(0xFFF) == BOOST_VERSION_NUMBER(0xF,0xF,0xF));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_0X_VVRP(0xFFFF) == BOOST_VERSION_NUMBER(0xFF,0xF,0xF));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_0X_VRPP(0xFFFF) == BOOST_VERSION_NUMBER(0xF,0xF,0xFF));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_0X_VVRR(0xFFFF) == BOOST_VERSION_NUMBER(0xFF,0xFF,0x0));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_0X_VRRPPPP(0xFFFFFFF) == BOOST_VERSION_NUMBER(0xF,0xFF,0xFFFF));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_0X_VVRRP(0xFFFFF) == BOOST_VERSION_NUMBER(0xFF,0xFF,0xF));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_0X_VRRPP000(0xFFFFF000) == BOOST_VERSION_NUMBER(0xF,0xFF,0xFF));

    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VRP(999) == BOOST_VERSION_NUMBER(9,9,9));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VPPP(9999) == BOOST_VERSION_NUMBER(9,0,999));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VVRRPP(999999) == BOOST_VERSION_NUMBER(99,99,99));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VVRR(9999) == BOOST_VERSION_NUMBER(99,99,0));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VRPP(9999) == BOOST_VERSION_NUMBER(9,9,99));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VRRPP(99999) == BOOST_VERSION_NUMBER(9,99,99));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VRP000(999000) == BOOST_VERSION_NUMBER(9,9,9));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VRR000(999000) == BOOST_VERSION_NUMBER(9,99,0));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VVRR00PP00(2199009900u) == BOOST_VERSION_NUMBER(21,99,99));
    /* PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VVRR00PP00(9999009900ull) == BOOST_VERSION_NUMBER(99,99,99)); */
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VVRR0PP00(999909900) == BOOST_VERSION_NUMBER(99,99,99));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VV00(9900) == BOOST_VERSION_NUMBER(99,00,00));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VVRR0PPPP(999909999) == BOOST_VERSION_NUMBER(99,99,9999));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_10_VRR(999) == BOOST_VERSION_NUMBER(9,99,00));

    PREDEF_CHECK(BOOST_PREDEF_MAKE_DATE(1971,1,1) == BOOST_VERSION_NUMBER(1,1,1));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_YYYYMMDD(19700101) == BOOST_VERSION_NUMBER(0,1,1));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_YYYYMMDD(19710101) == BOOST_VERSION_NUMBER(1,1,1));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_YYYYMMDD(20691231) == BOOST_VERSION_NUMBER(99,12,31));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_YYYYMM(197001) == BOOST_VERSION_NUMBER(0,1,1));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_YYYYMM(197101) == BOOST_VERSION_NUMBER(1,1,1));
    PREDEF_CHECK(BOOST_PREDEF_MAKE_YYYYMM(206912) == BOOST_VERSION_NUMBER(99,12,1));
}

int main()
{
    test_BOOST_VERSION_NUMBER();

    unsigned fail_count = 0;
    std::vector<test_info>::iterator i = test_results.begin();
    std::vector<test_info>::iterator e = test_results.end();
    for (; i != e; ++i)
    {
        std::cout
            << (i->passed ? "[passed]" : "[failed]")
            << " " << i->value
            << std::endl;
        fail_count += i->passed ? 0 : 1;
    }
    std::cout
        << std::endl
        << "TOTAL: "
        << "passed " << (test_results.size()-fail_count) << ", "
        << "failed " << (fail_count) << ", "
        << "of " << (test_results.size())
        << std::endl;
    return fail_count;
}
