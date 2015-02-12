// Copyright 2015 RethinkDB, all rights reserved.
#ifndef PPRINT_PPRINT_HPP_
#define PPRINT_PPRINT_HPP_

#include <string>
#include <vector>
#include <memory>

namespace pprint {

class document_visitor_t;
class document_t {
public:
    virtual ~document_t() {}
    virtual unsigned int width() const = 0;
    virtual void visit(const document_visitor_t &v) const = 0;
    virtual std::string str() const = 0;
};

typedef std::shared_ptr<const document_t> doc_handle_t;

// textual element
doc_handle_t make_text(std::string text);

// primitive for conditional linebreaks.
//
// `small` is what will be printed if there is no linebreak.  If there
// is a linebreak, `tail` will be printed on the previous line (think
// Python's need to backslash escape newlines) and `cont` will be
// printed at the start of the new line.
//
// You probably shouldn't use this; use `br` or `dot` when they work.
doc_handle_t make_cond(const std::string small, const std::string cont,
                       const std::string tail);

// document composition
doc_handle_t make_concat(std::vector<doc_handle_t> args);
doc_handle_t make_concat(std::initializer_list<doc_handle_t> args);

template <typename... Ts>
inline doc_handle_t make_concat(Ts... docs) {
    return make_concat({docs...});
}

// document enclosure where all linebreaks are interpreted consistently.
doc_handle_t make_group(doc_handle_t doc);

// document enclosure where all linebreaks are indented to the start of the nest.
//
// implicitly wraps `doc` in a group.
doc_handle_t make_nest(doc_handle_t doc);

extern const doc_handle_t empty;
extern const doc_handle_t br;
extern const doc_handle_t dot;


// documents separated by commas and then a `br`.
//
// think `1, 2, 3`
doc_handle_t comma_separated(std::initializer_list<doc_handle_t> init);

template <typename... Ts>
inline doc_handle_t comma_separated(Ts... docs) {
    return comma_separated({docs...});
}

// argument list; comma separated arguments wrapped in parens with a nest.
//
// think `(1, 2, 3)`
doc_handle_t arglist(std::initializer_list<doc_handle_t> init);

template <typename... Ts>
inline doc_handle_t arglist(Ts... docs) {
    return arglist({docs...});
}

// documents separated by `dot`.
//
// think `r.foo().bar().baz()`
doc_handle_t dotted_list(std::initializer_list<doc_handle_t> init);

template <typename... Ts>
inline doc_handle_t dotted_list(Ts... docs) {
    return dotted_list({docs...});
}

// function call document, where `name` is the call and `init` are the args.
//
// think `foo(1, 2, 3)`
doc_handle_t funcall(const std::string &name, std::initializer_list<doc_handle_t> init);

template <typename... Ts>
inline doc_handle_t funcall(const std::string &name, Ts... docs) {
    return funcall(name, {docs...});
}

// helper for r.foo.bar.baz expressions.
doc_handle_t r_dot(std::initializer_list<doc_handle_t> args);

template <typename... Ts>
inline doc_handle_t r_dot(Ts... docs) {
    return r_dot({docs...});
}

// render document at the given width.
std::string pretty_print(unsigned int width, doc_handle_t doc);

} // namespace pprint

#endif  // PPRINT_PPRINT_HPP_
