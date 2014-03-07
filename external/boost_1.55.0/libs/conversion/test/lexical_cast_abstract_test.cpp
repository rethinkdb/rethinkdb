//  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Sergey Shandar 2005, Alexander Nasonov, 2007.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).
//
// Test abstract class. Bug 1358600:
// http://sf.net/tracker/?func=detail&aid=1358600&group_id=7586&atid=107586

#include <boost/config.hpp>

#if defined(__INTEL_COMPILER)
#pragma warning(disable: 193 383 488 981 1418 1419)
#elif defined(BOOST_MSVC)
#pragma warning(disable: 4097 4100 4121 4127 4146 4244 4245 4511 4512 4701 4800)
#endif

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost;

void test_abstract();

unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    unit_test::test_suite *suite =
        BOOST_TEST_SUITE("lexical_cast unit test");
    suite->add(BOOST_TEST_CASE(&test_abstract));

    return suite;
}

class A
{
public:
    virtual void out(std::ostream &) const = 0;
};

class B: public A
{
public:
    virtual void out(std::ostream &O) const { O << "B"; }
};

std::ostream &operator<<(std::ostream &O, const A &a)
{
    a.out(O);
    return O;
}

void test_abstract()
{
    const A &a = B();
    BOOST_CHECK(boost::lexical_cast<std::string>(a) == "B");
}

