// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "pprint/pprint.hpp"

#include <vector>
#include <list>
#include <functional>
#include <memory>
#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "rdb_protocol/error.hpp"

namespace pprint {

// Pretty printing occurs in two global phases.  Rather than try to
// print some random C++ tree directly, which could get quite ugly
// quite quickly, we build a "pretty printer document" out of some
// very simple primitives.  These primitives (and our algorithm) are
// due to Oppen[1] originally and later Kiselyov[2].  Oppen's original
// formulation had `Text`, `LineBreak`, `Concat`, and `Group`.  I
// generalized `LineBreak` to `cond_t` which became our `cond_t`
// because we need to do more sophisticated breaks, and I added
// `nest_t` for controllable indentation.
//
// [1]: Oppen, D.C.: Prettyprinting. ACM Trans. Program. Lang. Syst. 2
//      (1980) 465–483.  Not available online without an ACM subscription.
//
// [2]: Kiselyov, O., Petyon-Jones, S. and Sabry, A.: Lazy v. Yield:
//      Incremental, Linear Pretty-printing.  Available online at
//      http://okmij.org/ftp/continuations/PPYield/yield-pp.pdf

class text_t;
class linebreak_t;
class cond_t;
class concat_t;
class group_t;
class nest_t;

class document_visitor_t {
public:
    virtual ~document_visitor_t() {}

    virtual void operator()(const text_t &) const = 0;
    virtual void operator()(const linebreak_t &) const = 0;
    virtual void operator()(const cond_t &) const = 0;
    virtual void operator()(const concat_t &) const = 0;
    virtual void operator()(const group_t &) const = 0;
    virtual void operator()(const nest_t &) const = 0;
};

class text_t : public document_t {
public:
    std::string text;

    explicit text_t(const std::string &_str) : text(_str) {}
    explicit text_t(std::string &&_str) : text(std::move(_str)) {}
    ~text_t() override {}

    size_t width() const override { return text.length(); }
    void visit(const document_visitor_t &v) const override { v(*this); }
    std::string str() const override { return "Text(\"" + text + "\")"; }
};

counted_t<const document_t> make_text(const std::string text) {
    return make_counted<text_t>(std::move(text));
}

class cond_t : public document_t {
public:
    std::string small, cont, tail;

    cond_t(const std::string l, const std::string r, const std::string t="")
        : small(std::move(l)), cont(std::move(r)), tail(std::move(t)) {}
    ~cond_t() override {}

    // no linebreaks, so only `small` is relevant
    size_t width() const override { return small.length(); }
    std::string str() const override {
        return "Cond(\"" + small + "\",\"" + cont + "\",\"" + tail + "\")";
    }

    void visit(const document_visitor_t &v) const override { v(*this); }
};

counted_t<const document_t> make_cond(const std::string l, const std::string r,
                       const std::string t) {
    return make_counted<cond_t>(std::move(l), std::move(r), std::move(t));
}

class linebreak_t : public document_t {
public:
    linebreak_t() {}
    ~linebreak_t() override {}

    size_t width() const override { return 0; }
    void visit(const document_visitor_t &v) const override { v(*this); }
    std::string str() const override { return "CR"; }
};

// concatenation of multiple documents
class concat_t : public document_t {
public:
    std::vector<counted_t<const document_t> > children;

    explicit concat_t(std::vector<counted_t<const document_t> > args)
        : children(std::move(args)) {}
    template <typename It>
    concat_t(It begin, It end)
        : children(std::move(begin), std::move(end)) {}
    explicit concat_t(std::initializer_list<counted_t<const document_t> > init)
        : children(std::move(init)) {}
    ~concat_t() override {}

    size_t width() const override {
        size_t w = 0;
        for (const auto &child : children) {
            w += child->width();
        }
        return w;
    }
    std::string str() const override {
        std::string result = "";
        for (const auto &child : children) {
            result += child->str();
        }
        return result;
    }

