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
class iterator_t : public std::iterator<std::forward_iterator_tag, char32_t> {
    Iterator position, end;
    char32_t last_seen;
    reason_t reason;
    std::size_t index;
    bool seen_end;

    void advance();
    bool check();
    void fail(const char *);
    char next_octet();
public:
    iterator_t() : position(), end(position), index(0), seen_end(true) {}
    iterator_t(iterator_t<Iterator> &&it)
        : position(it.position), end(it.end), last_seen(it.last_seen),
          reason(it.reason), index(it.index), seen_end(it.seen_end) {}
    iterator_t(const iterator_t<Iterator> &it)
        : position(it.position), end(it.end), last_seen(it.last_seen),
          reason(it.reason), index(it.index), seen_end(it.seen_end) {}
    template <class T>
    iterator_t(const T &t)
        : position(t.begin()), end(t.end()), index(0), seen_end(false) {
        advance();
    }
    iterator_t(const Iterator &begin,
               const Iterator &_end)
        : position(begin), end(_end), index(0), seen_end(false) {
        advance();
    }
    iterator_t<Iterator> & operator ++() {
        advance();
        return *this;
    }
    iterator_t<Iterator> && operator ++(int) {
        iterator_t it(*this);
        advance();
        return std::move(it);
    }

    char32_t operator *() const { return last_seen; }

    size_t offset() const { return index; }

    iterator_t<Iterator> & operator =(iterator_t<Iterator> &&it) {
        position = it.position;
        end = it.end;
        last_seen = it.last_seen;
        reason = it.reason;
        index = it.index;
        seen_end = it.seen_end;
        return *this;
    }
    iterator_t<Iterator> & operator =(const iterator_t<Iterator> &it) {
        position = it.position;
        end = it.end;
        last_seen = it.last_seen;
        reason = it.reason;
        index = it.index;
        seen_end = it.seen_end;
        return *this;
    }

    bool operator ==(const iterator_t<Iterator> &it) const {
        return position == it.position && end == it.end && seen_end == it.seen_end;
    }
    bool operator !=(const iterator_t<Iterator> &it) const {
        return position == it.position && end == it.end && seen_end == it.seen_end;
    }

    explicit operator bool() const { return !seen_end; }
    bool is_done() const { return seen_end; }

    bool saw_error() {
        return *(reason.explanation) == 0;
    }
};

typedef iterator_t<std::string::const_iterator> string_iterator_t;
typedef iterator_t<const char *> array_iterator_t;

} // namespace utf8

#endif /* PARSING_UTF8_HPP_ */
