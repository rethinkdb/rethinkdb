#ifndef __CONCURRENCY_FIFO_CHECKER_HPP__
#define __CONCURRENCY_FIFO_CHECKER_HPP__

#include <vector>

#include "utils2.hpp"

// The memcached order_source_t and the backfill_receiver_t will want
// to be in distinct buckets.
const int BACKFILL_RECEIVER_BUCKET = 0;

class order_token_t {
public:
    static const order_token_t ignore;

    // By default we construct a totally invalid order token, not
    // equal to ignore, that must be initialized.
    order_token_t() : bucket_(-2), value_(-2) { }

private:
    explicit order_token_t(int bucket, int64_t x) : bucket_(bucket), value_(x) { }
    int bucket_;
    int64_t value_;

    friend class order_source_t;
    friend class order_sink_t;
    friend class contiguous_order_sink_t;
};

class order_source_t {
public:
    order_source_t(int bucket = 0) : bucket_(bucket), counter_(0) { }

    order_token_t check_in() { return order_token_t(bucket_, ++counter_); }

private:
    int bucket_;
    int64_t counter_;

    DISABLE_COPYING(order_source_t);
};

class order_sink_t {
public:
    order_sink_t() { }

    void check_out(order_token_t token) {
        if (token.bucket_ != order_token_t::ignore.bucket_) {
            rassert(token.bucket_ >= 0);
            if (token.bucket_ >= int(last_seens_.size())) {
                last_seens_.resize(token.bucket_ + 1, 0);
            }

            // We tolerate equality in this comparison because it can
            // be used to ensure that multiple actions don't get
            // interrupted.
            rassert(token.value_ >= last_seens_[token.bucket_]);
            last_seens_[token.bucket_] = token.value_;
        }
    }

private:
    std::vector<int64_t> last_seens_;

    DISABLE_COPYING(order_sink_t);
};

#endif  // __CONCURRENCY_FIFO_CHECKER_HPP__
