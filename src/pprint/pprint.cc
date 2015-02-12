#include "pprint/pprint.hpp"

#include <vector>
#include <list>
#include <functional>
#include <memory>

#include "errors.hpp"
#include <boost/optional.hpp>

namespace pprint {

class text_t;
class cond_t;
class concat_t;
class group_t;
class nest_t;

class document_visitor_t {
public:
    virtual ~document_visitor_t() {}

    virtual void operator()(const text_t &) const = 0;
    virtual void operator()(const cond_t &) const = 0;
    virtual void operator()(const concat_t &) const = 0;
    virtual void operator()(const group_t &) const = 0;
    virtual void operator()(const nest_t &) const = 0;
};

class text_t : public document_t {
public:
    std::string text;

    text_t(const std::string &str) : text(str) {}
    text_t(std::string &&str) : text(str) {}
    virtual ~text_t() {}

    virtual unsigned int width() const { return text.length(); }
    virtual void visit(const document_visitor_t &v) const { v(*this); }
    virtual std::string str() const { return "Text(\"" + text + "\")"; }
};

doc_handle_t make_text(const std::string text) {
    return std::make_shared<text_t>(std::move(text));
}

class cond_t : public document_t {
public:
    std::string small, cont, tail;

    cond_t(const std::string l, const std::string r, const std::string t="")
        : small(std::move(l)), cont(std::move(r)), tail(std::move(t)) {}
    virtual ~cond_t() {}

    // no linebreaks, so only `small` is relevant
    virtual unsigned int width() const { return small.length(); }
    virtual std::string str() const {
        return "Cond(\"" + small + "\",\"" + cont + "\",\"" + tail + "\")";
    }

    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

doc_handle_t make_cond(const std::string l, const std::string r,
                       const std::string t) {
    return std::make_shared<cond_t>(std::move(l), std::move(r), std::move(t));
}

// concatenation of multiple documents
class concat_t : public document_t {
public:
    std::vector<doc_handle_t> children;

    concat_t(std::vector<doc_handle_t> args)
        : children(std::move(args)) {}
    template <typename It>
    concat_t(const It &begin, const It &end)
        : children(begin, end) {}
    concat_t(std::initializer_list<doc_handle_t> init)
        : children(std::move(init)) {}
    virtual ~concat_t() {}

    virtual unsigned int width() const {
        unsigned int w = 0;
        for (const auto &child : children) {
            w += child->width();
        }
        return w;
    }
    virtual std::string str() const {
        std::string result = "";
        for (const auto &child : children) {
            result += child->str();
        }
        return result;
    }

    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

doc_handle_t make_concat(std::vector<doc_handle_t> args) {
    return std::make_shared<concat_t>(std::move(args));
}
doc_handle_t make_concat(std::initializer_list<doc_handle_t> args) {
    return std::make_shared<concat_t>(std::move(args));
}
template <typename It>
doc_handle_t make_concat(const It &begin, const It &end) {
    return std::make_shared<concat_t>(begin, end);
}

class group_t : public document_t {
public:
    doc_handle_t child;

    group_t(doc_handle_t doc) : child(doc) {}
    virtual ~group_t() {}

    virtual unsigned int width() const {
        return child->width();
    }
    virtual std::string str() const {
        return "Group(" + child->str() + ")";
    }

    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

doc_handle_t make_group(doc_handle_t child) {
    return std::make_shared<group_t>(child);
}

class nest_t : public document_t {
public:
    doc_handle_t child;

    nest_t(doc_handle_t doc) : child(doc) {}
    virtual ~nest_t() {}

    virtual unsigned int width() const {
        return child->width();
    }
    virtual std::string str() const {
        return "Nest(" + child->str() + ")";
    }

