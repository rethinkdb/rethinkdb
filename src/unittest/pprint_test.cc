// Copyright 2015 RethinkDB, all rights reserved.

#include "pprint/pprint.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

using namespace pprint; // NOLINT(build/namespaces)

TEST(PPrintTest, SimpleText) {
    counted_t<const document_t> handle = make_text("some text");

    ASSERT_EQ("some text", pretty_print(10, handle));
    ASSERT_EQ("some text", pretty_print(4, handle));
}

TEST(PPrintTest, SimpleCond) {
    counted_t<const document_t> handle = make_concat(make_text("some text"),
                                                     cond_linebreak,
                                                     make_text("some more text"));

    // Oddity of Kiselyov's algorithm is it always breaks if not in a group.
    ASSERT_EQ("some text\nsome more text", pretty_print(80, handle));
    ASSERT_EQ("some text\nsome more text", pretty_print(4, handle));
}

TEST(PPrintTest, GroupedCond) {
    counted_t<const document_t> handle
        = make_group(make_concat(make_text("some text"), cond_linebreak,
                                 make_text("some more text")));

    ASSERT_EQ("some text some more text", pretty_print(80, handle));
    ASSERT_EQ("some text\nsome more text", pretty_print(4, handle));
}

TEST(PPrintTest, GroupedCondWithTail) {
    counted_t<const document_t> handle
        = make_group(make_concat(make_text("some text"), make_cond(" ", "", " \\"),
                                 make_text("some more text")));

    ASSERT_EQ("some text some more text", pretty_print(80, handle));
    ASSERT_EQ("some text \\\nsome more text", pretty_print(4, handle));
}

TEST(PPrintTest, Nest) {
    counted_t<const document_t> handle = r_dot(funcall("foo"),
                                funcall("foo"),
                                funcall("foo"),
                                funcall("foo"),
                                funcall("foo"));

    ASSERT_EQ("r.foo().foo().foo().foo().foo()", pretty_print(80, handle));
    // even though part of this can fit, consistent CRs mean they all break.
    ASSERT_EQ("r.foo()\n .foo()\n .foo()\n .foo()\n .foo()",
              pretty_print(20, handle));
    ASSERT_EQ("r.foo()\n .foo()\n .foo()\n .foo()\n .foo()",
              pretty_print(4, handle));
}

TEST(PPrintTest, Involved) {
    counted_t<const document_t> handle
        = r_dot(funcall("expr", make_text("5")),
                funcall("add", r_dot(funcall("expr", make_text("7")),
                                     funcall("frob"))),
                funcall("mul", r_dot(funcall("expr", make_text("17"))),
                        funcall("mul", r_dot(funcall("expr", make_text("17")))),
                        funcall("mul", r_dot(funcall("expr", make_text("17"))))));

    ASSERT_EQ("r.expr(5).add(r.expr(7).frob()).mul(r.expr(17), "
              "mul(r.expr(17)), mul(r.expr(17)))", pretty_print(180, handle));
    // because the subexpressions can fit, they don't get broken even
    // though the whole thing is too big.
    ASSERT_EQ("r.expr(5)\n .add(r.expr(7).frob())\n .mul(r.expr(17), "
              "mul(r.expr(17)), mul(r.expr(17)))", pretty_print(80, handle));
    ASSERT_EQ("r.expr(5)\n .add(r.expr(7)\n       .frob())\n .mul(r.expr(17),"
              "\n      mul(r.expr(17)),\n      mul(r.expr(17)))",
              pretty_print(20, handle));
    // never break on first dot, even if it is too tight otherwise,
    // and breaking on the parens in a funcall also is ugly.
    ASSERT_EQ("r.expr(5)\n .add(r.expr(7)\n       .frob())\n .mul(r.expr(17),"
              "\n      mul(r.expr(17)),\n      mul(r.expr(17)))",
              pretty_print(4, handle));
}

}  // namespace unittest