    void visit(const document_visitor_t &v) const override { v(*this); }
};

counted_t<const document_t> make_concat(std::vector<counted_t<const document_t> > args) {
    return make_counted<concat_t>(std::move(args));
}
counted_t<const document_t>
make_concat(std::initializer_list<counted_t<const document_t> > args) {
    return make_counted<concat_t>(std::move(args));
}

class group_t : public document_t {
public:
    counted_t<const document_t> child;

    explicit group_t(counted_t<const document_t> doc) : child(doc) {}
    ~group_t() override {}

    size_t width() const override {
        return child->width();
    }
    std::string str() const override {
        return "Group(" + child->str() + ")";
    }

    void visit(const document_visitor_t &v) const override { v(*this); }
};

counted_t<const document_t> make_group(counted_t<const document_t> child) {
    return make_counted<group_t>(child);
}

class nest_t : public document_t {
public:
    counted_t<const document_t> child;

    explicit nest_t(counted_t<const document_t> doc) : child(doc) {}
    ~nest_t() override {}

    size_t width() const override {
        return child->width();
    }
    std::string str() const override {
        return "Nest(" + child->str() + ")";
    }

    void visit(const document_visitor_t &v) const override { v(*this); }
};

counted_t<const document_t> make_nest(counted_t<const document_t> child) {
    return make_counted<nest_t>(child);
}

const counted_t<const document_t> empty = make_counted<text_t>("");
const counted_t<const document_t> cond_linebreak = make_counted<cond_t>(" ", "");
const counted_t<const document_t> dot_linebreak = make_counted<cond_t>(".", ".");
const counted_t<const document_t> uncond_linebreak = make_counted<linebreak_t>();

counted_t<const document_t>
comma_separated(std::initializer_list<counted_t<const document_t> > init) {
    if (init.size() == 0) return empty;
    std::vector<counted_t<const document_t> > v;
    auto it = init.begin();
    v.push_back(*it++);
    for (; it != init.end(); it++) {
        v.push_back(make_counted<text_t>(","));
        v.push_back(cond_linebreak);
        v.push_back(*it);
    }
    return make_nest(make_concat(std::move(v)));
}

counted_t<const document_t>
arglist(std::initializer_list<counted_t<const document_t> > init) {
    static const counted_t<const document_t> lparen = make_counted<text_t>("(");
    static const counted_t<const document_t> rparen = make_counted<text_t>(")");
    return make_concat({ lparen, comma_separated(init), rparen });
}

template <typename It>
counted_t<const document_t> dotted_list_int(It begin, It end) {
    static const counted_t<const document_t> plain_dot = make_counted<text_t>(".");
    if (begin == end) return empty;
    std::vector<counted_t<const document_t> > v;
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
    return make_concat({v[0], make_nest(make_concat(v.begin()+1, v.end()))});
}

counted_t<const document_t>
dotted_list(std::initializer_list<counted_t<const document_t> > init) {
    return dotted_list_int(init.begin(), init.end());
}

counted_t<const document_t> funcall(const std::string &name,
                     std::initializer_list<counted_t<const document_t> > init) {
    return make_concat({make_text(name), arglist(init)});
}

counted_t<const document_t>
r_dot(std::initializer_list<counted_t<const document_t> > args) {
    static const counted_t<const document_t> r = make_counted<text_t>("r");
    std::vector<counted_t<const document_t> > v;
    v.push_back(r);
    v.insert(v.end(), args.begin(), args.end());
    return dotted_list_int(v.begin(), v.end());
}

// The document tree is convenient for certain operations, but we're
// going to convert it straightaway into a linear stream through
// essentially an in-order traversal.  We do this because it's easier
// to compute the width in linear time; it's possible to do it
// directly on the tree, but the naive algorithm recomputes the widths
// constantly and a dynamic programming or memorized version is more
// annoying.  This stream has the attractive property that we can
// process it one element at a time, so it does not need to be created
// in its entirety.
//
// Our stream type translates `text_t` to `text_element_t` and
// `cond_t` to `cond_element_t`.  Since we're streaming, the extra
// structure for the `concat_t` goes away.  The tricky ones are groups
// and nests, which must preserve their heirarchy somehow.  We do this
// by wrapping the child contents with a GBeg or here `gbeg_element_t`
// meaning Group Begin and ending with a GEnd or `gend_element_t`
// meaning Group End.  Similarly with `nbeg_element_t` and
// `nend_element_t`.

class text_element_t;
class cond_element_t;
class crlf_element_t;
class nbeg_element_t;
class nend_element_t;
class gbeg_element_t;
class gend_element_t;

class stream_element_visitor_t
    : public single_threaded_countable_t<stream_element_visitor_t> {
public:
    virtual ~stream_element_visitor_t() {}

    // boost::static_visitor uses references for this pattern, but because
    // our style guide says that we use pointers if we will modify the
    // parameter, we use pointers here.  The originals are invariably
    // stored in some `counted_t`.
    virtual void operator()(text_element_t *) = 0;
    virtual void operator()(cond_element_t *) = 0;
    virtual void operator()(crlf_element_t *) = 0;
    virtual void operator()(nbeg_element_t *) = 0;
    virtual void operator()(nend_element_t *) = 0;
    virtual void operator()(gbeg_element_t *) = 0;
    virtual void operator()(gend_element_t *) = 0;
};

// Streaming version of the document tree, suitable for printing after
// much processing.  Because we don't have any constant values here
// due to the `hpos` stuff, this can be `single_threaded_countable_t`.
class stream_element_t : public single_threaded_countable_t<stream_element_t> {
public:
    friend class annotate_stream_visitor_t;
    friend class correct_gbeg_visitor_t;
    boost::optional<size_t> hpos;

