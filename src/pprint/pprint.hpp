// Copyright 2015 RethinkDB, all rights reserved.
#ifndef PPRINT_PPRINT_HPP_
#define PPRINT_PPRINT_HPP_

#include <string>
#include <vector>
#include <utility>

#include "containers/counted.hpp"

namespace pprint {

class document_visitor_t;
// Pretty printer document.  Has to be `slow_atomic_countable_t`
// because of reuse of things like `br` and `dot`.
class document_t : public slow_atomic_countable_t<document_t> {
public:
    virtual ~document_t() {}
    virtual size_t width() const = 0;
    virtual void visit(const document_visitor_t &v) const = 0;
    virtual std::string str() const = 0;
};

// Primitive textual element.
counted_t<const document_t> make_text(std::string text);

// Primitive for conditional linebreaks.
//
// `small` is what will be printed if there is no linebreak.  If there
// is a linebreak, `tail` will be printed on the previous line (think
// Python's need to backslash escape newlines) and `cont` will be
// printed at the start of the new line.
//
// You probably shouldn't use this; use `br` or `dot` when they work.
counted_t<const document_t> make_cond(const std::string small, const std::string cont,
                       const std::string tail);

// A concatenation of documents.
counted_t<const document_t> make_concat(std::vector<counted_t<const document_t> > args);
counted_t<const document_t>
make_concat(std::initializer_list<counted_t<const document_t> > args);

template <typename It>
counted_t<const document_t> make_concat(It &&begin, It &&end) {
    std::vector<counted_t<const document_t> > v(std::forward<It>(begin),
                                                std::forward<It>(end));
    return make_concat(std::move(v));
}

// A document enclosure where all linebreaks are interpreted consistently.
counted_t<const document_t> make_group(counted_t<const document_t> doc);

// A document enclosure where all linebreaks are indented to the start of the nest.
//
// This implicitly wraps `doc` in a group.
counted_t<const document_t> make_nest(counted_t<const document_t> doc);

extern const counted_t<const document_t> empty;
extern const counted_t<const document_t> cond_linebreak;
// unconditional line break
extern const counted_t<const document_t> uncond_linebreak;
extern const counted_t<const document_t> dot_linebreak;


// Documents separated by commas and then a `br`.
//
// Think `1, 2, 3`.
counted_t<const document_t>
comma_separated(std::initializer_list<counted_t<const document_t> > init);

template <typename... Ts>
inline counted_t<const document_t> comma_separated(Ts &&... docs) {
    return comma_separated({std::forward<Ts>(docs)...});
}

// Argument list; comma separated arguments wrapped in parens with a nest.
//
// Think `(1, 2, 3)`.
counted_t<const document_t>
arglist(std::initializer_list<counted_t<const document_t> >);

template <typename... Ts>
inline counted_t<const document_t> arglist(Ts &&... docs) {
    return arglist({std::forward<Ts>(docs)...});
}

// Documents separated by `dot`.
//
// Think `r.foo().bar().baz()`.
counted_t<const document_t>
dotted_list(std::initializer_list<counted_t<const document_t> >);

template <typename... Ts>
inline counted_t<const document_t> dotted_list(Ts &&... docs) {
    return dotted_list({std::forward<Ts>(docs)...});
}

// Function call document, where `name` is the call and `init` are the args.
//
// Think `foo(1, 2, 3)`.
counted_t<const document_t>
funcall(const std::string &, std::initializer_list<counted_t<const document_t> >);

template <typename... Ts>
inline counted_t<const document_t> funcall(const std::string &name, Ts &&... docs) {
    return funcall(name, {std::forward<Ts>(docs)...});
}

// Helper for r.foo.bar.baz expressions.
counted_t<const document_t> r_dot(std::initializer_list<counted_t<const document_t> >);

template <typename... Ts>
inline counted_t<const document_t> r_dot(Ts &&... docs) {
    return r_dot({std::forward<Ts>(docs)...});
}

// Render document at the given width.
std::string pretty_print(size_t width, counted_t<const document_t> doc);

} // namespace pprint

#endif  // PPRINT_PPRINT_HPP_
