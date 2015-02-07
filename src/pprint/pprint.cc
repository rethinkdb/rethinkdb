#include "pprint/pprint.hpp"

#include <vector>
#include <list>
#include <functional>
#include <memory>

namespace pprint {

const doc_handle_t empty = std::make_shared<text_t>("");
const doc_handle_t br = std::make_shared<cond_t>(" ", "");
const doc_handle_t dot = std::make_shared<cond_t>(".", ".");

struct text_element_t;
struct cond_element_t;
struct nbeg_element_t;
struct nend_element_t;
struct gbeg_element_t;
struct gend_element_t;

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
struct stream_element_t {
    int hpos;                   // -1 means not set yet

    stream_element_t() : hpos(-1) {}
    stream_element_t(unsigned int n) : hpos(n) {}
    virtual ~stream_element_t() {}

    virtual void visit(stream_element_visitor_t &v) = 0;
    virtual std::unique_ptr<stream_element_t> clone() = 0;
};

struct text_element_t : public stream_element_t {
    std::string payload;

    text_element_t(const std::string &text, unsigned int hpos)
        : stream_element_t(hpos), payload(text) {}
    text_element_t(const std::string &text)
        : stream_element_t(), payload(text) {}
    virtual ~text_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::unique_ptr<stream_element_t> clone() {
        return std::unique_ptr<text_element_t>(
            new text_element_t(payload, hpos));
    }
};

struct cond_element_t : public stream_element_t {
    std::string small, tail, cont;

    cond_element_t(const std::string &l, const std::string &t,
                   const std::string &r)
        : stream_element_t(), small(l), tail(t), cont(r) {}
    cond_element_t(const std::string &l, const std::string &t,
                   const std::string &r, unsigned int hpos)
        : stream_element_t(hpos), small(l), tail(t), cont(r) {}
    virtual ~cond_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::unique_ptr<stream_element_t> clone() {
        return std::unique_ptr<cond_element_t>(
            new cond_element_t(small, tail, cont, hpos));
    }
};

struct nbeg_element_t : public stream_element_t {
    nbeg_element_t() : stream_element_t() {}
    nbeg_element_t(unsigned int hpos) : stream_element_t(hpos) {}
    virtual ~nbeg_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::unique_ptr<stream_element_t> clone() {
        return std::unique_ptr<nbeg_element_t>(new nbeg_element_t(hpos));
    }
};

struct nend_element_t : public stream_element_t {
    nend_element_t() : stream_element_t() {}
    nend_element_t(unsigned int hpos) : stream_element_t(hpos) {}
    virtual ~nend_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::unique_ptr<stream_element_t> clone() {
        return std::unique_ptr<nend_element_t>(new nend_element_t(hpos));
    }
};

struct gbeg_element_t : public stream_element_t {
    gbeg_element_t() : stream_element_t() {}
    gbeg_element_t(unsigned int hpos) : stream_element_t(hpos) {}
    virtual ~gbeg_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::unique_ptr<stream_element_t> clone() {
        return std::unique_ptr<gbeg_element_t>(new gbeg_element_t(hpos));
    }
};

struct gend_element_t : public stream_element_t {
    gend_element_t() : stream_element_t() {}
    gend_element_t(unsigned int hpos) : stream_element_t(hpos) {}
    virtual ~gend_element_t() {}

    virtual void visit(stream_element_visitor_t &v) { v(*this); }
    virtual std::unique_ptr<stream_element_t> clone() {
        return std::unique_ptr<gend_element_t>(new gend_element_t(hpos));
    }
};

class fn_wrapper_t {
    std::shared_ptr<stream_element_visitor_t> v;

public:
    fn_wrapper_t(std::shared_ptr<stream_element_visitor_t> _v)
      : v(_v) {}

    void operator()(stream_element_t &e) { e.visit(*v); }
    template <typename T>
    void operator()(T &&t) { T e(t); e.visit(*v); }
    template <typename T> void operator()(std::unique_ptr<T> &t) {
        t->visit(*v);
    }
};

typedef std::shared_ptr<fn_wrapper_t> thunk_t;

class generate_stream_visitor_t : public document_visitor_t {
    thunk_t fn;
public:
    generate_stream_visitor_t(thunk_t &&f) : fn(f) {}
    virtual ~generate_stream_visitor_t() {}

    virtual void operator()(const text_t &t) const {
        (*fn)(text_element_t(t.text));
    }

    virtual void operator()(const cond_t &c) const {
        (*fn)(cond_element_t(c.small, c.tail, c.cont));
    }

    virtual void operator()(const concat_t &c) const {
        std::for_each(c.children.begin(), c.children.end(),
                      [this](doc_handle_t d) { d->visit(*this); });
    }

    virtual void operator()(const group_t &g) const {
        (*fn)(gbeg_element_t());
        g.child->visit(*this);
        (*fn)(gend_element_t());
    }

