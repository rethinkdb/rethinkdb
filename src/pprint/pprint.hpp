// Copyright 2015 RethinkDB, all rights reserved.
#ifndef PPRINT_PPRINT_HPP_
#define PPRINT_PPRINT_HPP_

#include <string>
#include <vector>
#include <memory>

namespace pprint {

struct text_t;
struct cond_t;
struct concat_t;
struct group_t;
struct nest_t;

class document_visitor_t {
public:
    virtual ~document_visitor_t() {}

    virtual void operator()(const text_t &) const = 0;
    virtual void operator()(const cond_t &) const = 0;
    virtual void operator()(const concat_t &) const = 0;
    virtual void operator()(const group_t &) const = 0;
    virtual void operator()(const nest_t &) const = 0;
};

class document_t {
public:
    virtual ~document_t() {}
    virtual unsigned int width() const = 0;
    virtual void visit(const document_visitor_t &v) const = 0;
};

typedef std::shared_ptr<const document_t> doc_handle_t;

// textual element
struct text_t : public document_t {
    std::string text;

    text_t(const std::string &str) : text(str) {}
    text_t(std::string &&str) : text(str) {}
    virtual ~text_t() {}

    virtual unsigned int width() const { return text.length(); }
    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

extern const doc_handle_t empty;

// primitive class for conditional linebreaks.  `small` is what will be
// printed if there is no linebreak.  If there is a linebreak, `tail`
// will be printed on the previous line (think Python's need to
// backslash escape newlines) and `cont` will be printed at the start
// of the new line.
//
// You probably shouldn't use this; use `br` or `dot`.
struct cond_t : public document_t {
    std::string small, cont, tail;

    cond_t(const std::string &l, const std::string &r)
        : small(l), cont(r), tail("") {}
    cond_t(const std::string &l, const std::string &r, const std::string &t)
        : small(l), cont(r), tail(t) {}
    cond_t(std::string &&l, std::string &&r) : small(l), cont(r), tail("") {}
    cond_t(std::string &&l, std::string &&r, std::string &&t)
        : small(l), cont(r), tail(t) {}
    virtual ~cond_t() {}

    // no linebreaks, so only `small` is relevant
    virtual unsigned int width() const { return small.length(); }

    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

extern const doc_handle_t br;
extern const doc_handle_t dot;

// concatenation of multiple documents
struct concat_t : public document_t {
    std::vector<doc_handle_t> children;

    concat_t(std::vector<doc_handle_t> &&args)
        : children(args) {}
    template <typename It>
    concat_t(It &begin, It &end)
        : children(begin, end) {}
    concat_t(std::initializer_list<doc_handle_t> init)
        : children(init) {}
    virtual ~concat_t() {}

    unsigned int width() const {
        unsigned int w = 0;
        for (auto i = children.begin(); i != children.end(); i++) {
            w += (*i)->width();
        }
        return w;
    }

    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

// An enclosure where all linebreaks are interpreted consistently.
struct group_t : public document_t {
    doc_handle_t child;

    group_t(doc_handle_t doc) : child(doc) {}
    virtual ~group_t() {}

    unsigned int width() const {
        return child->width();
    }

    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

// An enclosure where linebreaks have consistent indentation.
struct nest_t : public document_t {
    doc_handle_t child;

    nest_t(doc_handle_t doc) : child(doc) {}
    virtual ~nest_t() {}

    unsigned int width() const {
        return child->width();
    }

    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

template <typename... Ts>
doc_handle_t comma_separated(Ts... docs) {
    return comma_separated({docs...});
}

doc_handle_t comma_separated(std::initializer_list<doc_handle_t> init) {
    if (init.size() == 0) return empty;
    std::vector<doc_handle_t> v;
    auto it = init.begin();
    v.push_back(*it++);
    for (; it != init.end(); it++) {
        v.push_back(std::make_shared<text_t>(","));
        v.push_back(br);
        v.push_back(*it);
    }
    return std::make_shared<nest_t>(std::make_shared<concat_t>(std::move(v)));
}

template <typename... Ts>
doc_handle_t arglist(Ts... docs) {
    return arglist({docs...});
}

doc_handle_t arglist(std::initializer_list<doc_handle_t> init) {
    static const doc_handle_t lparen = std::make_shared<text_t>("(");
    static const doc_handle_t rparen = std::make_shared<text_t>(")");
    std::vector<doc_handle_t> v { lparen, comma_separated(init), rparen };
    return std::make_shared<concat_t>(std::move(v));
}

template <typename... Ts>
doc_handle_t dotted_list(Ts... docs) {
    return dotted_list({docs...});
}

doc_handle_t
dotted_list(std::initializer_list<doc_handle_t> init) {
    static const doc_handle_t plain_dot = std::make_shared<text_t>(".");
    if (init.size() == 0) return empty;
    std::vector<doc_handle_t> v;
    auto it = init.begin();
    v.push_back(*it++);
    bool first = true;
    for (; it != init.end(); it++) {
        if (first) {
            v.push_back(plain_dot);
            first = false;
        } else {
            v.push_back(dot);
        }
        v.push_back(*it);
    }
    return std::make_shared<nest_t>(std::make_shared<concat_t>(std::move(v)));
}

template <typename... Ts>
doc_handle_t funcall(const std::string &name, Ts... docs) {
    return funcall(name, {docs...});
}

doc_handle_t funcall(const std::string &name,
                     std::initializer_list<doc_handle_t> init) {
    std::vector<doc_handle_t> v { std::make_shared<text_t>(name), arglist(init) };
    return std::make_shared<concat_t>(std::move(v));
}

std::string pretty_print(doc_handle_t doc);

} // namespace pprint

#endif  // PPRINT_PPRINT_HPP_
