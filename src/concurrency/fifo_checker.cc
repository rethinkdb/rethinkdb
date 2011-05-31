#include "concurrency/fifo_checker.hpp"

#ifndef NDEBUG

#define ORDER_INVALID (-2)
#define ORDER_INVALID (-2)

#define ORDER_IGNORE (-1)
#define ORDER_IGNORE (-1)

const order_token_t order_token_t::ignore(order_bucket_t(ORDER_IGNORE, ORDER_IGNORE), ORDER_IGNORE, false);
#else
const order_token_t order_token_t::ignore;
#endif  // ifndef NDEBUG



#ifndef NDEBUG

bool operator==(const order_bucket_t &a, const order_bucket_t &b) {
    return a.thread_ == b.thread_ && a.number_ == b.number_;
}

bool operator!=(const order_bucket_t &a, const order_bucket_t &b) {
    return !(a == b);
}

bool operator<(const order_bucket_t &a, const order_bucket_t &b) {
    if (a.thread_ < b.thread_) {
        return true;
    } else if (a.thread_ == b.thread_) {
        if (a.number_ < b.number_) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool order_bucket_t::valid() {
    return (thread_ >= 0 && number_ >= 0);
}



order_token_t::order_token_t() : bucket_(ORDER_INVALID, ORDER_INVALID), value_(ORDER_INVALID) { }

order_token_t::order_token_t(order_bucket_t bucket, int64_t x, bool read_mode)
    : bucket_(bucket), read_mode_(read_mode), value_(x) { }

order_token_t order_token_t::with_read_mode() const {
    return order_token_t(bucket_, value_, true);
}

bool order_token_t::read_mode() const { return read_mode_; }



/* `order_source_pigeoncoop_t` is used to assign unique bucket numbers to buckets. There
is one per thread; they don't know about each other. */

struct order_source_pigeoncoop_t {

    order_source_pigeoncoop_t() : least_unregistered_bucket_(0) { }

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

    // The buckets less than least_unregistered_bucket_ that are free.
    std::vector<std::pair<int, int64_t> > free_buckets_;

    DISABLE_COPYING(order_source_pigeoncoop_t);
};

order_source_pigeoncoop_t pigeoncoops[MAX_THREADS];

order_source_t::order_source_t() {
    std::pair<int, int64_t> p = pigeoncoops[get_thread_id()].register_for_bucket();
    bucket_ = order_bucket_t(get_thread_id(), p.first);
    counter_ = p.second;
}

order_source_t::~order_source_t() {
    pigeoncoops[get_thread_id()].unregister_bucket(bucket_.number_, counter_);
}

order_token_t order_source_t::check_in() {
    assert_thread();
    ++counter_;
    return order_token_t(bucket_, counter_, false);
}



const int64_t REALTIME_COUNTER_INCREMENT = 0x100000000LL;

backfill_receiver_order_source_t::backfill_receiver_order_source_t() :
    backfill_active_(false)
{
    std::pair<int, int64_t> p = pigeoncoops[get_thread_id()].register_for_bucket();
    bucket_ = order_bucket_t(get_thread_id(), p.first);
    counter_ = p.second;
}

backfill_receiver_order_source_t::~backfill_receiver_order_source_t() {
    pigeoncoops[get_thread_id()].unregister_bucket(
        bucket_.number_,
        /* We have to pass the pigeoncoop a number that's greater than or equal
        to any number that we've given out before. If `backfill_active_` is true,
        then we may have given out some tokens which had numbers of the form
        `(counter_ + REALTIME_COUNTER_INCREMENT)`, so we need to take that into
        account. */
        counter_ + (backfill_active_ ? REALTIME_COUNTER_INCREMENT : 0));
}

void backfill_receiver_order_source_t::backfill_begun() {
    assert_thread();
    // TODO: We have no way of calling this function.
    rassert(!backfill_active_);
    backfill_active_ = true;
}

void backfill_receiver_order_source_t::backfill_done() {
    assert_thread();
    // TODO: We can't really assert backfill_active stuff here because
    // we might not have received the backfilling messages.
    backfill_active_ = true;

    rassert(backfill_active_);
    backfill_active_ = false;
    counter_ += REALTIME_COUNTER_INCREMENT;
}

order_token_t backfill_receiver_order_source_t::check_in_backfill_operation() {
    assert_thread();
    backfill_active_ = true;
    rassert(backfill_active_);
    ++counter_;
    return order_token_t(bucket_, counter_, false);
}

order_token_t backfill_receiver_order_source_t::check_in_realtime_operation() {
    assert_thread();
    ++counter_;
    return order_token_t(bucket_, counter_ + (backfill_active_ ? REALTIME_COUNTER_INCREMENT : 0), false);
}



order_sink_t::order_sink_t() { }

void order_sink_t::check_out(order_token_t token) {
    assert_thread();
    if (token.bucket_ != order_token_t::ignore.bucket_) {
        rassert(token.bucket_.valid());
        /* If we haven't seen this bucket before, then this will
        make a new entry in the map and fill it with a pair of (0, 0).
        Either way, `last_seen` will be a pointer to the `std::pair`
        that ends up in the map. */
        last_seens_map_t::iterator it = last_seens_.insert(
            last_seens_map_t::value_type(token.bucket_, std::pair<int64_t, int64_t>(0, 0))
            ).first;
        std::pair<int64_t, int64_t> *last_seen = &(*it).second;
        verify_token_value_and_update(token, last_seen);
    }
}

void order_sink_t::verify_token_value_and_update(order_token_t token, std::pair<int64_t, int64_t> *ls_pair) {
    // We tolerate equality in this comparison because it can be used
    // to ensure that multiple actions don't get interrupted.  And
    // resending the same action isn't normally a problem.
    if (token.read_mode_) {
        rassert(token.value_ >= ls_pair->first, "token.value_ = %ld, last_seens_[token.bucket_].first = %ld, token.bucket_ = (%d,%d)", token.value_, ls_pair->first, token.bucket_.thread_, token.bucket_.number_);
        ls_pair->second = std::max(ls_pair->second, token.value_);
    } else {
        rassert(token.value_ >= ls_pair->second, "token.value_ = %ld, last_seens_[token.bucket_].second = %ld, token.bucket_ = (%d,%d)", token.value_, ls_pair->second, token.bucket_.thread_, token.bucket_.number_);
        ls_pair->first = ls_pair->second = token.value_;
    }
}



plain_sink_t::plain_sink_t() : ls_pair_(0, 0), have_bucket_(false) { }

void plain_sink_t::check_out(order_token_t token) {
    assert_thread();
    if (token.bucket_ != order_token_t::ignore.bucket_) {
        rassert(token.bucket_.valid());
        if (!have_bucket_) {
            bucket_ = token.bucket_;
            have_bucket_ = true;
        } else {
            rassert(token.bucket_ == bucket_, "plain_sink_t can only be used with one order_source_t.");
        }

        order_sink_t::verify_token_value_and_update(token, &ls_pair_);
    }
}



order_token_t order_checkpoint_t::check_through(order_token_t tok) {
    assert_thread();
    sink_.check_out(tok);
    order_token_t tok2 = source_.check_in();
    if (tok.read_mode()) tok2 = tok2.with_read_mode();
    return tok2;
}

#endif  // ifndef NDEBUG
