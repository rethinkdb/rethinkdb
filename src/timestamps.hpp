#ifndef __TIMESTAMPS_HPP__
#define __TIMESTAMPS_HPP__

#include <stdint.h>

#include "rpc/serialize_macros.hpp"

/* These are the timestamp types used by the clustering code.
`repli_timestamp_t`, which is used internally within the btree code, is defined
elsewhere. */

/* `state_timestamp_t` is a unique identifier of a particular point in a
timeline. `transition_timestamp_t` is the unique identifier of a transition from
one `state_timestamp_t` to the next. Databases have `state_timestamp_t`s, and
transactions have `transition_timestamp_t`s. */

class state_timestamp_t {

public:
    bool operator==(state_timestamp_t t) const { return num == t.num; }
    bool operator!=(state_timestamp_t t) const { return num != t.num; }
    bool operator<(state_timestamp_t t) const { return num < t.num; }
    bool operator>(state_timestamp_t t) const { return num > t.num; }
    bool operator<=(state_timestamp_t t) const { return num <= t.num; }
    bool operator>=(state_timestamp_t t) const { return num >= t.num; }

    static state_timestamp_t zero() {
        state_timestamp_t t;
        t.num = 0;
        return t;
    }

private:
    friend class transition_timestamp_t;
    friend std::ostream &operator<<(std::ostream &os, state_timestamp_t ts);
    uint64_t num;
    RDB_MAKE_ME_SERIALIZABLE_1(num);
};

inline std::ostream &operator<<(std::ostream &os, state_timestamp_t ts) {
    return os << ts.num;
}

class transition_timestamp_t {

public:
    bool operator==(transition_timestamp_t t) const { return before == t.before; }
    bool operator!=(transition_timestamp_t t) const { return before != t.before; }
    bool operator<(transition_timestamp_t t) const { return before < t.before; }
    bool operator>(transition_timestamp_t t) const { return before > t.before; }
    bool operator<=(transition_timestamp_t t) const { return before <= t.before; }
    bool operator>=(transition_timestamp_t t) const { return before >= t.before; }

    static transition_timestamp_t starting_from(state_timestamp_t before) {
        transition_timestamp_t t;
        t.before = before;
        return t;
    }

    state_timestamp_t timestamp_before() {
        return before;
    }

    state_timestamp_t timestamp_after() {
        state_timestamp_t after;
        after.num = before.num + 1;
        guarantee(after > before, "timestamp counter overflowed");
        return after;
    }

private:
    state_timestamp_t before;
    RDB_MAKE_ME_SERIALIZABLE_1(before);
};

#endif /* __TIMESTAMPS_HPP__ */
