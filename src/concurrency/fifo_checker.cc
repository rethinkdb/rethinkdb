// Copyright 2010-2013 RethinkDB, all rights reserved.
#define __STDC_FORMAT_MACROS
#include "concurrency/fifo_checker.hpp"

#include <vector>

#include "arch/runtime/runtime.hpp"
#include "config/args.hpp"
#include "containers/archive/stl_types.hpp"

#ifndef NDEBUG

#define ORDER_INVALID (-2)
#define ORDER_IGNORE (-1)

const order_token_t order_token_t::ignore(order_bucket_t::invalid(), ORDER_IGNORE, false, "order_token_t::ignore");
#else
const order_token_t order_token_t::ignore;
#endif  // ifndef NDEBUG



#ifndef NDEBUG

bool operator==(const order_bucket_t &a, const order_bucket_t &b) {
    return a.uuid_ == b.uuid_;
}

bool operator!=(const order_bucket_t &a, const order_bucket_t &b) {
    return !(a == b);
}

bool operator<(const order_bucket_t &a, const order_bucket_t &b) {
    return a.uuid_ < b.uuid_;
}

bool order_bucket_t::valid() const {
    return !uuid_.is_nil();
}



order_token_t::order_token_t() : bucket_(order_bucket_t::invalid()), value_(ORDER_INVALID) { }

order_token_t::order_token_t(order_bucket_t bucket, int64_t x, bool read_mode, const std::string &tag)
    : bucket_(bucket), read_mode_(read_mode), value_(x), tag_(tag) { }

order_token_t order_token_t::with_read_mode() const {
    return order_token_t(bucket_, value_, true, tag_);
}

void order_token_t::assert_read_mode() const {
    rassert(is_ignore() || read_mode_, "Expected an order token in read mode");
}

void order_token_t::assert_write_mode() const {
    rassert(is_ignore() || !read_mode_, "Expected an order token in write mode");
}

const std::string &order_token_t::tag() const { return tag_; }

bool order_token_t::is_invalid() const { return !bucket_.valid() && value_ == ORDER_INVALID; }
bool order_token_t::is_ignore() const { return !bucket_.valid() && value_ == ORDER_IGNORE; }

#ifndef NDEBUG
RDB_IMPL_ME_SERIALIZABLE_4(order_token_t, bucket_, read_mode_, value_, tag_);
#endif


order_source_t::order_source_t() : bucket_(order_bucket_t::create()), counter_(0) { }
order_source_t::order_source_t(threadnum_t specified_home_thread)
    : home_thread_mixin_debug_only_t(specified_home_thread),
      bucket_(order_bucket_t::create()), counter_(0) { }

order_source_t::~order_source_t() { }

order_token_t order_source_t::check_in(const std::string &tag) {
    assert_thread();
    ++counter_;
    return order_token_t(bucket_, counter_, false, tag);
}




order_sink_t::order_sink_t() { }

void order_sink_t::check_out(order_token_t token) {
    assert_thread();
    if (!token.is_ignore()) {
        rassert(!token.is_invalid());
        /* If we haven't seen this bucket before, then this will
        make a new entry in the map and fill it with a pair of (0, 0).
        Either way, `last_seen` will be a pointer to the `std::pair`
        that ends up in the map. */
        last_seens_map_t::iterator it = last_seens_.insert(last_seens_map_t::value_type(token.bucket_, std::pair<tagged_seen_t, tagged_seen_t>(tagged_seen_t(0, "0"), tagged_seen_t(0, "0")))).first;
        std::pair<tagged_seen_t, tagged_seen_t> *last_seen = &it->second;
        verify_token_value_and_update(token, last_seen);
    }
}

void order_sink_t::verify_token_value_and_update(order_token_t token, std::pair<tagged_seen_t, tagged_seen_t> *ls_pair) {
    // We tolerate equality in this comparison because it can be used
    // to ensure that multiple actions don't get interrupted.  And
    // resending the same action isn't normally a problem.
    if (token.read_mode_) {
        rassert(token.value_ >= ls_pair->first.value, "read_mode expected (0x%" PRIx64 " >= 0x%" PRIx64 "), (%s >= %s), bucket = (%s)", token.value_, ls_pair->first.value, token.tag_.c_str(), ls_pair->first.tag.c_str(), uuid_to_str(token.bucket_.uuid_).c_str());
        if (ls_pair->second.value < token.value_) {
            ls_pair->second = tagged_seen_t(token.value_, token.tag_);
        }
    } else {
        rassert(token.value_ >= ls_pair->second.value, "write_mode expected (0x%" PRIx64 " >= 0x%" PRIx64 "), (%s >= %s), bucket = (%s)", token.value_, ls_pair->second.value, token.tag_.c_str(), ls_pair->second.tag.c_str(), uuid_to_str(token.bucket_.uuid_).c_str());
        ls_pair->first = ls_pair->second = tagged_seen_t(token.value_, token.tag_);
    }
}


plain_sink_t::plain_sink_t() : ls_pair_(tagged_seen_t(0, "0"), tagged_seen_t(0, "0")), have_bucket_(false), bucket_(order_bucket_t::invalid()) { }

void plain_sink_t::check_out(order_token_t token) {
    assert_thread();
    if (!token.is_ignore()) {
        rassert(!token.is_invalid());
        if (!have_bucket_) {
            bucket_ = token.bucket_;
            have_bucket_ = true;
        } else {
            rassert(token.bucket_ == bucket_, "plain_sink_t can only be used with one order_source_t. for %s (last seens %s, %s)", token.tag_.c_str(), ls_pair_.first.tag.c_str(), ls_pair_.second.tag.c_str());
        }

        order_sink_t::verify_token_value_and_update(token, &ls_pair_);
    }
}

void order_checkpoint_t::set_tagappend(const std::string &tagappend) {
    rassert(tagappend_.empty());
    tagappend_ = "+" + tagappend;
}

order_token_t order_checkpoint_t::check_through(order_token_t tok) {
    assert_thread();
    if (!tok.is_ignore()) {
        sink_.check_out(tok);
        order_token_t tok2 = source_.check_in(tok.tag() + tagappend_);
        if (tok.read_mode_) tok2 = tok2.with_read_mode();
        return tok2;
    } else {
        return order_token_t::ignore;
    }
}

order_token_t order_checkpoint_t::checkpoint_raw_check_in() {
    return source_.check_in("(nothing)" + tagappend_);
}

#endif  // ifndef NDEBUG
