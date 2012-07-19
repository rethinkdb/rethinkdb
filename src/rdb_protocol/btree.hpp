#ifndef RDB_PROTOCOL_BTREE_HPP_
#define RDB_PROTOCOL_BTREE_HPP_

#include "errors.hpp"

#include "buffer_cache/blob.hpp"
#include "rdb_protocol/protocol.hpp"

typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;

typedef rdb_protocol_t::rget_read_t rget_read_t;
typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

static const size_t rget_max_chunk_size = MEGABYTE;

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

#define MAX_RDB_VALUE_SIZE MAX_IN_NODE_VALUE_SIZE

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value);

template <>
class value_sizer_t<rdb_value_t> : public value_sizer_t<void> {
public:
    explicit value_sizer_t<rdb_value_t>(block_size_t bs) : block_size_(bs) { }

    static const rdb_value_t *as_rdb(const void *p) {
        return reinterpret_cast<const rdb_value_t *>(p);
    }

    int size(const void *value) const {
        return as_rdb(value)->inline_size(block_size_);
    }

    bool fits(const void *value, int length_available) const {
        return btree_value_fits(block_size_, length_available, as_rdb(value));
    }

    bool deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const {
        if (!fits(value, length_available)) {
            *msg_out = "value does not fit in length_available";
            return false;
        }

        return blob::deep_fsck(getter, block_size_, as_rdb(value)->value_ref(), blob::btree_maxreflen, msg_out);
    }

    int max_possible_size() const {
        return blob::btree_maxreflen;
    }

    static block_magic_t leaf_magic() {
        block_magic_t magic = { { 'r', 'd', 'b', 'l' } };
        return magic;
    }

    block_magic_t btree_leaf_magic() const {
        return leaf_magic();
    }

    block_size_t block_size() const { return block_size_; }

private:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;

    DISABLE_COPYING(value_sizer_t<rdb_value_t>);
};

point_read_response_t rdb_get(const store_key_t &key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock);

point_write_response_t rdb_set(const store_key_t &key, boost::shared_ptr<scoped_cJSON_t> data,
                       btree_slice_t *slice, repli_timestamp_t timestamp,
                       transaction_t *txn, superblock_t *superblock);


class backfill_callback_t {
public:
    virtual void on_delete_range(const key_range_t &range) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency) = 0;
    virtual void on_keyvalue(const rdb_protocol_details::backfill_atom_t& atom) = 0;
protected:
    virtual ~backfill_callback_t() { }
};


void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when, backfill_callback_t *callback,
                    transaction_t *txn, superblock_t *superblock, parallel_traversal_progress_t *p);


point_delete_response_t rdb_delete(const store_key_t &key, btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn, superblock_t *superblock);

void rdb_erase_range(btree_slice_t *slice, key_tester_t *tester,
                                 const key_range_t &keys,
                                 transaction_t *txn, superblock_t *superblock);

/* RGETS */
size_t estimate_rget_response_size(const boost::shared_ptr<scoped_cJSON_t> &json);

struct rget_response_t {
    std::vector<boost::shared_ptr<scoped_cJSON_t> > pairs;
    bool truncated;
};

rget_read_response_t rdb_rget_slice(btree_slice_t *slice, const key_range_t &range,
                               int maximum, transaction_t *txn, superblock_t *superblock);

#endif /* RDB_PROTOCOL_BTREE_HPP_ */
