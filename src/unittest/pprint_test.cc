// Copyright 2015 RethinkDB, all rights reserved.

#include <functional>

#include "pprint/pprint.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

using pprint::pprint_streamer;

std::string pretty_print(size_t width, pprint_streamer &&stream) {
    return pretty_print(width, std::move(stream).elems());
}

// We never do a group without a nest in js pretty-printing, just in unit tests.
class grouped {
    pprint_streamer *pp_;
    size_t gbeg_index_;
    DISABLE_COPYING(grouped);
public:
    explicit grouped(pprint_streamer *pp) : pp_(pp) {
        gbeg_index_ = pp->add_gbeg();
    }
    ~grouped() {
        pp_->update_gbeg(gbeg_index_);
        pp_->add_gend();
    }
};

// We have this convoluted std::function thing to adapt old pretty-print unit tests that
// used a document_t tree object.

// Pretty printer document.
struct document_t {
    std::function<void(pprint_streamer *pp)> func;
};

std::string pretty_print(size_t width, document_t doc) {
    pprint_streamer stream;
    doc.func(&stream);
    return pretty_print(width, std::move(stream));
}

document_t make_concat(const document_t *begin,
                       const document_t *end) {
    std::vector<document_t> docs(begin, end);
    return document_t{[docs](pprint_streamer *pp) {
        for (auto &doc : docs) {
            doc.func(pp);
        }
    }};
}

document_t make_concat(std::vector<document_t> args) {
    return make_concat(args.data(), args.data() + args.size());
}
document_t make_concat(std::initializer_list<document_t> args) {
    return make_concat(args.begin(), args.end());
}

document_t make_text(std::string&& text) {
    return document_t{[text](pprint_streamer *pp) {
        pp->add_text(std::string(text));
    }};
}

document_t make_cond(std::string &&l, std::string &&r, std::string &&t) {
    pprint::cond_elem_spec elem{std::move(l), std::move(r), std::move(t)};
    return document_t{[elem](pprint_streamer *pp) {
        pp->add(elem);
    }};
}

document_t make_group(document_t child) {
    return document_t{[child](pprint_streamer *pp) {
        grouped group(pp);
        child.func(pp);
    }};
}

document_t make_nest(document_t child) {
    return document_t{[child](pprint_streamer *pp) {
        pprint::nested nest(pp);
        child.func(pp);
    }};
}

const document_t empty = make_text("");
const document_t cond_linebreak = make_cond(" ", "", "");
const document_t dot_linebreak = make_cond(".", ".", "");
const document_t uncond_linebreak{[](pprint_streamer *pp) { pp->add_crlf(); }};

// Documents separated by commas and then a `br`.
//
// Think `1, 2, 3`.
document_t
comma_separated(std::initializer_list<document_t> init);

template <typename... Ts>
inline document_t comma_separated(Ts &&... docs) {
    return comma_separated({std::forward<Ts>(docs)...});
}

// Argument list; comma separated arguments wrapped in parens with a nest.
//
// Think `(1, 2, 3)`.
document_t
arglist(std::initializer_list<document_t>);

template <typename... Ts>
inline document_t arglist(Ts &&... docs) {
    return arglist({std::forward<Ts>(docs)...});
}

// Documents separated by `dot`.
//
// Think `r.foo().bar().baz()`.
document_t
dotted_list(std::initializer_list<document_t>);

template <typename... Ts>
inline document_t dotted_list(Ts &&... docs) {
    return dotted_list({std::forward<Ts>(docs)...});
}

// Function call document, where `name` is the call and `init` are the args.
//
// Think `foo(1, 2, 3)`.
document_t
funcall(std::string &&, std::initializer_list<document_t>);

template <typename... Ts>
inline document_t funcall(std::string &&name, Ts &&... docs) {
    return funcall(std::move(name), {std::forward<Ts>(docs)...});
}

// Helper for r.foo.bar.baz expressions.
document_t r_dot(std::initializer_list<document_t>);