    stream_element_t() : hpos() {}
    explicit stream_element_t(size_t n) : hpos(n) {}
    virtual ~stream_element_t() {}

    virtual void visit(stream_element_visitor_t *v) = 0;
    virtual std::string str() const = 0;
    std::string pos_or_not() const {
        return hpos ? std::to_string(*hpos) : "-1";
    }
};

class text_element_t : public stream_element_t {
public:
    std::string payload;

    text_element_t(const std::string &text, size_t _hpos)
        : stream_element_t(_hpos), payload(text) {}
    explicit text_element_t(const std::string &text)
        : stream_element_t(), payload(text) {}
    ~text_element_t() override {}
    std::string str() const override {
        return "TE(\"" + payload + "\"," + pos_or_not() + ")";
    }

    void visit(stream_element_visitor_t *v) override { (*v)(this); }
};

class cond_element_t : public stream_element_t {
public:
    std::string small, tail, cont;

    cond_element_t(std::string l, std::string t, std::string r)
        : stream_element_t(), small(std::move(l)), tail(std::move(t)),
          cont(std::move(r)) {}
    cond_element_t(std::string l, std::string t, std::string r, size_t _hpos)
        : stream_element_t(_hpos), small(std::move(l)), tail(std::move(t)),
          cont(std::move(r)) {}
    ~cond_element_t() override {}
    std::string str() const override {
        return "CE(\"" + small + "\",\"" + tail + "\",\"" + cont + "\","
            + pos_or_not() + ")";
    }

    void visit(stream_element_visitor_t *v) override { (*v)(this); }
};

class crlf_element_t : public stream_element_t {
public:
    crlf_element_t() : stream_element_t() {}
    explicit crlf_element_t(size_t _hpos) : stream_element_t(_hpos) {}
    ~crlf_element_t() override {}

    void visit(stream_element_visitor_t *v) override { (*v)(this); }
    std::string str() const override {
        return "CR(" + pos_or_not() + ")";
    }
};

class nbeg_element_t : public stream_element_t {
public:
    nbeg_element_t() : stream_element_t() {}
    explicit nbeg_element_t(size_t _hpos) : stream_element_t(_hpos) {}
    ~nbeg_element_t() override {}

