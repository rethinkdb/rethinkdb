// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ERASE_RANGE_HPP_
#define RDB_PROTOCOL_ERASE_RANGE_HPP_

#include <vector>

#include "errors.hpp"
#include "btree/keys.hpp"
#include "btree/types.hpp"
#include "buffer_cache/types.hpp"

class btree_slice_t;
struct btree_key_t;
class deletion_context_t;
struct rdb_modification_report_t;
class superblock_t;
class signal_t;

class key_tester_t {
public:
    key_tester_t() { }
    virtual bool key_should_be_erased(const btree_key_t *key) = 0;

protected:
    virtual ~key_tester_t() { }
private:
    DISABLE_COPYING(key_tester_t);
};

struct always_true_key_tester_t : public key_tester_t {
    bool key_should_be_erased(UNUSED const btree_key_t *key) {
        return true;
    }
};

/* `rdb_erase_small_range` has a complexity of O(log n * m) where n is the size of
 * the btree, and m is the number of documents actually being deleted.
 * It also requires O(m) memory.
 * It returns a number of modification reports that should be applied
 * to secondary indexes separately. Blobs are detached, and should be deleted later
 * if required (passing the modification reports to store_t::update_sindexes()
 * takes care of that). */
done_traversing_t rdb_erase_small_range(
    btree_slice_t *btree_slice,
    key_tester_t *tester,
    const key_range_t &keys,
    superblock_t *superblock,
    const deletion_context_t *deletion_context,
    signal_t *interruptor,
    uint64_t max_keys_to_erase /* 0 = unlimited */,
    std::vector<rdb_modification_report_t> *mod_reports_out);

#endif  // RDB_PROTOCOL_ERASE_RANGE_HPP_
