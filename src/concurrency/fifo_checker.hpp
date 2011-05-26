#ifndef __CONCURRENCY_FIFO_CHECKER_HPP__
#define __CONCURRENCY_FIFO_CHECKER_HPP__

#include <vector>

#include <boost/function.hpp>

#include "arch/arch.hpp"
#include "utils2.hpp"


struct order_bucket_t {
    order_bucket_t(int thread, int number) :
        thread_(thread), number_(number)
        { }
    order_bucket_t() { }
    int thread_;
    int number_;
    bool valid();
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

    // By default we construct a totally invalid order token, not
    // equal to ignore, that must be initialized.
    order_token_t();
    order_token_t with_read_mode() const;
    bool read_mode() const;

private:
    order_token_t(order_bucket_t bucket, int64_t x, bool read_mode);
    order_bucket_t bucket_;
    bool read_mode_;
    int64_t value_;

    friend class order_source_t;
    friend class order_sink_t;
    friend class backfill_receiver_order_source_t;
    friend class plain_sink_t;
};


/* Order sources create order tokens with increasing values for a
   specific bucket.  When they are destroyed they call a void()
   function which might inform somebody that the bucket is now
   available for reuse. */
class order_source_t : public home_thread_mixin_t {
public:
    order_source_t();
    ~order_source_t();

    order_token_t check_in();

private:
    order_bucket_t bucket_;
    int64_t counter_;

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
class backfill_receiver_order_source_t : public home_thread_mixin_t {
public:
    backfill_receiver_order_source_t();
    ~backfill_receiver_order_source_t();

    void backfill_begun();
    void backfill_done();

    order_token_t check_in_backfill_operation();
    order_token_t check_in_realtime_operation();

private:
    order_bucket_t bucket_;
    int64_t counter_;
    bool backfill_active_;

    DISABLE_COPYING(backfill_receiver_order_source_t);
};


/* Eventually order tokens get to an order sink, and those of the same
   bucket had better arrive in the right order. */
class order_sink_t : public home_thread_mixin_t {
public:
    order_sink_t();

    void check_out(order_token_t token);

private:

    friend class plain_sink_t;
    static void verify_token_value_and_update(order_token_t token, std::pair<int64_t, int64_t> *ls_pair);

    // We keep two last seen values because reads can be reordered.
    // .first = last seen write, .second = max(last seen read, last seen write)
    typedef std::map<order_bucket_t, std::pair<int64_t, int64_t> > last_seens_map_t;
    last_seens_map_t last_seens_;

    DISABLE_COPYING(order_sink_t);
};


// An order sink with less overhead, for situations where there is
// only one source (like the top of a btree slice) and many sinks.  If
// there's one bucket there's no point in instantiating a std::map.
class plain_sink_t : public home_thread_mixin_t {
public:
    plain_sink_t();

    void check_out(order_token_t token);

private:
    // The pair of last seen values.
    std::pair<int64_t, int64_t> ls_pair_;

    /* If `have_bucket_`, then `bucket_` is the bucket that we are associated with.
    If we get an order token from a different bucket we crap out. */
    bool have_bucket_;
    order_bucket_t bucket_;

    DISABLE_COPYING(plain_sink_t);
};

#endif  // __CONCURRENCY_FIFO_CHECKER_HPP__
