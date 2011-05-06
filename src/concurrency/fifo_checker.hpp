#ifndef __CONCURRENCY_FIFO_CHECKER_HPP__
#define __CONCURRENCY_FIFO_CHECKER_HPP__

#include <vector>

#include "utils2.hpp"

class order_token_t {
public:
    static const order_token_t ignore;
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
    order_sink_t(int num_buckets = 1) : last_seens_(num_buckets, 0) { }

    void check_out(order_token_t token) {
        if (token.bucket_ != order_token_t::ignore.bucket_) {
            rassert(token.bucket_ >= 0);
            rassert(token.bucket_ < int(last_seens_.size()));
            rassert(token.value_ > last_seens_[token.bucket_]);
            last_seens_[token.bucket_] = token.value_;
        }
    }

private:
    std::vector<int64_t> last_seens_;

    DISABLE_COPYING(order_sink_t);
};

#endif  // __CONCURRENCY_FIFO_CHECKER_HPP__