    virtual void operator()(const nest_t &n) const {
        (*fn)(nbeg_element_t());
        (*fn)(gbeg_element_t());
        n.child->visit(*this);
        (*fn)(gend_element_t());
        (*fn)(nend_element_t());
    }
};

void generate_stream(doc_handle_t doc, thunk_t &&fn) {
    generate_stream_visitor_t v(std::move(fn));
    doc->visit(v);
}

class annotate_stream_visitor_t : public stream_element_visitor_t {
    thunk_t fn;
    unsigned int position;
public:
    annotate_stream_visitor_t(thunk_t &&f) : fn(f), position(0) {}
    virtual ~annotate_stream_visitor_t() {}

    virtual void operator()(text_element_t &t) {
        position += t.payload.size();
        t.hpos = position;
        (*fn)(t);
    }

    virtual void operator()(cond_element_t &c) {
        position += c.small.size();
        c.hpos = position;
        (*fn)(c);
    }

    virtual void operator()(gbeg_element_t &e) {
        e.hpos = position;
        (*fn)(e);
    }

    virtual void operator()(gend_element_t &e) {
        e.hpos = position;
        (*fn)(e);
    }

    virtual void operator()(nbeg_element_t &e) {
        e.hpos = position;
        (*fn)(e);
    }

    virtual void operator()(nend_element_t &e) {
        e.hpos = position;
        (*fn)(e);
    }
};

thunk_t &&annotate_stream(thunk_t &&fn) {
    return std::move(std::make_shared<fn_wrapper_t>(
        std::make_shared<annotate_stream_visitor_t>(std::move(fn))));
}

class correct_gbeg_visitor_t : public stream_element_visitor_t {
    thunk_t fn;
    typedef std::unique_ptr<stream_element_t> elt_t;
    typedef std::unique_ptr<std::list<elt_t> > buffer_t;
    std::vector<buffer_t> lookahead;

public:
    correct_gbeg_visitor_t(thunk_t &&f) : fn(f), lookahead() {}
    virtual ~correct_gbeg_visitor_t() {}

    void maybe_push(stream_element_t &e) {
        if (lookahead.empty()) {
            (*fn)(e);
        } else {
            lookahead.back()->push_back(e.clone());
        }
    }

    virtual void operator()(text_element_t &t) {
        maybe_push(t);
    }

    virtual void operator()(cond_element_t &c) {
        maybe_push(c);
    }

    virtual void operator()(nbeg_element_t &e) {
        maybe_push(e);
    }

    virtual void operator()(nend_element_t &e) {
        maybe_push(e);
    }

    virtual void operator()(gbeg_element_t &) {
        lookahead.push_back(buffer_t(new std::list<elt_t>()));
    }

    virtual void operator()(gend_element_t &e) {
        buffer_t b(std::move(lookahead.back()));
        lookahead.pop_back();
        if (lookahead.empty()) {
            // this is then the topmost group
            (*fn)(gbeg_element_t(e.hpos));
            std::for_each(b->begin(), b->end(), *fn);
            (*fn)(e);
        } else {
            buffer_t &b2 = lookahead.back();
            b2->push_back(
                std::unique_ptr<gbeg_element_t>(new gbeg_element_t(e.hpos)));
            b2->splice(b2->end(), *b);
            b2->push_back(e.clone());
        }
    }
};

thunk_t &&correct_gbeg_stream(thunk_t &&fn) {
    return std::move(std::make_shared<fn_wrapper_t>(
        std::make_shared<correct_gbeg_visitor_t>(std::move(fn))));
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
            unsigned int currentIndent = indent.back();
            result += c.tail;
            result += '\n';
            result += std::string(currentIndent, ' ');
            result += c.cont;
            fittingElements = 0;
            hpos = currentIndent + c.cont.size();
            rightEdge = (width - hpos) + c.hpos;
        } else {
            result += c.small;
            hpos += c.small.size();
        }
    }

    virtual void operator()(gbeg_element_t &e) {
        if (fittingElements != 0 ||
            static_cast<unsigned int>(e.hpos) <= rightEdge) {
            ++fittingElements;
        } else {
            fittingElements = 0;
        }
    }

    virtual void operator()(gend_element_t &) {
        if (fittingElements != 0)
            --fittingElements;
    }

    virtual void operator()(nbeg_element_t &) {
        indent.push_back(hpos);
    }

    virtual void operator()(nend_element_t &) { indent.pop_back(); }
};

std::string &&pretty_print(unsigned int width, doc_handle_t doc) {
    std::shared_ptr<output_visitor_t> output =
        std::make_shared<output_visitor_t>(width);
    thunk_t corr_gbeg =
        correct_gbeg_stream(std::make_shared<fn_wrapper_t>(output));
    thunk_t annotate = annotate_stream(std::move(corr_gbeg));
    generate_stream(doc, std::move(annotate));
    return std::move(output->result);
}

} // namespace pprint
