#ifndef RDB_PROTOCOL_LAZY_JSON_HPP_
#define RDB_PROTOCOL_LAZY_JSON_HPP_

#include "http/json.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/types.hpp"

#define MAX_RDB_VALUE_SIZE MAX_IN_NODE_VALUE_SIZE

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

std::shared_ptr<scoped_cJSON_t> get_data(const rdb_value_t *value,
                                           transaction_t *txn);

class lazy_json_pointee_t : public single_threaded_countable_t<lazy_json_pointee_t> {
    lazy_json_pointee_t(const rdb_value_t *_rdb_value, transaction_t *_txn)
        : rdb_value(_rdb_value), txn(_txn) {
        guarantee(rdb_value != NULL);
        guarantee(txn != NULL);
    }

    explicit lazy_json_pointee_t(const std::shared_ptr<scoped_cJSON_t> &_ptr)
        : ptr(_ptr), rdb_value(NULL), txn(NULL) {
        guarantee(ptr);
    }

    friend class lazy_json_t;

    // If empty, we haven't loaded the value yet.
    std::shared_ptr<scoped_cJSON_t> ptr;

    // A pointer to the rdb value buffer in the leaf node (or perhaps a copy), and the
    // transaction with which to load it.
    const rdb_value_t *rdb_value;
    transaction_t *txn;

    DISABLE_COPYING(lazy_json_pointee_t);
};

class lazy_json_t {
public:
    explicit lazy_json_t(const std::shared_ptr<scoped_cJSON_t> &ptr)
        : pointee(new lazy_json_pointee_t(ptr)) { }

    lazy_json_t(const rdb_value_t *rdb_value, transaction_t *txn)
        : pointee(new lazy_json_pointee_t(rdb_value, txn)) { }

    const std::shared_ptr<scoped_cJSON_t> &get() const;

private:
    counted_t<lazy_json_pointee_t> pointee;
};



#endif  // RDB_PROTOCOL_LAZY_JSON_HPP_
