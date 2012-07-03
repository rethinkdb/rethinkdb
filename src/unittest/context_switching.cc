#include "unittest/gtest.hpp"

#include "arch/runtime/context_switching.hpp"

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

static __thread context_ref_t
    *original_context = NULL,
    *artificial_stack_1_context = NULL,
    *artificial_stack_2_context = NULL;
static __thread int test_int;

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
    original_context = new context_ref_t;
    test_int = 5;
    {
        artificial_stack_t a(&switch_context_test, 1024*1024);
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
    delete original_context;
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
    original_context = new context_ref_t;
    test_int = 99;
    {
        artificial_stack_t a1(&first_switch, 1024*1024);
        artificial_stack_t a2(&second_switch, 1024*1024);
        artificial_stack_1_context = &a1.context;
        artificial_stack_2_context = &a2.context;

        context_switch(original_context, artificial_stack_1_context);
    }
    EXPECT_EQ(test_int, 101);
    delete original_context;
    original_context = NULL;
}

}   /* namespace unittest */
