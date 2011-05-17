#ifndef __CONCURRENCY_FIFO_CHECKER_HPP__
#define __CONCURRENCY_FIFO_CHECKER_HPP__

#include <vector>

#include <boost/function.hpp>

#include "arch/arch.hpp"
#include "utils2.hpp"

// The memcached order_source_t and the backfill_receiver_t will want
// to be in distinct buckets.

// The master and slave never happen in the same process.  Then the
// memcache buckets have to be different.
// TODO: get rid of SLAVE_... and MASTER_... and just use BACKFILL_RECEIVER_...
const int SLAVE_ORDER_SOURCE_BUCKET = 0;
const int MASTER_ORDER_SOURCE_BUCKET = 0;
const int BACKFILL_RECEIVER_ORDER_SOURCE_BUCKET = 0;
const int MEMCACHE_START_BUCKET = 1;

class order_token_t {
public:
    static const order_token_t ignore;

    // By default we construct a totally invalid order token, not
    // equal to ignore, that must be initialized.
    order_token_t();

    order_token_t with_read_mode() const;

    int bucket() const;
    bool read_mode() const;
    int64_t value() const;

private:
    order_token_t(int bucket, int64_t x, bool read_mode);
    int bucket_;
    bool read_mode_;
    int64_t value_;

    friend class order_source_t;
    friend class order_sink_t;
    friend class backfill_receiver_order_source_t;
    friend class plain_sink_t;
    friend class contiguous_order_sink_t;
};

class order_source_pigeoncoop_t {
public:
    order_source_pigeoncoop_t(int starting_bucket = 0);

    friend class order_source_t;
private:
    static void nop();

    void unregister_bucket(int bucket, int64_t counter);

    std::pair<int, int64_t> register_for_bucket();

    // The bucket we should use next, if free_buckets_ is empty.
    int least_unregistered_bucket_;

    // The buckets less than least_unregistered_bucket_.
    std::vector<std::pair<int, int64_t> > free_buckets_;

    DISABLE_COPYING(order_source_pigeoncoop_t);
};

class order_source_t {
public:
    order_source_t(int bucket = 0, boost::function<void()> unregisterator = order_source_pigeoncoop_t::nop);
    order_source_t(order_source_pigeoncoop_t *coop);
    ~order_source_t();

    order_token_t check_in();

private:
    int bucket_;
    int64_t counter_;
    boost::function<void ()> unregister_;

    DISABLE_COPYING(order_source_t);
};


class backfill_receiver_order_source_t {
public:
    backfill_receiver_order_source_t(int bucket = BACKFILL_RECEIVER_ORDER_SOURCE_BUCKET);

    void backfill_begun();
    void backfill_done();

    order_token_t check_in_backfill_operation();
    order_token_t check_in_realtime_operation();

private:
    int bucket_;
    int64_t counter_;
    bool backfill_active_;

    DISABLE_COPYING(backfill_receiver_order_source_t);
};


class order_sink_t {
public:
    order_sink_t();

    void check_out(order_token_t token);

private:

    friend class plain_sink_t;
    static void verify_token_value_and_update(order_token_t token, std::pair<int64_t, int64_t> *ls_pair);

    // .first = last seen write, .second = max(last seen read, last seen write)
    std::vector<std::pair<int64_t, int64_t> > last_seens_;

    DISABLE_COPYING(order_sink_t);
};

// An order sink with less overhead, for situations where there is
// only one source (like the top of a btree slice) and many sinks.
class plain_sink_t {
public:
    plain_sink_t();

    void check_out(order_token_t token);

private:
    std::pair<int64_t, int64_t> ls_pair_;

    DISABLE_COPYING(plain_sink_t);
};

#endif  // __CONCURRENCY_FIFO_CHECKER_HPP__
