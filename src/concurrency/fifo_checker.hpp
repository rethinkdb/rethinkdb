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
const int SLAVE_ORDER_SOURCE_BUCKET = 0;
const int MASTER_ORDER_SOURCE_BUCKET = 0;
const int MEMCACHE_START_BUCKET = 1;

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

class order_source_pigeoncoop_t {
public:
    order_source_pigeoncoop_t(int starting_bucket = 0)
        : least_unregistered_bucket_(starting_bucket) { }

    friend class order_source_t;
private:
    static void nop() { }

    void unregister_bucket(int bucket) {
        ASSERT_NO_CORO_WAITING;
        rassert(bucket < least_unregistered_bucket_);
        free_buckets_.push_back(bucket);
    }

    int register_for_bucket() {
        ASSERT_NO_CORO_WAITING;
        if (free_buckets_.empty()) {
            int ret = least_unregistered_bucket_;
            ++ least_unregistered_bucket_;
            return ret;
        } else {
            int ret = free_buckets_.back();
            rassert(ret < least_unregistered_bucket_);
            free_buckets_.pop_back();
            return ret;
        }
    }

    // The bucket we should use next, if free_buckets_ is empty.
    int least_unregistered_bucket_;

    // The buckets less than least_unregistered_bucket_.
    std::vector<int> free_buckets_;

    DISABLE_COPYING(order_source_pigeoncoop_t);
};

class order_source_t {
public:
    order_source_t(int bucket = 0, boost::function<void()> unregisterator = order_source_pigeoncoop_t::nop)
        : bucket_(bucket), counter_(0), unregister_(unregisterator) { }

    order_source_t(order_source_pigeoncoop_t *coop)
        : bucket_(coop->register_for_bucket()), counter_(0),
          unregister_(boost::bind(&order_source_pigeoncoop_t::unregister_bucket, coop, bucket_)) { }

    ~order_source_t() { unregister_(); }

    order_token_t check_in() { return order_token_t(bucket_, ++counter_); }

private:
    int bucket_;
    int64_t counter_;
    boost::function<void ()> unregister_;

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
            // interrupted.  And resending the same action isn't
            // normally a problem.
            rassert(token.value_ >= last_seens_[token.bucket_], "token.value_ = %ld, last_seens_[token.bucket_] = %ld", token.value_, last_seens_[token.bucket_]);
            last_seens_[token.bucket_] = token.value_;
        }
    }

private:
    std::vector<int64_t> last_seens_;

    DISABLE_COPYING(order_sink_t);
};

#endif  // __CONCURRENCY_FIFO_CHECKER_HPP__
