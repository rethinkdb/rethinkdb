#ifndef RDB_PROTOCOL_BTREE_HPP_
#define RDB_PROTOCOL_BTREE_HPP_

#include <string>
#include <utility>
#include <vector>

#include "backfill_progress.hpp"
#include "rdb_protocol/protocol.hpp"

class key_tester_t;
class parallel_traversal_progress_t;
struct rdb_value_t;


typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;

typedef rdb_protocol_t::rget_read_t rget_read_t;
typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

typedef rdb_protocol_t::distribution_read_t distribution_read_t;
typedef rdb_protocol_t::distribution_read_response_t distribution_read_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_modify_t point_modify_t;
typedef rdb_protocol_t::point_modify_response_t point_modify_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

namespace query_language {
    class runtime_environment_t;
} //namespace query_language

class parallel_traversal_progress_t;

static const size_t rget_max_chunk_size = MEGABYTE;

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value);

template <>
class value_sizer_t<rdb_value_t> : public value_sizer_t<void> {
public:
    explicit value_sizer_t<rdb_value_t>(block_size_t bs);

    static const rdb_value_t *as_rdb(const void *p);

    int size(const void *value) const;

    bool fits(const void *value, int length_available) const;

    bool deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const;
    int max_possible_size() const;

    static block_magic_t leaf_magic();

    block_magic_t btree_leaf_magic() const;

    block_size_t block_size() const;

private:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;

    DISABLE_COPYING(value_sizer_t<rdb_value_t>);
};

point_read_response_t rdb_get(const store_key_t &key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock);

point_modify_response_t rdb_modify(const std::string &primary_key, const store_key_t &key, const point_modify::op_t op,
                                   query_language::runtime_environment_t *env, const Mapping &mapping,
                                   btree_slice_t *slice, repli_timestamp_t timestamp,
                                   transaction_t *txn, superblock_t *superblock);

point_write_response_t rdb_set(const store_key_t &key, boost::shared_ptr<scoped_cJSON_t> data,
                       btree_slice_t *slice, repli_timestamp_t timestamp,
                       transaction_t *txn, superblock_t *superblock);


class rdb_backfill_callback_t {
public:
    virtual void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_keyvalue(const rdb_protocol_details::backfill_atom_t& atom, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
protected:
    virtual ~rdb_backfill_callback_t() { }
};


void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range,
        repli_timestamp_t since_when, rdb_backfill_callback_t *callback,
        transaction_t *txn, superblock_t *superblock,
        parallel_traversal_progress_t *p, signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);


point_delete_response_t rdb_delete(const store_key_t &key, btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn, superblock_t *superblock);

void rdb_erase_range(btree_slice_t *slice, key_tester_t *tester,
                                 const key_range_t &keys,
                                 transaction_t *txn, superblock_t *superblock);

/* RGETS */
size_t estimate_rget_response_size(const boost::shared_ptr<scoped_cJSON_t> &json);

struct rget_response_t {
    std::vector<std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> > > pairs;
    bool truncated;
};

rget_read_response_t rdb_rget_slice(btree_slice_t *slice, const key_range_t &range,
                               int maximum, transaction_t *txn, superblock_t *superblock,
                               query_language::runtime_environment_t *env, const rdb_protocol_details::transform_t &transform,
                               boost::optional<rdb_protocol_details::terminal_t> terminal);

distribution_read_response_t rdb_distribution_get(btree_slice_t *slice, int max_depth, const store_key_t &left_key, 
                                                 transaction_t *txn, superblock_t *superblock);

#endif /* RDB_PROTOCOL_BTREE_HPP_ */
