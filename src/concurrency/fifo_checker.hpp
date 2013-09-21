// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_FIFO_CHECKER_HPP_
#define CONCURRENCY_FIFO_CHECKER_HPP_

#include <map>
#include <string>
#include <utility>

#include "rpc/serialize_macros.hpp"
#include "containers/uuid.hpp"
#include "utils.hpp"



struct order_bucket_t {
    uuid_u uuid_;

    static order_bucket_t invalid() { return order_bucket_t(nil_uuid()); }
    static order_bucket_t create() { return order_bucket_t(generate_uuid()); }

    bool valid() const;
private:
    RDB_MAKE_ME_SERIALIZABLE_1(uuid_);

    explicit order_bucket_t(uuid_u uuid) : uuid_(uuid) { }
};



bool operator==(const order_bucket_t &a, const order_bucket_t &b);
bool operator!=(const order_bucket_t &a, const order_bucket_t &b);
bool operator<(const order_bucket_t &a, const order_bucket_t &b);


/* Order tokens of the same bucket need to arrive at order sinks in a
   certain order.  Pretending that read_mode is always false, they
   need to arrive by ascending order of their value -- the same order
   that they were created in.  This is because write operations cannot
   be reordered.  However, read operations may be shuffled around, and
   so may order_tokens with read_mode set to true.
 */
class order_token_t {
public:
    static const order_token_t ignore;

#ifndef NDEBUG
    // By default we construct a totally invalid order token, not
    // equal to ignore, that must be initialized.
    order_token_t();
    order_token_t with_read_mode() const;
    void assert_read_mode() const;
    void assert_write_mode() const;
    const std::string &tag() const;
#else
    order_token_t() { }
    order_token_t with_read_mode() const { return order_token_t(); }
    void assert_read_mode() const { }
    void assert_write_mode() const { }
    std::string tag() const { return ""; }
#endif  // ifndef NDEBUG

private:
#ifndef NDEBUG
    order_token_t(order_bucket_t bucket, int64_t x, bool read_mode, const std::string &tag);

    bool is_invalid() const;
    bool is_ignore() const;

    // TODO: Make these fields const.
    order_bucket_t bucket_;
    bool read_mode_;
    int64_t value_;
    // This tag would be inefficient on VC++ or some other non-GNU
    // std::string implementation, since we copy by value.
    std::string tag_;

    RDB_DECLARE_ME_SERIALIZABLE;
#else
    RDB_MAKE_ME_SERIALIZABLE_0();
#endif  // ifndef NDEBUG

    friend class order_source_t;
    friend class order_sink_t;
    friend class order_checkpoint_t;
    friend class plain_sink_t;
};


/* Order sources create order tokens with increasing values for a
   specific bucket. */
class order_source_t : public home_thread_mixin_debug_only_t {
public:
#ifndef NDEBUG
    order_source_t();
    explicit order_source_t(threadnum_t specified_home_thread);
    ~order_source_t();

    // Makes a write-mode order token.
    order_token_t check_in(const std::string &tag);
#else
    order_source_t() { }
    explicit order_source_t(threadnum_t specified_home_thread) : home_thread_mixin_debug_only_t(specified_home_thread) { }
    ~order_source_t() { }

    order_token_t check_in(const std::string&) { return order_token_t(); }
#endif  // ndef NDEBUG

private:
#ifndef NDEBUG
    order_bucket_t bucket_;
    int64_t counter_;
#endif  // ifndef NDEBUG

    DISABLE_COPYING(order_source_t);
};

struct tagged_seen_t {
    int64_t value;
    std::string tag;

    tagged_seen_t(int64_t _value, const std::string &_tag) : value(_value), tag(_tag) { }
};

/* Eventually order tokens get to an order sink, and those of the same
   bucket had better arrive in the right order. */
class order_sink_t : public home_thread_mixin_debug_only_t {
public:
#ifndef NDEBUG
    order_sink_t();

    void check_out(order_token_t token);
#else
    order_sink_t() { }

    void check_out(UNUSED  order_token_t token) { }

#endif  // ifndef NDEBUG

private:

#ifndef NDEBUG
    friend class plain_sink_t;
    static void verify_token_value_and_update(order_token_t token, std::pair<tagged_seen_t, tagged_seen_t> *ls_pair);

    // We keep two last seen values because reads can be reordered.
    // .first = last seen write, .second = max(last seen read, last seen write)
    typedef std::map<order_bucket_t, std::pair<tagged_seen_t, tagged_seen_t> > last_seens_map_t;
    last_seens_map_t last_seens_;
#endif  // ifndef NDEBUG

    DISABLE_COPYING(order_sink_t);
};


// An order sink with less overhead, for situations where there is
// only one source (like the top of a btree slice) and many sinks.  If
// there's one bucket there's no point in instantiating a `std::map`.
// TODO: Is a `std::map` of one item really expensive enough to justify
// having a separate type?
class plain_sink_t : public home_thread_mixin_debug_only_t {
public:
#ifndef NDEBUG
    plain_sink_t();

    void check_out(order_token_t token);
#else
    plain_sink_t() { }

    void check_out(UNUSED order_token_t token) { }
#endif  // NDEBUG

private:
#ifndef NDEBUG
    // The pair of last seen values.
    std::pair<tagged_seen_t, tagged_seen_t> ls_pair_;

    /* If `have_bucket_`, then `bucket_` is the bucket that we are associated with.
    If we get an order token from a different bucket we crap out. */
    bool have_bucket_;
    order_bucket_t bucket_;
#endif  // NDEBUG
};


// `order_checkpoint_t` is an `order_sink_t` plus an `order_source_t`.
class order_checkpoint_t : public home_thread_mixin_debug_only_t {
public:
    order_checkpoint_t() { }
#ifndef NDEBUG
    explicit order_checkpoint_t(const std::string &tagappend) : tagappend_(tagappend) { }
    void set_tagappend(const std::string &tagappend);
    order_token_t check_through(order_token_t token);
    order_token_t checkpoint_raw_check_in();
#else
    explicit order_checkpoint_t(UNUSED const std::string &tagappend) { }
    void set_tagappend(UNUSED const std::string &tagappend) { }
    order_token_t check_through(UNUSED order_token_t token) { return order_token_t(); }
    order_token_t checkpoint_raw_check_in() { return order_token_t(); }
#endif  // ndef NDEBUG

private:
#ifndef NDEBUG
    order_sink_t sink_;
    order_source_t source_;
    std::string tagappend_;
#endif
};


#endif  // CONCURRENCY_FIFO_CHECKER_HPP_