    void visit(stream_element_visitor_t *v) override { (*v)(this); }
    std::string str() const override {
        return "NBeg(" + pos_or_not() + ")";
    }
};

class nend_element_t : public stream_element_t {
public:
    nend_element_t() : stream_element_t() {}
    explicit nend_element_t(size_t _hpos) : stream_element_t(_hpos) {}
    ~nend_element_t() override {}

    void visit(stream_element_visitor_t *v) override { (*v)(this); }
    std::string str() const override {
        return "NEnd(" + pos_or_not() + ")";
    }
};

class gbeg_element_t : public stream_element_t {
public:
    gbeg_element_t() : stream_element_t() {}
    explicit gbeg_element_t(size_t _hpos) : stream_element_t(_hpos) {}
    ~gbeg_element_t() override {}

    void visit(stream_element_visitor_t *v) override { (*v)(this); }
    std::string str() const override {
        return "GBeg(" + pos_or_not() + ")";
    }
};

class gend_element_t : public stream_element_t {
public:
    gend_element_t() : stream_element_t() {}
    explicit gend_element_t(size_t _hpos) : stream_element_t(_hpos) {}
    ~gend_element_t() override {}

    void visit(stream_element_visitor_t *v) override { (*v)(this); }
    std::string str() const override {
        return "GEnd(" + pos_or_not() + ")";
    }
};

// Once we have the stream, we can begin massaging it prior to pretty
// printing.  C++ native streams aren't really suitable for us; we
// have too much internal state.  Fortunately chain calling functions
// can work, so we set up some machinery to help with that.
class fn_wrapper_t : public single_threaded_countable_t<fn_wrapper_t> {
    counted_t<stream_element_visitor_t> v;
    std::string name;

public:
    fn_wrapper_t(counted_t<stream_element_visitor_t> _v, std::string _name)
        : v(_v), name(_name) {}

    void operator()(counted_t<stream_element_t> e) {
        e->visit(v.get());
    }
};

// The first phase is to just generate the stream elements from the
// document tree, which is simple enough.
class generate_stream_visitor_t : public document_visitor_t {
    counted_t<fn_wrapper_t> fn;
public:
    explicit generate_stream_visitor_t(counted_t<fn_wrapper_t> f) : fn(f) {}
    ~generate_stream_visitor_t() override {}

    void operator()(const text_t &t) const override {
        (*fn)(make_counted<text_element_t>(t.text));
    }

    void operator()(const linebreak_t &) const override {
        (*fn)(make_counted<crlf_element_t>());
    }

    void operator()(const cond_t &c) const override {
        (*fn)(make_counted<cond_element_t>(c.small, c.tail, c.cont));
    }

    void operator()(const concat_t &c) const override {
        std::for_each(c.children.begin(), c.children.end(),
                      [this](counted_t<const document_t> d) { d->visit(*this); });
    }

    void operator()(const group_t &g) const override {
        (*fn)(make_counted<gbeg_element_t>());
        g.child->visit(*this);
        (*fn)(make_counted<gend_element_t>());
    }

    void operator()(const nest_t &n) const override {
        (*fn)(make_counted<nbeg_element_t>());
        (*fn)(make_counted<gbeg_element_t>());
        n.child->visit(*this);
        (*fn)(make_counted<gend_element_t>());
        (*fn)(make_counted<nend_element_t>());
    }
};

void generate_stream(counted_t<const document_t> doc, counted_t<fn_wrapper_t> fn) {
    generate_stream_visitor_t v(fn);
    doc->visit(v);
}

// The second phase is to annotate the stream elements with the
// horizontal position of their last character (assuming no line
// breaks).  We can't actually do this successfully for
// `nbeg_element_t` and `gbeg_element_t` at this time, but everything
// else is pretty easy.
class annotate_stream_visitor_t : public stream_element_visitor_t {
    counted_t<fn_wrapper_t> fn;
    size_t position;
public:
    explicit annotate_stream_visitor_t(counted_t<fn_wrapper_t> f) : fn(f), position(0) {}
    ~annotate_stream_visitor_t() override {}

