// Copyright 2015 RethinkDB, all rights reserved.
#ifndef PPRINT_PPRINT_HPP_
#define PPRINT_PPRINT_HPP_

#include <string>
#include <vector>
#include <utility>

#include "errors.hpp"
#include <boost/variant.hpp>

namespace unittest { class grouped; }

namespace pprint {

struct text_elem { std::string payload; };
struct crlf_elem { size_t hpos; };

// Primitive for conditional linebreaks.
//
// `small` is what will be printed if there is no linebreak.  If there
// is a linebreak, `tail` will be printed on the previous line (think
// Python's need to backslash escape newlines) and `cont` will be
// printed at the start of the new line.
struct cond_elem { std::string small, cont, tail; size_t hpos; };

// A nest:
// A document enclosure where all linebreaks are indented to the start of the nest.
struct nbeg_elem { };
struct nend_elem { };

// A group:
// A document enclosure where all linebreaks are interpreted consistently.
struct gbeg_elem { size_t end_hpos; };
struct gend_elem { };

struct stream_elem {
    boost::variant<text_elem, crlf_elem, cond_elem, nbeg_elem, nend_elem, gbeg_elem, gend_elem> v;
};

// Specifies a cond_elem.  Doesn't include hpos, because that gets added dynamically.
struct cond_elem_spec {
    std::string small, cont, tail;
};


// Printing an AST generally involves constructing one of these and calling add..() its
// methods as you traverse the AST.
//
// Its general job is to keep track of how many characters you've printed, and to
// annotate crlf_elem, cond_elem, and gbeg_elem with that information.
class pprint_streamer {
public:
    pprint_streamer() : position_(0) { }

    std::vector<stream_elem> elems() RVALUE_THIS { return std::move(elems_); }

    void add(text_elem e) {
        position_ += e.payload.size();
        elems_.push_back(stream_elem{std::move(e)});
    }

    void add_text(std::string &&s) {
        add(text_elem{std::move(s)});
    }

    void add_crlf() {
        elems_.push_back(stream_elem{crlf_elem{position_}});
    }

    void add(cond_elem_spec e) {
        position_ += e.small.size();
        elems_.push_back(stream_elem{
                cond_elem{std::move(e.small), std::move(e.cont), std::move(e.tail), position_}});
    }

private:
    size_t add_gbeg() {
        size_t ret = elems_.size();
        elems_.push_back(stream_elem{gbeg_elem{SIZE_MAX}});
        return ret;
    }

    void add_gend() {
        elems_.push_back(stream_elem{gend_elem{}});
    }

    void add_nbeg() {
        elems_.push_back(stream_elem{nbeg_elem{}});
    }

    void add_nend() {
        elems_.push_back(stream_elem{nend_elem{}});
    }

    void update_gbeg(size_t index) {
        gbeg_elem &elem = boost::get<gbeg_elem>(elems_.at(index).v);
        rassert(elem.end_hpos == SIZE_MAX);
        elem.end_hpos = position_;
    }

    friend class nested;
    friend class unittest::grouped;

    std::vector<stream_elem> elems_;
    size_t position_;
};

class nested {
    pprint_streamer *pp_;
    size_t gbeg_index_;
    DISABLE_COPYING(nested);
public:
    explicit nested(pprint_streamer *pp) : pp_(pp) {
        pp->add_nbeg();
        gbeg_index_ = pp->add_gbeg();
    }
    ~nested() {
        pp_->update_gbeg(gbeg_index_);
        pp_->add_gend();
        pp_->add_nend();
    }
};


std::string pretty_print(size_t width, std::vector<stream_elem> &&elems);

// Prints a variable name from a variable number for use inside of lambda functions.
std::string print_var(int64_t var_num);

} // namespace pprint

#endif  // PPRINT_PPRINT_HPP_