    virtual void visit(const document_visitor_t &v) const { v(*this); }
};

doc_handle_t make_nest(doc_handle_t child) {
    return std::make_shared<nest_t>(child);
}

const doc_handle_t empty = std::make_shared<text_t>("");
const doc_handle_t br = std::make_shared<cond_t>(" ", "");
const doc_handle_t dot = std::make_shared<cond_t>(".", ".");

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
    return make_nest(make_concat(std::move(v)));
}

doc_handle_t arglist(std::initializer_list<doc_handle_t> init) {
    static const doc_handle_t lparen = std::make_shared<text_t>("(");
    static const doc_handle_t rparen = std::make_shared<text_t>(")");
    return make_concat({ lparen, comma_separated(init), rparen });
}

template <typename Container>
doc_handle_t dotted_list_int(Container init) {
    static const doc_handle_t plain_dot = std::make_shared<text_t>(".");
    if (init.size() == 0) return empty;
    if (init.size() == 1) return make_nest(*(init.begin()));
    std::vector<doc_handle_t> v;
    auto it = init.begin();
    v.push_back(*it++);
    bool first = true;
    for (; it != init.end(); it++) {
        // a bit involved here, because we don't want to break on the
        // first dot (looks ugly)
        if (first) {
            v.push_back(plain_dot);
            first = false;
        } else {
            v.push_back(dot);
        }
        v.push_back(*it);
    }
    // the idea here is that dotted(a, b, c) turns into concat(a,
    // nest(concat(".", b, dot, c))) which means that the dots line up
    // on linebreaks nicely.
    return make_concat({v[0], make_nest(make_concat(v.begin()+1, v.end()))});
}

doc_handle_t dotted_list(std::initializer_list<doc_handle_t> init) {
    return dotted_list_int(init);
}

doc_handle_t funcall(const std::string &name,
                     std::initializer_list<doc_handle_t> init) {
    return make_concat({make_text(name), arglist(init)});
}

doc_handle_t r_dot(std::initializer_list<doc_handle_t> args) {
    static const doc_handle_t r = std::make_shared<text_t>("r");
    std::vector<doc_handle_t> v;
    v.push_back(r);
    v.insert(v.end(), args.begin(), args.end());
    return dotted_list_int(v);
}

class text_element_t;
class cond_element_t;
class nbeg_element_t;
class nend_element_t;
class gbeg_element_t;
class gend_element_t;

class stream_element_visitor_t {
public:
    virtual ~stream_element_visitor_t() {}

    virtual void operator()(text_element_t &) = 0;
    virtual void operator()(cond_element_t &) = 0;
    virtual void operator()(nbeg_element_t &) = 0;
    virtual void operator()(nend_element_t &) = 0;
    virtual void operator()(gbeg_element_t &) = 0;
    virtual void operator()(gend_element_t &) = 0;
};

// Streaming version of the document tree, suitable for printing after
// much processing
class stream_element_t : public std::enable_shared_from_this<stream_element_t> {
public:
    boost::optional<size_t> hpos;

    stream_element_t() : hpos() {}
    stream_element_t(size_t n) : hpos(n) {}
    virtual ~stream_element_t() {}

    virtual void visit(stream_element_visitor_t &v) = 0;
    virtual std::string str() const = 0;
    std::string pos_or_not() const {
        return hpos ? std::to_string(*hpos) : "-1";
    }
};

typedef std::shared_ptr<stream_element_t> stream_handle_t;

class text_element_t : public stream_element_t {
public:
    std::string payload;

    text_element_t(const std::string &text, size_t hpos)
        : stream_element_t(hpos), payload(text) {}
    text_element_t(const std::string &text)
        : stream_element_t(), payload(text) {}
    virtual ~text_element_t() {}
    virtual std::string str() const {
        return "TE(\"" + payload + "\"," + pos_or_not() + ")";
    }

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
};

class cond_element_t : public stream_element_t {
public:
    std::string small, tail, cont;

