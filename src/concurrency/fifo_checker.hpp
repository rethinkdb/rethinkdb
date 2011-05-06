#ifndef __CONCURRENCY_FIFO_CHECKER_HPP__
#define __CONCURRENCY_FIFO_CHECKER_HPP__

#include "utils2.hpp"

class order_token_t {
    explicit order_token_t(int64_t x) : value_(x) { }
    int64_t value_;

    friend class order_source_t;
    friend class order_sink_t;
    friend class contiguous_order_sink_t;
};

class order_source_t {
public:
    order_source_t() : counter_(0) { }

    order_token_t check_in() { return order_token_t(++counter_); }

private:
    int64_t counter_;

    DISABLE_COPYING(order_source_t);
};

class order_sink_t {
public:
    order_sink_t() : last_seen_(0) { }

    void check_out(order_token_t token) {
        rassert(token.value_ > last_seen_);
        last_seen_ = token.value_;
    }

private:
    int64_t last_seen_;

    DISABLE_COPYING(order_sink_t);
};


class contiguous_order_sink_t {
public:
    contiguous_order_sink_t() : last_seen_(0) { }

    void check_out(order_token_t token) {
        rassert(token.value_ == last_seen_ + 1);
        last_seen_ = token.value_;
    }

private:
    int64_t last_seen_;

    DISABLE_COPYING(contiguous_order_sink_t);
};

#endif  // __CONCURRENCY_FIFO_CHECKER_HPP__