    void operator()(text_element_t *t) override {
        position += t->payload.size();
        t->hpos = position;
        (*fn)(t->counted_from_this());
    }

    void operator()(crlf_element_t *e) override {
        e->hpos = position;
        (*fn)(e->counted_from_this());
    }

    void operator()(cond_element_t *c) override {
        position += c->small.size();
        c->hpos = position;
        (*fn)(c->counted_from_this());
    }

    void operator()(gbeg_element_t *e) override {
        // can't do this accurately
        (*fn)(e->counted_from_this());
    }

    void operator()(gend_element_t *e) override {
        e->hpos = position;
        (*fn)(e->counted_from_this());
    }

    void operator()(nbeg_element_t *e) override {
        // can't do this accurately
        (*fn)(e->counted_from_this());
    }

    void operator()(nend_element_t *e) override {
        e->hpos = position;
        (*fn)(e->counted_from_this());
    }
};

counted_t<fn_wrapper_t> annotate_stream(counted_t<fn_wrapper_t> fn) {
    return make_counted<fn_wrapper_t>(
        make_counted<annotate_stream_visitor_t>(fn),
        "annotate");
}

// The third phase is to accurately compute the `hpos` for
// `gbeg_element_t`.  We don't care about the hpos for
// `nbeg_element_t`, but the `gbeg_element_t` is important for line
// breaking.  We couldn't accurately annotate it
// `annotate_stream_visitor_t`; this corrects that oversight.
class correct_gbeg_visitor_t : public stream_element_visitor_t {
    counted_t<fn_wrapper_t> fn;
    typedef std::unique_ptr<std::list<counted_t<stream_element_t> > > buffer_t;
    std::vector<buffer_t> lookahead;

public:
    explicit correct_gbeg_visitor_t(counted_t<fn_wrapper_t> f) : fn(f), lookahead() {}
    ~correct_gbeg_visitor_t() override {}

    void maybe_push(stream_element_t *e) {
        if (lookahead.empty()) {
            (*fn)(e->counted_from_this());
        } else {
            lookahead.back()->push_back(e->counted_from_this());
        }
    }

    void operator()(text_element_t *t) override {
        r_sanity_check(t->hpos);
        maybe_push(t);
    }

    void operator()(crlf_element_t *e) override {
        r_sanity_check(e->hpos);
        maybe_push(e);
    }

    void operator()(cond_element_t *c) override {
        r_sanity_check(c->hpos);
        maybe_push(c);
    }

    void operator()(nbeg_element_t *e) override {
        r_sanity_check(!e->hpos);     // don't care about `nbeg_element_t` hpos
        maybe_push(e);
    }

    void operator()(nend_element_t *e) override {
        r_sanity_check(e->hpos);
        maybe_push(e);
    }

    void operator()(gbeg_element_t *e) override {
        r_sanity_check(!e->hpos);     // `hpos` shouldn't be set for `gbeg_element_t`
        lookahead.push_back(buffer_t(new std::list<counted_t<stream_element_t> >()));
    }

