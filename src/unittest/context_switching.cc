// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/context_switching.hpp"

#include <stdexcept>

#include "containers/scoped.hpp"
#include "unittest/gtest.hpp"
#include "arch/compiler.hpp"

namespace unittest {

static void noop(void) {
}

TEST(ContextSwitchingTest, ContextRefSemantics) {
    coro_context_ref_t nil_ref;
    EXPECT_TRUE(nil_ref.is_nil());
}

TEST(ContextSwitchingTest, CreateArtificialStack) {
	coro_initialize_for_thread();
    coro_stack_t a(&noop, 1024*1024);
    EXPECT_FALSE(a.context.is_nil());
}

/* Thread-local variables for use in test functions, because we cannot pass a
`void*` to the test functions... */

static DECL_THREAD_LOCAL coro_context_ref_t
    *original_context = NULL,
    *artificial_stack_1_context = NULL,
    *artificial_stack_2_context = NULL;
static DECL_THREAD_LOCAL int test_int;

static void switch_context_test(void) {
    test_int++;
    context_switch(artificial_stack_1_context, original_context);
    test_int += 2;
    context_switch(artificial_stack_1_context, original_context);
    test_int += 3;
    context_switch(artificial_stack_1_context, original_context);
    /* This will never get run */
    test_int += 10000;
}

TEST(ContextSwitchingTest, SwitchToContextRepeatedly) {
	coro_initialize_for_thread();
    scoped_ptr_t<coro_context_ref_t> orig_context_local(new coro_context_ref_t);
    original_context = orig_context_local.get();

    test_int = 5;
    {
        coro_stack_t a(&switch_context_test, 1024*1024);
        artificial_stack_1_context = &a.context;

        /* `context_switch` will cause `switch_context_test` to be run, which
        will increment `test_int` and then switch back from `src_context` to
        `dest_context`. */
        context_switch(original_context, artificial_stack_1_context);
        context_switch(original_context, artificial_stack_1_context);
        context_switch(original_context, artificial_stack_1_context);

        EXPECT_FALSE(a.context.is_nil());
    }
    EXPECT_EQ(test_int, 11);
    original_context = NULL;
}

static void first_switch(void) {
    test_int++;
    context_switch(artificial_stack_1_context, artificial_stack_2_context);
}

static void second_switch(void) {
    test_int++;
    context_switch(artificial_stack_2_context, original_context);
}

TEST(ContextSwitchingTest, SwitchBetweenContexts) {
	coro_initialize_for_thread();

    scoped_ptr_t<coro_context_ref_t> orig_context_local(new coro_context_ref_t);
    original_context = orig_context_local.get();
    test_int = 99;
    {
        coro_stack_t a1(&first_switch, 1024*1024);
        coro_stack_t a2(&second_switch, 1024*1024);
        artificial_stack_1_context = &a1.context;
        artificial_stack_2_context = &a2.context;

        context_switch(original_context, artificial_stack_1_context);
    }
    EXPECT_EQ(test_int, 101);
    original_context = NULL;
}

ATTR_NORETURN static void throw_an_exception() {
    throw std::runtime_error("This is a test exception");
}

ATTR_NORETURN static void throw_exception_from_coroutine() {
    coro_stack_t artificial_stack(&throw_an_exception, 1024*1024);
    coro_context_ref_t _original_context;
    context_switch(&_original_context, &artificial_stack.context);
    unreachable();
}

// ATN TODO: issue with gtest: throwing an exception from a death test doesn't seem to work
TEST(ContextSwitchingTest, DISABLED_UncaughtException) {
	coro_initialize_for_thread();
	EXPECT_DEATH(throw std::runtime_error("foo"), "foo");
    //EXPECT_DEATH(throw_exception_from_coroutine(), "This is a test exception");
}

}   /* namespace unittest */