    cond_element_t(std::string l, std::string t, std::string r)
        : stream_element_t(), small(std::move(l)), tail(std::move(t)),
          cont(std::move(r)) {}
    cond_element_t(std::string l, std::string t, std::string r, size_t hpos)
        : stream_element_t(hpos), small(std::move(l)), tail(std::move(t)),
          cont(std::move(r)) {}
    virtual ~cond_element_t() {}
    virtual std::string str() const {
        return "CE(\"" + small + "\",\"" + tail + "\",\"" + cont + "\","
            + pos_or_not() + ")";
    }

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
};

class nbeg_element_t : public stream_element_t {
public:
    nbeg_element_t() : stream_element_t() {}
    nbeg_element_t(size_t hpos) : stream_element_t(hpos) {}
    virtual ~nbeg_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::string str() const {
        return "NBeg(" + pos_or_not() + ")";
    }
};

class nend_element_t : public stream_element_t {
public:
    nend_element_t() : stream_element_t() {}
    nend_element_t(size_t hpos) : stream_element_t(hpos) {}
    virtual ~nend_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::string str() const {
        return "NEnd(" + pos_or_not() + ")";
    }
};

class gbeg_element_t : public stream_element_t {
public:
    gbeg_element_t() : stream_element_t() {}
    gbeg_element_t(size_t hpos) : stream_element_t(hpos) {}
    virtual ~gbeg_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::string str() const {
        return "GBeg(" + pos_or_not() + ")";
    }
};

class gend_element_t : public stream_element_t {
public:
    gend_element_t() : stream_element_t() {}
    gend_element_t(size_t hpos) : stream_element_t(hpos) {}
    virtual ~gend_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::string str() const {
        return "GEnd(" + pos_or_not() + ")";
    }
};

class fn_wrapper_t {
    std::shared_ptr<stream_element_visitor_t> v;
    std::string name;

public:
    fn_wrapper_t(std::shared_ptr<stream_element_visitor_t> _v, std::string _name)
        : v(_v), name(_name) {}

    void operator()(stream_handle_t e) {
        e->visit(*v);
    }
};

typedef std::shared_ptr<fn_wrapper_t> thunk_t;

class generate_stream_visitor_t : public document_visitor_t {
    thunk_t fn;
public:
    generate_stream_visitor_t(thunk_t f) : fn(f) {}
    virtual ~generate_stream_visitor_t() {}

    virtual void operator()(const text_t &t) const {
        (*fn)(std::make_shared<text_element_t>(t.text));
    }

    virtual void operator()(const cond_t &c) const {
        (*fn)(std::make_shared<cond_element_t>(c.small, c.tail, c.cont));
    }

    virtual void operator()(const concat_t &c) const {
        std::for_each(c.children.begin(), c.children.end(),
                      [this](doc_handle_t d) { d->visit(*this); });
    }

    virtual void operator()(const group_t &g) const {
        (*fn)(std::make_shared<gbeg_element_t>());
        g.child->visit(*this);
        (*fn)(std::make_shared<gend_element_t>());
    }

    virtual void operator()(const nest_t &n) const {
        (*fn)(std::make_shared<nbeg_element_t>());
        (*fn)(std::make_shared<gbeg_element_t>());
        n.child->visit(*this);
        (*fn)(std::make_shared<gend_element_t>());
        (*fn)(std::make_shared<nend_element_t>());
    }
};

void generate_stream(doc_handle_t doc, thunk_t fn) {
    generate_stream_visitor_t v(std::move(fn));
    doc->visit(v);
}

class annotate_stream_visitor_t : public stream_element_visitor_t {
    thunk_t fn;
    unsigned int position;
public:
    annotate_stream_visitor_t(thunk_t f) : fn(f), position(0) {}
    virtual ~annotate_stream_visitor_t() {}

    virtual void operator()(text_element_t &t) {
        position += t.payload.size();
        t.hpos = position;
        (*fn)(t.shared_from_this());
    }

    virtual void operator()(cond_element_t &c) {
        position += c.small.size();
        c.hpos = position;
        (*fn)(c.shared_from_this());
    }

    virtual void operator()(gbeg_element_t &e) {
        e.hpos = position;
        (*fn)(e.shared_from_this());
    }

    virtual void operator()(gend_element_t &e) {
        e.hpos = position;
        (*fn)(e.shared_from_this());
    }

    virtual void operator()(nbeg_element_t &e) {
        e.hpos = position;
        (*fn)(e.shared_from_this());
    }