    void operator()(gend_element_t *e) override {
        r_sanity_check(e->hpos);
        buffer_t b(std::move(lookahead.back()));
        lookahead.pop_back();
        if (lookahead.empty()) {
            // this is then the topmost group
            (*fn)(make_counted<gbeg_element_t>(*(e->hpos)));
            for (const auto &element : *b) (*fn)(element);
            (*fn)(e->counted_from_this());
        } else {
            buffer_t &b2 = lookahead.back();
            b2->push_back(make_counted<gbeg_element_t>(*(e->hpos)));
            b2->splice(b2->end(), *b);
            b2->push_back(e->counted_from_this());
        }
    }
};

counted_t<fn_wrapper_t> correct_gbeg_stream(counted_t<fn_wrapper_t> fn) {
    return make_counted<fn_wrapper_t>(
        make_counted<correct_gbeg_visitor_t>(fn),
        "correct_gbeg");
}

// Kiselyov's original formulation includes an alternate third phase
// which limits lookahead to the width of the page.  This is difficult
// for us because we don't guarantee docs are of nonzero length,
// although that could be finessed, and also it adds extra complexity
// for minimal benefit, so skip it.

// The final phase is to compute output.  Each time we see a
// `gbeg_element_t`, we can compare its `hpos` with `rightEdge` to see
// whether it'll fit without breaking.  If it does fit, increment
// `fittingElements` and proceed, which will cause the logic for
// `text_element_t` and `cond_element_t` to just append stuff without
// line breaks.  If it doesn't fit, set `fittingElements` to 0, which
// will cause `cond_element_t` to do line breaks.  When we do a line
// break, we need to compute where the new right edge of the 'page'
// would be in the context of the original stream; so if we saw a
// `cond_element_t` with `e.hpos` of 300 (meaning it ends at
// horizontal position 300), the new right edge would be 300 -
// indentation + page width.
//
// `output_visitor_t` outputs to a string which is used as an append
// buffer; it could, in theory, stream the output but this isn't
// useful at present.
class output_visitor_t : public stream_element_visitor_t {
    const size_t width;
    size_t fittingElements, rightEdge, hpos;
    std::vector<size_t> indent;
public:
    std::string result;
    explicit output_visitor_t(size_t w)
        : width(w), fittingElements(0), rightEdge(w), hpos(0), indent(),
          result() {}
    ~output_visitor_t() override {}

    void operator()(text_element_t *t) override {
        result += t->payload;
        hpos += t->payload.size();
    }

    void operator()(crlf_element_t *c) override {
        size_t currentIndent = indent.empty() ? 0 : indent.back();
        result += '\n';
        result += std::string(currentIndent, ' ');
        fittingElements = 0;
        hpos = currentIndent;
        rightEdge = (width - hpos) + *(c->hpos);
    }

    void operator()(cond_element_t *c) override {
        if (fittingElements == 0) {
            size_t currentIndent = indent.empty() ? 0 : indent.back();
            result += c->tail;
            result += '\n';
            result += std::string(currentIndent, ' ');
            result += c->cont;
            fittingElements = 0;
            hpos = currentIndent + c->cont.size();
            rightEdge = (width - hpos) + *(c->hpos);
        } else {
            result += c->small;
            hpos += c->small.size();
        }
    }

    void operator()(gbeg_element_t *e) override {
        if (fittingElements != 0 || *(e->hpos) <= rightEdge) {
            ++fittingElements;
        } else {
            fittingElements = 0;
        }
    }

    void operator()(gend_element_t *) override {
        if (fittingElements != 0) {
            --fittingElements;
        }
    }

    void operator()(nbeg_element_t *) override {
        indent.push_back(hpos);
    }

    void operator()(nend_element_t *) override { indent.pop_back(); }
};

// Here we assemble the chain whose elements we have previously forged.
std::string pretty_print(size_t width, counted_t<const document_t> doc) {
    counted_t<output_visitor_t> output =
        make_counted<output_visitor_t>(width);
    counted_t<fn_wrapper_t> corr_gbeg =
        correct_gbeg_stream(make_counted<fn_wrapper_t>(output, "output"));
    counted_t<fn_wrapper_t> annotate = annotate_stream(corr_gbeg);
    generate_stream(doc, annotate);
    return output->result;
}

std::string print_var(int64_t var_num) {
    if (var_num < 0) {
        // Internal variables
        return "_var" + std::to_string(-var_num);
    } else {
        return "var" + std::to_string(var_num);
    }
}

} // namespace pprint
