// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_LAZY_JSON_HPP_
#define RDB_PROTOCOL_LAZY_JSON_HPP_

#include "btree/slice.hpp"  // RSI: for SLICE_ALT
#include "buffer_cache/blob.hpp"
#include "buffer_cache/types.hpp"
#include "rdb_protocol/datum.hpp"

struct rdb_value_t {
    char contents[];

public:
    int inline_size(block_size_t bs) const {
        return blob::ref_size(bs, contents, blob::btree_maxreflen);
    }

    int64_t value_size() const {
        return blob::value_size(contents, blob::btree_maxreflen);
    }

    const char *value_ref() const {
        return contents;
    }

    char *value_ref() {
        return contents;
    }
};

#if SLICE_ALT
counted_t<const ql::datum_t> get_data(const rdb_value_t *value,
                                      alt::alt_buf_parent_t parent);
#else
counted_t<const ql::datum_t> get_data(const rdb_value_t *value,
                                      transaction_t *txn);
#endif

class lazy_json_pointee_t : public single_threaded_countable_t<lazy_json_pointee_t> {
#if SLICE_ALT
    // RSI: Make sure callers/constructors get the lifetime of the buf parent right.
    lazy_json_pointee_t(const rdb_value_t *_rdb_value, alt::alt_buf_parent_t _parent)
        : rdb_value(_rdb_value), parent(_parent) {
#else
    lazy_json_pointee_t(const rdb_value_t *_rdb_value, transaction_t *_txn)
        : rdb_value(_rdb_value), txn(_txn) {
#endif
        guarantee(rdb_value != NULL);
#if !SLICE_ALT
        guarantee(txn != NULL);
#endif
    }

#if SLICE_ALT
    explicit lazy_json_pointee_t(const counted_t<const ql::datum_t> &_ptr)
        : ptr(_ptr), rdb_value(NULL), parent() {
        guarantee(ptr);
    }
#else
    explicit lazy_json_pointee_t(const counted_t<const ql::datum_t> &_ptr)
        : ptr(_ptr), rdb_value(NULL), txn(NULL) {
        guarantee(ptr);
    }
#endif

    friend class lazy_json_t;

    // If empty, we haven't loaded the value yet.
    counted_t<const ql::datum_t> ptr;

#if SLICE_ALT
    // A pointer to the rdb value buffer in the leaf node (or perhaps a copy), and
    // the transaction with which to load it.  Non-NULL only if ptr is empty.
    const rdb_value_t *rdb_value;
    alt::alt_buf_parent_t parent;
#else
    // A pointer to the rdb value buffer in the leaf node (or perhaps a copy), and
    // the transaction with which to load it.  Non-NULL only if ptr is empty.
    const rdb_value_t *rdb_value;
    transaction_t *txn;
#endif

    DISABLE_COPYING(lazy_json_pointee_t);
};

class lazy_json_t {
public:
    explicit lazy_json_t(const counted_t<const ql::datum_t> &ptr)
        : pointee(new lazy_json_pointee_t(ptr)) { }

#if SLICE_ALT
    lazy_json_t(const rdb_value_t *rdb_value, alt::alt_buf_parent_t parent)
        : pointee(new lazy_json_pointee_t(rdb_value, parent)) { }
#else
    lazy_json_t(const rdb_value_t *rdb_value, transaction_t *txn)
        : pointee(new lazy_json_pointee_t(rdb_value, txn)) { }
#endif

    const counted_t<const ql::datum_t> &get() const;
    void reset();

private:
    counted_t<lazy_json_pointee_t> pointee;
};



#endif  // RDB_PROTOCOL_LAZY_JSON_HPP_
