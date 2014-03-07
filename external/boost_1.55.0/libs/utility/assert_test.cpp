//
//  assert_test.cpp - a test for boost/assert.hpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (2) Beman Dawes 2011
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/detail/lightweight_test.hpp>

#include <boost/assert.hpp>

void test_default()
{
    int x = 1;

    BOOST_ASSERT(1);
    BOOST_ASSERT(x);
    BOOST_ASSERT(x == 1);
    BOOST_ASSERT(&x);

    BOOST_ASSERT_MSG(1, "msg");
    BOOST_ASSERT_MSG(x, "msg");
    BOOST_ASSERT_MSG(x == 1, "msg");
    BOOST_ASSERT_MSG(&x, "msg");
}

#define BOOST_DISABLE_ASSERTS
#include <boost/assert.hpp>

void test_disabled()
{
    int x = 1;

    BOOST_ASSERT(1);
    BOOST_ASSERT(x);
    BOOST_ASSERT(x == 1);
    BOOST_ASSERT(&x);

    BOOST_ASSERT_MSG(1, "msg");
    BOOST_ASSERT_MSG(x, "msg");
    BOOST_ASSERT_MSG(x == 1, "msg");
    BOOST_ASSERT_MSG(&x, "msg");

    BOOST_ASSERT(0);
    BOOST_ASSERT(!x);
    BOOST_ASSERT(x == 0);

    BOOST_ASSERT_MSG(0, "msg");
    BOOST_ASSERT_MSG(!x, "msg");
    BOOST_ASSERT_MSG(x == 0, "msg");

    void * p = 0;

    BOOST_ASSERT(p);
    BOOST_ASSERT_MSG(p, "msg");

    // supress warnings
    p = &x;
    p = &p;
}

#undef BOOST_DISABLE_ASSERTS

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <cstdio>

int handler_invoked = 0;
int msg_handler_invoked = 0;

void boost::assertion_failed(char const * expr, char const * function, char const * file, long line)
{
#if !defined(BOOST_NO_STDC_NAMESPACE)
    using std::printf;
#endif

    printf("Expression: %s\nFunction: %s\nFile: %s\nLine: %ld\n\n", expr, function, file, line);
    ++handler_invoked;
}

void boost::assertion_failed_msg(char const * expr, char const * msg, char const * function,
  char const * file, long line)
{
#if !defined(BOOST_NO_STDC_NAMESPACE)
    using std::printf;
#endif

    printf("Expression: %s Message: %s\nFunction: %s\nFile: %s\nLine: %ld\n\n",
      expr, msg, function, file, line);
    ++msg_handler_invoked;
}

struct X
{
    static void f()
    {
        BOOST_ASSERT(0);
        BOOST_ASSERT_MSG(0, "msg f()");
    }
};

void test_handler()
{
    int x = 1;

    BOOST_ASSERT(1);
    BOOST_ASSERT(x);
    BOOST_ASSERT(x == 1);
    BOOST_ASSERT(&x);

    BOOST_ASSERT_MSG(1, "msg2");
    BOOST_ASSERT_MSG(x, "msg3");
    BOOST_ASSERT_MSG(x == 1, "msg4");
    BOOST_ASSERT_MSG(&x, "msg5");

    BOOST_ASSERT(0);
    BOOST_ASSERT(!x);
    BOOST_ASSERT(x == 0);

    BOOST_ASSERT_MSG(0,"msg 0");
    BOOST_ASSERT_MSG(!x, "msg !x");
    BOOST_ASSERT_MSG(x == 0, "msg x == 0");

    void * p = 0;

    BOOST_ASSERT(p);
    BOOST_ASSERT_MSG(p, "msg p");

    X::f();

    BOOST_ASSERT(handler_invoked == 5);
    BOOST_TEST(handler_invoked == 5);

    BOOST_ASSERT_MSG(msg_handler_invoked == 5, "msg_handler_invoked count is wrong");
    BOOST_TEST(msg_handler_invoked == 5);
}

#undef BOOST_ENABLE_ASSERT_HANDLER
#undef BOOST_ENABLE_ASSERT_MSG_HANDLER

int main()
{
    test_default();
    test_disabled();
    test_handler();

    return boost::report_errors();
}
