#ifndef __CONCURRENCY_FIFO_CHECKER_HPP__
#define __CONCURRENCY_FIFO_CHECKER_HPP__

#include <vector>

#include <boost/function.hpp>

#include "arch/arch.hpp"
#include "utils2.hpp"

// The memcached order_source_t and the backfill_receiver_t will want
// to be in distinct buckets.

// We'll need our order sources' buckets (operating for the same code
// path) at any given point in time to be distinct values. The
// backfill receiver (be it on the master side or slave side) gets
// bucket 0.  Then the memcache connections get buckets 1,2,3,...
//
// Btree slice operations then get an order source with bucket 0,
// since they operate independently and their tokens reach a different
// set of order sinks.
const int BACKFILL_RECEIVER_ORDER_SOURCE_BUCKET = 0;
const int MEMCACHE_START_BUCKET = 1;

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

/* Buckets are like file descriptors.  We don't want two order sources
   to have the same bucket, and we can reuse buckets when an order
   source is destroyed.  A pigeoncoop keeps track of all the available
   pigeonholes and what the initial value should be for the pigeon, I
   mean order source, that next gets assigned a given bucket. */
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

    // The buckets less than least_unregistered_bucket_ that are free.
    std::vector<std::pair<int, int64_t> > free_buckets_;

    DISABLE_COPYING(order_source_pigeoncoop_t);
};

/* Order sources create order tokens with increasing values for a
   specific bucket.  When they are destroyed they call a void()
   function which might inform somebody that the bucket is now
   available for reuse. */
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

/* A backfill receiver order source is a bit special, because when we
   backfill, we want backfill operations to get in before "realtime"
   operations.  So we play tricks with the counter depending on
   whether we're currently backfilling and whether we're checking in a
   backfill or a realtime operation.  (This assumes there will be no
   more than 4 billion backfill operations, which is ok for
   debugging purposes.)
 */
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

/* Eventually order tokens get to an order sink, and those of the same
   bucket had better arrive in the right order. */
class order_sink_t {
public:
    order_sink_t();

    void check_out(order_token_t token);

private:

    friend class plain_sink_t;
    static void verify_token_value_and_update(order_token_t token, std::pair<int64_t, int64_t> *ls_pair);

    // We keep two last seen values because reads can be reordered.
    // .first = last seen write, .second = max(last seen read, last seen write)
    std::vector<std::pair<int64_t, int64_t> > last_seens_;

    DISABLE_COPYING(order_sink_t);
};

// An order sink with less overhead, for situations where there is
// only one source (like the top of a btree slice) and many sinks.  If
// there's one bucket there's no point in instantiating a std::vector.
class plain_sink_t {
public:
    plain_sink_t();

    void check_out(order_token_t token);

private:
    // The pair of last seen values.
    std::pair<int64_t, int64_t> ls_pair_;

    DISABLE_COPYING(plain_sink_t);
};

#endif  // __CONCURRENCY_FIFO_CHECKER_HPP__
