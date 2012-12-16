// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/context_switching.hpp"

#include <stdexcept>

#include "containers/scoped.hpp"
#include "thread_local.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

static void noop(void) {
}

TEST(ContextSwitchingTest, ContextRefSemantics) {
    context_ref_t nil_ref;
    EXPECT_TRUE(nil_ref.is_nil());
}

TEST(ContextSwitchingTest, CreateArtificialStack) {
    artificial_stack_t a(&noop, 1024*1024);
    EXPECT_FALSE(a.context.is_nil());
}

/* Thread-local variables for use in test functions, because we cannot pass a
`void*` to the test functions... */

TLS_with_init(context_ref_t *, original_context, NULL);
TLS_with_init(context_ref_t *, artificial_stack_1_context, NULL);
TLS_with_init(context_ref_t *, artificial_stack_2_context, NULL);
TLS_with_init(int, test_int, 0);

static void switch_context_test(void) {
    TLS_set_test_int(1 + TLS_get_test_int());
    context_switch(TLS_get_artificial_stack_1_context(), TLS_get_original_context());
    TLS_set_test_int(2 + TLS_get_test_int());
    context_switch(TLS_get_artificial_stack_1_context(), TLS_get_original_context());
    TLS_set_test_int(3 + TLS_get_test_int());
    context_switch(TLS_get_artificial_stack_1_context(), TLS_get_original_context());
    /* This will never get run */
    TLS_set_test_int(10000 + TLS_get_test_int());
}

TEST(ContextSwitchingTest, SwitchToContextRepeatedly) {
    scoped_ptr_t<context_ref_t> orig_context_local(new context_ref_t);
    TLS_set_original_context(orig_context_local.get());

    TLS_set_test_int(5);
    {
        artificial_stack_t a(&switch_context_test, 1024*1024);
        TLS_set_artificial_stack_1_context(&a.context);

        /* `context_switch` will cause `switch_context_test` to be run, which
        will increment `test_int` and then switch back from `src_context` to
        `dest_context`. */
        context_switch(TLS_get_original_context(), TLS_get_artificial_stack_1_context());
        context_switch(TLS_get_original_context(), TLS_get_artificial_stack_1_context());
        context_switch(TLS_get_original_context(), TLS_get_artificial_stack_1_context());

        EXPECT_FALSE(a.context.is_nil());
    }
    EXPECT_EQ(TLS_get_test_int(), 11);
    TLS_set_original_context(NULL);
}

static void first_switch(void) {
    TLS_set_test_int(1 + TLS_get_test_int());
    context_switch(TLS_get_artificial_stack_1_context(), TLS_get_artificial_stack_2_context());
}

static void second_switch(void) {
    TLS_set_test_int(1 + TLS_get_test_int());
    context_switch(TLS_get_artificial_stack_2_context(), TLS_get_original_context());
}

TEST(ContextSwitchingTest, SwitchBetweenContexts) {
    scoped_ptr_t<context_ref_t> orig_context_local(new context_ref_t);
    TLS_set_original_context(orig_context_local.get());
    TLS_set_test_int(99);
    {
        artificial_stack_t a1(&first_switch, 1024*1024);
        artificial_stack_t a2(&second_switch, 1024*1024);
        TLS_set_artificial_stack_1_context(&a1.context);
        TLS_set_artificial_stack_2_context(&a2.context);

        context_switch(TLS_get_original_context(), TLS_get_artificial_stack_1_context());
    }
    EXPECT_EQ(TLS_get_test_int(), 101);
    TLS_set_original_context(NULL);
}

__attribute__((noreturn)) static void throw_an_exception() {
    throw std::runtime_error("This is a test exception");
}

__attribute__((noreturn)) static void throw_exception_from_coroutine() {
    artificial_stack_t artificial_stack(&throw_an_exception, 1024*1024);
    context_ref_t _original_context;
    context_switch(&_original_context, &artificial_stack.context);
    unreachable();
}

TEST(ContextSwitchingTest, UncaughtException) {
    EXPECT_DEATH(throw_exception_from_coroutine(), "This is a test exception");
}

}   /* namespace unittest */
