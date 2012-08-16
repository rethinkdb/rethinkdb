#ifndef TIMESTAMPS_HPP_
#define TIMESTAMPS_HPP_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include "containers/printf_buffer.hpp"
#include "repli_timestamp.hpp"
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

    state_timestamp_t plus_integer(int n) {
        state_timestamp_t t;
        t.num = num + n;
        return t;
    }

    // TODO get rid of this. This is only for a hack until we know what to do with timestamps
    uint64_t numeric_representation() const {
        return num;
    }

    // TODO get rid of this. This is only for a hack until we know what to do with timestamps
    repli_timestamp_t to_repli_timestamp() const {
        repli_timestamp_t ts;
        ts.longtime = num;
        return ts;
    }

    friend void debug_print(append_only_printf_buffer_t *buf, state_timestamp_t ts);

private:
    friend class transition_timestamp_t;
    uint64_t num;
    RDB_MAKE_ME_SERIALIZABLE_1(num);
};

inline void debug_print(append_only_printf_buffer_t *buf, state_timestamp_t ts) {
    buf->appendf("st_t{");
    debug_print(buf, ts.num);
    buf->appendf("}");
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

    transition_timestamp_t plus_integer(int n) {
        transition_timestamp_t t;
        t.before = before.plus_integer(n);
        return t;
    }

    state_timestamp_t timestamp_before() const {
        return before;
    }

    state_timestamp_t timestamp_after() const {
        state_timestamp_t after;
        after.num = before.num + 1;
        guarantee(after > before, "timestamp counter overflowed");
        return after;
    }

    // TODO get rid of this. This is only for a hack until we know what to do with timestamps
    uint64_t numeric_representation() const {
        return before.num;
    }

    // TODO get rid of this. This is only for a hack until we know what to do with timestamps
    repli_timestamp_t to_repli_timestamp() const {
        return before.to_repli_timestamp();
    }

private:
    state_timestamp_t before;
    RDB_MAKE_ME_SERIALIZABLE_1(before);
};


#endif /* TIMESTAMPS_HPP_ */