    virtual void operator()(nend_element_t &e) {
        e.hpos = position;
        (*fn)(e.shared_from_this());
    }
};

thunk_t annotate_stream(thunk_t fn) {
    return std::make_shared<fn_wrapper_t>(
        std::make_shared<annotate_stream_visitor_t>(fn),
        "annotate");
}

class correct_gbeg_visitor_t : public stream_element_visitor_t {
    thunk_t fn;
    typedef std::unique_ptr<std::list<stream_handle_t> > buffer_t;
    std::vector<buffer_t> lookahead;

public:
    correct_gbeg_visitor_t(thunk_t f) : fn(f), lookahead() {}
    virtual ~correct_gbeg_visitor_t() {}

    void maybe_push(stream_element_t &e) {
        if (lookahead.empty()) {
            (*fn)(e.shared_from_this());
        } else {
            lookahead.back()->push_back(e.shared_from_this());
        }
    }

    virtual void operator()(text_element_t &t) {
        guarantee(t.hpos);
        maybe_push(t);
    }

    virtual void operator()(cond_element_t &c) {
        guarantee(c.hpos);
        maybe_push(c);
    }

    virtual void operator()(nbeg_element_t &e) {
        guarantee(e.hpos);
        maybe_push(e);
    }

    virtual void operator()(nend_element_t &e) {
        guarantee(e.hpos);
        maybe_push(e);
    }

    virtual void operator()(gbeg_element_t &) {
        lookahead.push_back(buffer_t(new std::list<stream_handle_t>()));
    }

    virtual void operator()(gend_element_t &e) {
        guarantee(e.hpos);
        buffer_t b(std::move(lookahead.back()));
        lookahead.pop_back();
        if (lookahead.empty()) {
            // this is then the topmost group
            (*fn)(std::make_shared<gbeg_element_t>(*(e.hpos)));
            std::for_each(b->begin(), b->end(), *fn);
            (*fn)(e.shared_from_this());
        } else {
            buffer_t &b2 = lookahead.back();
            b2->push_back(std::make_shared<gbeg_element_t>(*(e.hpos)));
            b2->splice(b2->end(), *b);
            b2->push_back(e.shared_from_this());
        }
    }
};

thunk_t correct_gbeg_stream(thunk_t fn) {
    return std::make_shared<fn_wrapper_t>(
        std::make_shared<correct_gbeg_visitor_t>(fn),
        "correct_gbeg");
}

class output_visitor_t : public stream_element_visitor_t {
    const unsigned int width;
    unsigned int fittingElements, rightEdge, hpos;
    std::vector<unsigned int> indent;
public:
    std::string result;
    output_visitor_t(unsigned int w)
        : width(w), fittingElements(0), rightEdge(w), hpos(0), indent(),
          result() {}
    virtual ~output_visitor_t() {}

    virtual void operator()(text_element_t &t) {
        result += t.payload;
        hpos += t.payload.size();
    }

    virtual void operator()(cond_element_t &c) {
        if (fittingElements == 0) {
            unsigned int currentIndent = indent.empty() ? 0 : indent.back();
            result += c.tail;
            result += '\n';
            result += std::string(currentIndent, ' ');
            result += c.cont;
            fittingElements = 0;
            hpos = currentIndent + c.cont.size();
            rightEdge = (width - hpos) + *(c.hpos);
        } else {
            result += c.small;
            hpos += c.small.size();
        }
    }

    virtual void operator()(gbeg_element_t &e) {
        if (fittingElements != 0 ||
            static_cast<unsigned int>(*(e.hpos)) <= rightEdge) {
            ++fittingElements;
        } else {
            fittingElements = 0;
        }
    }

    virtual void operator()(gend_element_t &) {
        if (fittingElements != 0) {
            --fittingElements;
        }
    }

    virtual void operator()(nbeg_element_t &) {
        indent.push_back(hpos);
    }

    virtual void operator()(nend_element_t &) { indent.pop_back(); }
};

std::string pretty_print(unsigned int width, doc_handle_t doc) {
    std::shared_ptr<output_visitor_t> output =
        std::make_shared<output_visitor_t>(width);
    thunk_t corr_gbeg =
        correct_gbeg_stream(std::make_shared<fn_wrapper_t>(output, "output"));
    thunk_t annotate = annotate_stream(std::move(corr_gbeg));
    generate_stream(doc, std::move(annotate));
    return output->result;
}

} // namespace pprint
