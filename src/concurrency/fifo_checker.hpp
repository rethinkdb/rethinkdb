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

    order_token_t with_read_mode() {
        return order_token_t(bucket_, value_, true);
    }

    int bucket() const { return bucket_; }
    bool read_mode() const { return read_mode_; }
    int64_t value() const { return value_; }

private:
    explicit order_token_t(int bucket, int64_t x, bool read_mode)
        : bucket_(bucket), read_mode_(read_mode), value_(x) { }
    int bucket_;
    bool read_mode_;
    int64_t value_;

    friend class order_source_t;
    friend class order_sink_t;
    friend class plain_sink_t;
    friend class contiguous_order_sink_t;
};

class order_source_pigeoncoop_t {
public:
    order_source_pigeoncoop_t(int starting_bucket = 0)
        : least_unregistered_bucket_(starting_bucket) { }

    friend class order_source_t;
private:
    static void nop() { }

    void unregister_bucket(int bucket, int64_t counter) {
        ASSERT_NO_CORO_WAITING;
        rassert(bucket < least_unregistered_bucket_);
        free_buckets_.push_back(std::make_pair(bucket, counter));
    }

    std::pair<int, int64_t> register_for_bucket() {
        ASSERT_NO_CORO_WAITING;
        if (free_buckets_.empty()) {
            int ret = least_unregistered_bucket_;
            ++ least_unregistered_bucket_;
            return std::pair<int, int64_t>(ret, 0);
        } else {
            std::pair<int, int64_t> ret = free_buckets_.back();
            rassert(ret.first < least_unregistered_bucket_);
            rassert(ret.second >= 0);
            free_buckets_.pop_back();
            return ret;
        }
    }

    // The bucket we should use next, if free_buckets_ is empty.
    int least_unregistered_bucket_;

    // The buckets less than least_unregistered_bucket_.
    std::vector<std::pair<int, int64_t> > free_buckets_;

    DISABLE_COPYING(order_source_pigeoncoop_t);
};

class order_source_t {
public:
    order_source_t(int bucket = 0, boost::function<void()> unregisterator = order_source_pigeoncoop_t::nop)
        : bucket_(bucket), counter_(0), unregister_(unregisterator) { }

    order_source_t(order_source_pigeoncoop_t *coop) : bucket_(-2), counter_(-2) {
        std::pair<int, int64_t> p = coop->register_for_bucket();
        bucket_ = p.first;
        counter_ = p.second;
        unregister_ = boost::bind(&order_source_pigeoncoop_t::unregister_bucket, coop, bucket_, boost::ref(counter_));
    }

    ~order_source_t() { unregister_(); }

    order_token_t check_in() { return order_token_t(bucket_, ++counter_, false); }

    order_token_t check_in_read_mode() { return order_token_t(bucket_, ++counter_, true); }

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
                last_seens_.resize(token.bucket_ + 1, std::pair<int64_t, int64_t>(0, 0));
            }

            verify_token_value_and_update(token, &last_seens_[token.bucket_]);
        }
    }

private:

    friend class plain_sink_t;
    static void verify_token_value_and_update(order_token_t token, std::pair<int64_t, int64_t> *ls_pair) {
        // We tolerate equality in this comparison because it can
        // be used to ensure that multiple actions don't get
        // interrupted.  And resending the same action isn't
        // normally a problem.
        if (token.read_mode_) {
            rassert(token.value_ >= ls_pair->first, "token.value_ = %ld, last_seens_[token.bucket_].first = %ld, token.bucket_ = %d", token.value_, ls_pair->first, token.bucket_);
            ls_pair->second = std::max(ls_pair->second, token.value_);
        } else {
            rassert(token.value_ >= ls_pair->second, "token.value_ = %ld, last_seens_[token.bucket_].second = %ld, token.bucket_ = %d", token.value_, ls_pair->second, token.bucket_);
            ls_pair->first = ls_pair->second = token.value_;
        }
    }

    // .first = last seen write, .second = max(last seen read, last seen write)
    std::vector<std::pair<int64_t, int64_t> > last_seens_;

    DISABLE_COPYING(order_sink_t);
};

// An order sink with less overhead, for situations where there is
// only one source (like the top of a btree slice) and many sinks.
class plain_sink_t {
public:
    plain_sink_t() : ls_pair_(0, 0) { }

    void check_out(order_token_t token) {
        if (token.bucket_ != order_token_t::ignore.bucket_) {
            // TODO: do read/write mode.
            rassert(token.bucket_ >= 0);
            rassert(token.bucket_ == 0, "Only bucket 0 allowed, you made a programmer error");

            order_sink_t::verify_token_value_and_update(token, &ls_pair_);
        }
    }

private:
    std::pair<int64_t, int64_t> ls_pair_;

    DISABLE_COPYING(plain_sink_t);
};

#endif  // __CONCURRENCY_FIFO_CHECKER_HPP__
