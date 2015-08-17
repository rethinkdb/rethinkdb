// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PARSING_UTF8_HPP_
#define PARSING_UTF8_HPP_

#include <string>
#include <iterator>

class datum_string_t;

namespace utf8 {

// Simple UTF-8 validation.
bool is_valid(const std::string &);
bool is_valid(const char *, const char *);
bool is_valid(const datum_string_t &);

struct reason_t {
    const char *explanation;
    size_t position;
};

// UTF-8 validation with an explanation
bool is_valid(const std::string &, reason_t *);
bool is_valid(const char *, const char *, reason_t *);
bool is_valid(const datum_string_t &, reason_t *);

template <class Iterator>
Iterator next_codepoint(Iterator start, Iterator end) {
    reason_t reason;
    char32_t codepoint;
    return next_codepoint(std::forward<Iterator>(start),
                          std::forward<Iterator>(end), &codepoint, &reason);
}

template <class Iterator>
Iterator next_codepoint(Iterator start, Iterator end, reason_t *reason) {
    char32_t codepoint;
    return next_codepoint(std::forward<Iterator>(start),
                          std::forward<Iterator>(end), &codepoint, reason);
}

template <class Iterator>
Iterator next_codepoint(Iterator start, Iterator end, char32_t *codepoint) {
    reason_t reason;
    return next_codepoint(std::forward<Iterator>(start),
                          std::forward<Iterator>(end), codepoint, &reason);
}

template <class Iterator>
Iterator next_codepoint(Iterator start, Iterator end, char32_t *codepoint,
                        reason_t *reason);

template <class Iterator>
class iterator_t : public std::iterator<std::forward_iterator_tag, char32_t> {
    Iterator start, position, end;
    char32_t last_seen;
    reason_t reason;
    bool seen_end;

    void advance();
public:
    iterator_t() : start(), position(start), end(position), seen_end(true) {}
    iterator_t(iterator_t<Iterator> &&it) = default;
    iterator_t(const iterator_t<Iterator> &it) = default;
    template <class T>
    explicit iterator_t(const T &t)
        : start(t.begin()), position(t.begin()), end(t.end()),
          seen_end(t.begin() == t.end()) {
        advance();
    }
    iterator_t(const Iterator &_begin,
               const Iterator &_end)
        : start(_begin), position(_begin), end(_end), seen_end(_begin == _end) {
        advance();
    }
    iterator_t<Iterator> &operator++() {
        advance();
        return *this;
    }
    iterator_t<Iterator> operator++(int) {
        iterator_t it(*this);
        advance();
        return it;
    }

    char32_t operator*() const { return last_seen; }

    iterator_t<Iterator> &operator=(iterator_t<Iterator> &&it) = default;
    iterator_t<Iterator> &operator=(const iterator_t<Iterator> &it) = default;

    bool operator==(const iterator_t<Iterator> &it) const {
        if (seen_end && it.seen_end) return true;
        return start == it.start && position == it.position && end == it.end;
    }
    bool operator!=(const iterator_t<Iterator> &it) const {
        return !(*this == it);
    }

    explicit operator bool() const { return !seen_end; }
    bool is_done() const { return seen_end; }

    bool saw_error() {
        return *(reason.explanation) != 0;
    }

    const reason_t & error_explanation() const { return reason; }
};

typedef iterator_t<std::string::const_iterator> string_iterator_t;
typedef iterator_t<const char *> array_iterator_t;

} // namespace utf8

#endif /* PARSING_UTF8_HPP_ */
