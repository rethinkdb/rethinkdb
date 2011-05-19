#include "concurrency/fifo_checker.hpp"

const order_token_t order_token_t::ignore(-1, -1, false);

order_token_t::order_token_t() : bucket_(-2), value_(-2) { }

order_token_t::order_token_t(int bucket, int64_t x, bool read_mode)
    : bucket_(bucket), read_mode_(read_mode), value_(x) { }

order_token_t order_token_t::with_read_mode() const {
    return order_token_t(bucket_, value_, true);
}

int order_token_t::bucket() const { return bucket_; }
bool order_token_t::read_mode() const { return read_mode_; }
int64_t order_token_t::value() const { return value_; }



order_source_pigeoncoop_t::order_source_pigeoncoop_t(int starting_bucket)
    : least_unregistered_bucket_(starting_bucket) { }

void order_source_pigeoncoop_t::nop() { }

void order_source_pigeoncoop_t::unregister_bucket(int bucket, int64_t counter) {
    ASSERT_NO_CORO_WAITING;
    rassert(bucket < least_unregistered_bucket_);
    free_buckets_.push_back(std::make_pair(bucket, counter));
}

std::pair<int, int64_t> order_source_pigeoncoop_t::register_for_bucket() {
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


order_source_t::order_source_t(int bucket, boost::function<void()> unregisterator)
    : bucket_(bucket), counter_(0), unregister_(unregisterator) { }

order_source_t::order_source_t(order_source_pigeoncoop_t *coop) : bucket_(-2), counter_(-2) {
    std::pair<int, int64_t> p = coop->register_for_bucket();
    bucket_ = p.first;
    counter_ = p.second;
    unregister_ = boost::bind(&order_source_pigeoncoop_t::unregister_bucket, coop, bucket_, boost::ref(counter_));
}

order_source_t::~order_source_t() { unregister_(); }

order_token_t order_source_t::check_in() {
    ++counter_;
    return order_token_t(bucket_, counter_, false);
}





const int64_t REALTIME_COUNTER_INCREMENT = 0x100000000LL;

backfill_receiver_order_source_t::backfill_receiver_order_source_t(int bucket)
    : bucket_(bucket), counter_(0), backfill_active_(false) { }

void backfill_receiver_order_source_t::backfill_begun() {
    // TODO: We have no way of calling this function.
    rassert(!backfill_active_);
    backfill_active_ = true;
}

void backfill_receiver_order_source_t::backfill_done() {
    // TODO: We can't really assert backfill_active stuff here because
    // we might not have received the backfilling messages.
    backfill_active_ = true;

    rassert(backfill_active_);
    backfill_active_ = false;
    counter_ += REALTIME_COUNTER_INCREMENT;
}

order_token_t backfill_receiver_order_source_t::check_in_backfill_operation() {
    backfill_active_ = true;
    rassert(backfill_active_);
    ++counter_;
    return order_token_t(bucket_, counter_, false);
}

order_token_t backfill_receiver_order_source_t::check_in_realtime_operation() {
    ++counter_;
    return order_token_t(bucket_, counter_ + (backfill_active_ ? REALTIME_COUNTER_INCREMENT : 0), false);
}




order_sink_t::order_sink_t() { }

void order_sink_t::check_out(order_token_t token) {
    if (token.bucket_ != order_token_t::ignore.bucket_) {
        rassert(token.bucket_ >= 0);
        if (token.bucket_ >= int(last_seens_.size())) {
            last_seens_.resize(token.bucket_ + 1, std::pair<int64_t, int64_t>(0, 0));
        }

        verify_token_value_and_update(token, &last_seens_[token.bucket_]);
    }
}

void order_sink_t::verify_token_value_and_update(order_token_t token, std::pair<int64_t, int64_t> *ls_pair) {
    // We tolerate equality in this comparison because it can be used
    // to ensure that multiple actions don't get interrupted.  And
    // resending the same action isn't normally a problem.
    if (token.read_mode_) {
        rassert(token.value_ >= ls_pair->first, "token.value_ = %ld, last_seens_[token.bucket_].first = %ld, token.bucket_ = %d", token.value_, ls_pair->first, token.bucket_);
        ls_pair->second = std::max(ls_pair->second, token.value_);
    } else {
        rassert(token.value_ >= ls_pair->second, "token.value_ = %ld, last_seens_[token.bucket_].second = %ld, token.bucket_ = %d", token.value_, ls_pair->second, token.bucket_);
        ls_pair->first = ls_pair->second = token.value_;
    }
}


plain_sink_t::plain_sink_t() : ls_pair_(0, 0) { }

void plain_sink_t::check_out(order_token_t token) {
    if (token.bucket_ != order_token_t::ignore.bucket_) {
        rassert(token.bucket_ >= 0);
        rassert(token.bucket_ == 0, "Only bucket 0 allowed, you made a programmer error");

        order_sink_t::verify_token_value_and_update(token, &ls_pair_);
    }
}
