// Copyright 2015 RethinkDB, all rights reserved.
#ifndef PPRINT_PPRINT_HPP_
#define PPRINT_PPRINT_HPP_

#include <string>
#include <vector>
#include <memory>

namespace pprint {

class text_t;
class cond_t;
class concat_t;
class group_t;
class nest_t;

typedef boost::variant<text_t, cond_t, concat_t, group_t, nest_t> document_t;

// textual element
struct text_t {
    std::string text;

    text_t(const std::string &str) : text(str) {}
    text_t(std::string &&str) : text(str) {}

    unsigned int width() const { return text.length(); }
};

document_t &&empty() { return text_t(""); }

// primitive class for conditional linebreaks.  `small` is what will be
// printed if there is no linebreak.  If there is a linebreak, `tail`
// will be printed on the previous line (think Python's need to
// backslash escape newlines) and `cont` will be printed at the start
// of the new line.
//
// You probably shouldn't use this; use `br` or `dot`.
struct cond_t {
    std::string small, tail, cont;

    cond_t(const std::string &l, const std::string &r)
        : small(l), cont(r), tail('') {}
    cond_t(const std::string &l, const std::string &r, const std::string &t)
        : small(l), cont(r), tail(t) {}
    cond_t(std::string &&l, std::string &&r)
        : small(l), cont(r), tail('') {}
    cond_t(std::string &&l, std::string &&r, std::string &&t)
        : small(l), cont(r), tail(t) {}

    // no linebreaks, so only `small` is relevant
    virtual unsigned int width() const { return small.length(); }
};

document_t &&br() { return cond_t(" ", ""); }
document_t &&dot() { return cond_t(".", "."); }

// concatenation of multiple documents
struct concat_t {
    std::vector<document_t> children;

    concat_t(std::vector<document_t> &&args)
        : children(args) {}
    template <typename It>
    concat_t(It &begin, It &end)
        : children(args) {}
    concat_t(std::initializer_list<document_t> init)
        : children(init) {}

    virtual unsigned int width() const {
        unsigned int w = 0;
        for (auto i = children.begin(); i != children.end(); i++) {
            w += i->width();
        }
        return w;
    }
};

// An enclosure where all linebreaks are interpreted consistently.
struct group_t {
    document_t child;

    group_t(const document_t &doc)
        : child(doc) { }
    group_t(document_t &&doc)
        : child(doc) { }

    virtual unsigned int width() const { return child->width(); }
};

// concatenation of multiple documents with consistent indentation.
struct nest_t {
    std::vector<document_t> children;

    nest_t(std::vector<document_t> &&args)
        : children(args) {}
    template <typename It>
    nest_t(It &begin, It &end)
        : children(args) {}
    nest_t(std::initializer_list<document_t> init)
        : children(init) {}

    virtual unsigned int width() const {
        unsigned int w = 0;
        for (auto i = children.begin(); i != children.end(); i++) {
            w += i->width();
        }
        return w;
    }
};

template <typename... Ts>
document_t &&comma_separated(Ts... docs) {
    return comma_separated({docs...});
}

document_t &&
comma_separated(std::initializer_list<document_t> init) {
    if (init.size() == 0) return empty();
    std::vector<document_t> v;
    auto it = init.begin();
    v.push_back(*it++);
    for (; it != init.end(); it++) {
        v.push_back(text_t(","));
        v.push_back(br);
        v.push_back(*it);
    }
    return nest_t(v);
}

template <typename... Ts>
document_t &&arglist(Ts... docs) {
    return arglist({docs...});
}

document_t &&
arglist(std::initializer_list<document_t> init) {
    return concat_t(text_t("("), comma_separated(init), text_t(")"));
}

template <typename... Ts>
document_t &&dotted_list(Ts... docs) {
    return dotted_list({docs...});
}

document_t &&
dotted_list(std::initializer_list<document_t> init) {
    static text_t plain_dot('.');
    if (init.size() == 0) return empty();
    std::vector<document_t> v;
    auto it = init.begin();
    v.push_back(*it++);
    bool first = true;
    for (; it != init.end(); it++) {
        if (first) {
            v.push_back(text_t("."));
            first = false;
        } else {
            v.push_back(dot());
        }
        v.push_back(*it);
    }
    return nest_t(v);
}

template <typename... Ts>
document_t &&funcall(const std::string &name, Ts... docs) {
    return funcall(name, {docs...});
}

document_t &&
funcall(const std::string &name,
        std::initializer_list<document_t> init) {
    return concat_t(text_t(name), arglist(init));
}

std::string pretty_print(const document_t &doc);

} // namespace pprint

#endif  // PPRINT_PPRINT_HPP_