template <typename... Ts>
inline document_t r_dot(Ts &&... docs) {
    return r_dot({std::forward<Ts>(docs)...});
}


document_t
comma_separated(std::initializer_list<document_t> init) {
    if (init.size() == 0) return empty;
    std::vector<document_t> v;
    auto it = init.begin();
    v.push_back(*it++);
    for (; it != init.end(); it++) {
        v.push_back(make_text(","));
        v.push_back(cond_linebreak);
        v.push_back(*it);
    }
    return make_nest(make_concat(std::move(v)));
}

document_t
arglist(std::initializer_list<document_t> init) {
    static const document_t lparen = make_text("(");
    static const document_t rparen = make_text(")");
    return make_concat({ lparen, comma_separated(init), rparen });
}

template <typename It>
document_t dotted_list_int(It begin, It end) {
    static const document_t plain_dot = make_text(".");
    if (begin == end) return empty;
    std::vector<document_t> v;
    It it = begin;
    v.push_back(*it++);
    if (it == end) return make_nest(v[0]);
    bool first = true;
    for (; it != end; it++) {
        // a bit involved here, because we don't want to break on the
        // first dot (looks ugly)
        if (first) {
            v.push_back(plain_dot);
            first = false;
        } else {
            v.push_back(dot_linebreak);
        }
        v.push_back(*it);
    }
    // the idea here is that dotted(a, b, c) turns into concat(a,
    // nest(concat(".", b, dot, c))) which means that the dots line up
    // on linebreaks nicely.
    return make_concat({v[0], make_nest(make_concat(v.data()+1, v.data() + v.size()))});
}

document_t
dotted_list(std::initializer_list<document_t> init) {
    return dotted_list_int(init.begin(), init.end());
}

document_t funcall(std::string&& name,
                     std::initializer_list<document_t> init) {
    return make_concat({make_text(std::move(name)), arglist(init)});
}

document_t
r_dot(std::initializer_list<document_t> args) {
    static const document_t r = make_text("r");
    std::vector<document_t> v;
    v.push_back(r);
    v.insert(v.end(), args.begin(), args.end());
    return dotted_list_int(v.begin(), v.end());
}

TEST(PPrintTest, SimpleText) {
    pprint_streamer stream;
    stream.add_text("some text");

    pprint_streamer copy = stream;
    ASSERT_EQ("some text", pretty_print(10, std::move(stream)));
    ASSERT_EQ("some text", pretty_print(4, std::move(copy)));
}

TEST(PPrintTest, SimpleCond) {
    pprint_streamer stream;
    stream.add_text("some text");
    cond_linebreak.func(&stream);
    stream.add_text("some more text");

    pprint_streamer copy = stream;

    // Oddity of Kiselyov's algorithm is it always breaks if not in a group.
    ASSERT_EQ("some text\nsome more text", pretty_print(80, std::move(stream)));
    ASSERT_EQ("some text\nsome more text", pretty_print(4, std::move(copy)));
}

TEST(PPrintTest, GroupedCond) {
    pprint_streamer stream;
    {
        grouped group(&stream);
        stream.add_text("some text");
        cond_linebreak.func(&stream);
        stream.add_text("some more text");
    }
    pprint_streamer copy = stream;

    ASSERT_EQ("some text some more text", pretty_print(80, std::move(stream)));
    ASSERT_EQ("some text\nsome more text", pretty_print(4, std::move(copy)));
}

TEST(PPrintTest, GroupedCondWithTail) {
    pprint_streamer stream;
    {
        grouped group(&stream);
        stream.add_text("some text");
        stream.add(pprint::cond_elem_spec{" ", "", " \\"});
        stream.add_text("some more text");
    }
    pprint_streamer copy = stream;

    ASSERT_EQ("some text some more text", pretty_print(80, std::move(stream)));
    ASSERT_EQ("some text \\\nsome more text", pretty_print(4, std::move(copy)));
}

TEST(PPrintTest, Nest) {
    document_t handle = r_dot(funcall("foo"),
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
    document_t handle
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
