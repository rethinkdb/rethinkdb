/*
Copyright Redshift Software Inc 2011-2012
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/
#include <boost/predef/version_number.h>
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
    PREDEF_CHECK(BOOST_VERSION_NUMBER(0,0,1) == 1L);
    PREDEF_CHECK(BOOST_VERSION_NUMBER(99,99,99999) == 999999999L);
    PREDEF_CHECK(BOOST_VERSION_NUMBER(299,99,99999) != 2999999999L);
    PREDEF_CHECK(BOOST_VERSION_NUMBER(100,99,99999) != 1009999999L);
    PREDEF_CHECK(BOOST_VERSION_NUMBER(100,99,99999) == 9999999L);
    PREDEF_CHECK(BOOST_VERSION_NUMBER(100,100,100000) == 0L);
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
