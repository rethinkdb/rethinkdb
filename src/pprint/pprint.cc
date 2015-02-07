#include "pprint/pprint.cc"

namespace pprint {

// Streaming version of the document tree, suitable for printing after
// much processing
struct stream_element_t {
    int position() const { return hpos; }

    stream_element_t() : hpos(-1) {}
    stream_element_t(unsigned int n) : hpos(n) {}

    int hpos;                   // -1 means not set yet
};

struct text_element_t : public stream_element_t {
    std::string payload;

    text_element_t(const std::string &text, unsigned int hpos)
        : stream_element_t(hpos), payload(text) {}
    text_element_t(const std::string &text)
        : stream_element_t(), payload(text) {}

    const std::string &text() const { return payload; }
};

struct cond_element_t : public stream_element_t {
    std::string small, tail, cont;

    cond_element_t(const std::string &l, const std::string &t,
                   const std::string &r)
        : stream_element_t(), small(l), tail(t), cont(r) {}
    cond_element_t(const std::string &l, const std::string &t,
                   const std::string &r, unsigned int hpos)
        : stream_element_t(hpos), small(l), tail(t), cont(r) {}

    const std::string &text() const { return payload; }
};

struct nbeg_element_t : public stream_element_t {

    nbeg_element_t() : stream_element_t() {}
    nbeg_element_t(unsigned int hpos) : stream_element_t(hpos) {}
};

struct nend_element_t : public stream_element_t {

    nend_element_t() : stream_element_t() {}
    nend_element_t(unsigned int hpos) : stream_element_t(hpos) {}
};

struct gbeg_element_t : public stream_element_t {

    gbeg_element_t() : stream_element_t() {}
    gbeg_element_t(unsigned int hpos) : stream_element_t(hpos) {}
};

struct gend_element_t : public stream_element_t {

    gend_element_t() : stream_element_t() {}
    gend_element_t(unsigned int hpos) : stream_element_t(hpos) {}
};

typedef boost::variant<text_element_t, cond_element_t, nbeg_element_t,
                       nend_element_t, gbeg_element_t, gend_element_t> stream_elt_t;

class generate_stream_visitor : public boost::static_visitor<> {
    const std::function<void(stream_elt_t &)> &fn;
public:
    generate_stream_visitor(const std::function<void(stream_elt_t &)> &f) : fn(f) {}

    void operator(text_t &t) const {
        fn(text_element_t(t.text));
    }

    void operator(cond_t &c) const {
        fn(cond_element_t(c.small, c.tail, c.cont));
    }

    void operator(concat_t &c) const {
        std::for_each(c.children.begin(), c.children.end(), boost::apply_visitor(*this));
    }

    void operator(group_t &g) const {
        fn(gbeg_element_t());
        boost::apply_visitor(*this, g.child);
        fn(gend_element_t());
    }

    void operator(nest_t &n) const {
        fn(nbeg_element_t());
        fn(gbeg_element_t());
        std::for_each(n.children.begin(), n.children.end(), boost::apply_visitor(*this));
        fn(gend_element_t());
        fn(nend_element_t());
    }
};

void generate_stream(const document_t &doc,
                     const std::function<void(stream_elt_t &)> &fn) {
    boost::apply_visitor(generate_stream_visitor(fn), doc);
}

class annotate_stream_visitor : public boost::static_visitor<> {
    const std::function<void(stream_elt_t &)> &fn;
    unsigned int position;
public:
    annotate_stream_visitor(const std::function<void(stream_elt_t &)> &f)
        : fn(f), position(0) {}

    void operator(text_element_t &t) {
        position += t.payload.size();
        t.hpos = position;
        fn(t);
    }

    void operator(cond_element_t &c) {
        position += c.small.size();
        c.hpos = position;
        fn(c);
    }

    void operator(gbeg_element_t &e) {
        e.hpos = position;
        fn(e);
    }

    void operator(gend_element_t &e) {
        e.hpos = position;
        fn(e);
    }

    void operator(nbeg_element_t &e) {
        e.hpos = position;
        fn(e);
    }

    void operator(nend_element_t &e) {
        e.hpos = position;
        fn(e);
    }
};

std::function<void(stream_elt_t &)> &&
annotate_stream(const std::function<T(stream_elt_t &)> &fn) {
    boost::apply_visitor(annotate_stream_visitor(fn));
}

class correct_gbeg_visitor : public boost::static_visitor<> {
    const std::function<void(stream_elt_t &)> &fn;
    std::vector<std::list<stream_elt_t> > lookahead;
public:
    correct_gbeg_visitor(const std::function<void(stream_elt_t &)> &f)
        : fn(f), lookahead() {}

    template <typename T>
    void operator(T &t) {
        if (lookahead.empty()) {
            fn(t);
        } else {
            lookahead.back().push_back(t);
        }
    }

    void operator(gbeg_element_t &e) {
        lookahead.push_back(std::list<stream_elt_t>());
    }

    void operator(gend_element_t &e) {
        std::list<stream_elt_t> b = lookahead.pop_back();
        if (lookahead.empty()) {
            // this is then the topmost group
            fn(gbeg_element_t(e.hpos));
            std::for_each(b.begin(), b.end(), fn);
            fn(e);
        } else {
            std::list<stream_elt_t> &b2 = lookahead.back();
            b2.push_back(gbeg_element_t(e.hpos));
            b2.splice(b2.end(), b);
            b2.push_back(e);
        }
    }
};

std::function<void(stream_elt_t &)> &&
correct_gbeg_stream(const std::function<T(stream_elt_t &)> &fn) {
    boost::apply_visitor(correct_gbeg_visitor(fn));
}

class output_visitor : public boost::static_visitor<> {
    const unsigned int width;
    unsigned int fittingElements, rightEdge, hpos;
    std::vector<unsigned int> indent;
public:
    std::string result;
    output_visitor(unsigned int w)
        : width(w), fittingElements(0), rightEdge(w), hpos(0), result(), indent() {}

    void operator(text_element_t &t) {
        result += t.payload;
        hpos += t.payload.size();
    }

    void operator(cond_element_t &c) {
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

    void operator(gbeg_element_t &e) {
        if (fittingElements != 0 || e.hpos <= rightEdge) {
            ++fittingElements;
        } else {
            fittingElements = 0;
        }
    }

    void operator(gend_element_t &e) {
        if (fittingElements != 0)
            --fittingElements;
    }

    void operator(nbeg_element_t &e) {
        indent.push_back(hpos);
    }

    void operator(nend_element_t &e) {
        indent.pop();
    }
};

std::string &&pretty_print(unsigned int width, const std::shared_ptr<document_t> doc) {
    output_visitor output(width);
    generate_stream(doc,
                    annotate_stream(correct_gbeg_stream(boost::apply_visitor(output))));
    return std::move(output.result);
}

} // namespace pprint
