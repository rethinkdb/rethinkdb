// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_LAZY_BTREE_VAL_HPP_
#define RDB_PROTOCOL_LAZY_BTREE_VAL_HPP_

#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "rdb_protocol/datum.hpp"

struct rdb_value_t {
    char contents[];

public:
    int inline_size(max_block_size_t bs) const {
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

ql::datum_t get_data(const rdb_value_t *value,
                     buf_parent_t parent);

class lazy_btree_val_pointee_t
        : public single_threaded_countable_t<lazy_btree_val_pointee_t> {
    lazy_btree_val_pointee_t(const rdb_value_t *_rdb_value, buf_parent_t _parent)
        : rdb_value(_rdb_value), parent(_parent) {
        guarantee(rdb_value != NULL);
    }

    explicit lazy_btree_val_pointee_t(const ql::datum_t &_ptr)
        : ptr(_ptr), rdb_value(NULL), parent() {
        guarantee(ptr.has());
    }

    friend class lazy_btree_val_t;

    // If empty, we haven't loaded the value yet.
    ql::datum_t ptr;

    // A pointer to the rdb value buffer in the leaf node (or perhaps a copy), and
    // the transaction with which to load it.  Non-NULL only if ptr is empty.
    const rdb_value_t *rdb_value;
    buf_parent_t parent;

    DISABLE_COPYING(lazy_btree_val_pointee_t);
};

class lazy_btree_val_t {
public:
    explicit lazy_btree_val_t(const ql::datum_t &ptr)
        : pointee(new lazy_btree_val_pointee_t(ptr)) { }

    lazy_btree_val_t(const rdb_value_t *rdb_value, buf_parent_t parent)
        : pointee(new lazy_btree_val_pointee_t(rdb_value, parent)) { }

    const ql::datum_t &get() const;
    bool references_parent() const;
    void reset();

private:
    counted_t<lazy_btree_val_pointee_t> pointee;
};



#endif  // RDB_PROTOCOL_LAZY_BTREE_VAL_HPP_
