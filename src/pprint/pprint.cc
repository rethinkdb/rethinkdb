// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "pprint/pprint.hpp"

#include <algorithm>
#include <vector>
#include <list>
#include <functional>
#include <memory>
#include <string>

#include "utils.hpp"

namespace pprint {

// Pretty printing occurs in two passes.
//
// First we traverse an AST, producing stream elements annotated with character offset
// info.  Namely, gbeg gets annotated with the character offset of its corresponding
// gend.  (That's why we use two passes.)
//
// Second, we output.  Each time we see a `gbeg_elem`, we can compare its `end_hpos`
// with `rightEdge` to see whether it'll fit without breaking.  If it does fit,
// increment `fittingElements` and proceed, which will cause the logic for
// `text_element_t` and `cond_element_t` to just append stuff without line breaks.  If
// it doesn't fit, set `fittingElements` to 0, which will cause `cond_element_t` to do
// line breaks.  When we do a line break, we need to compute where the new right edge of
// the 'page' would be in the context of the original stream; so if we saw a
// `cond_element_t` with `e.hpos` of 300 (meaning it ends at horizontal position 300),
// the new right edge would be 300 - indentation + page width.
class output_visitor_t : public boost::static_visitor<void> {
    const size_t width;
    size_t fittingElements, rightEdge, hpos;
    std::vector<size_t> indent;
public:
    std::string result;
    explicit output_visitor_t(size_t w)
        : width(w), fittingElements(0), rightEdge(w), hpos(0), indent(),
          result() {}

    void operator()(const text_elem &t) {
        result += t.payload;
        hpos += t.payload.size();
    }

    void operator()(const crlf_elem &c) {
        size_t currentIndent = indent.empty() ? 0 : indent.back();
        result += '\n';
        result += std::string(currentIndent, ' ');
        fittingElements = 0;
        hpos = currentIndent;
        rightEdge = (width - hpos) + c.hpos;
    }

    void operator()(const cond_elem &c) {
        if (fittingElements == 0) {
            size_t currentIndent = indent.empty() ? 0 : indent.back();
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

    void operator()(const gbeg_elem &e) {
        if (fittingElements != 0 || e.end_hpos <= rightEdge) {
            ++fittingElements;
        } else {
            fittingElements = 0;
        }
    }

    void operator()(const gend_elem &) {
        if (fittingElements != 0) {
            --fittingElements;
        }
    }

    void operator()(const nbeg_elem &) {
        indent.push_back(hpos);
    }

    void operator()(const nend_elem &) {
        indent.pop_back();
    }
};

std::string pretty_print(size_t width, std::vector<stream_elem> &&elems) {
    output_visitor_t output(width);
    for (auto &e : elems) {
        boost::apply_visitor(output, e.v);
    }

    return std::move(output.result);
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
