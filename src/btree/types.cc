// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/depth_first_traversal.hpp"

scoped_key_value_t::scoped_key_value_t(const btree_key_t *key,
                                       const void *value,
                                       movable_t<counted_buf_lock_t> &&buf)
    : key_(key), value_(value), buf_(std::move(buf)) {
    guarantee(buf_.has());
}

scoped_key_value_t::scoped_key_value_t(scoped_key_value_t &&movee)
    : key_(movee.key_),
      value_(movee.value_),
      buf_(std::move(movee.buf_)) {
    movee.key_ = NULL;
    movee.value_ = NULL;
}

scoped_key_value_t::~scoped_key_value_t() { }


buf_parent_t scoped_key_value_t::expose_buf() {
    guarantee(buf_.has());
    return buf_parent_t(buf_.get());
}

// Releases the hold on the buf_lock_t, after which key(), value(), and expose_buf()
// may not be used.
void scoped_key_value_t::reset() { buf_.reset(); }
