// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BTREE_SECONDARY_OPERATIONS_HPP_
#define BTREE_SECONDARY_OPERATIONS_HPP_

#include <map>
#include <string>
#include <vector>

#include "buffer_cache/mirrored/writeback.hpp"
#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"
#include "serializer/types.hpp"

struct secondary_index_t {
    secondary_index_t()
        : superblock(NULL_BLOCK_ID), post_construction_complete(false),
          id(generate_uuid())
    { }

    /* A virtual superblock. */
    block_id_t superblock;

    /* An opaque_definition_t is a serializable description of the secondary
     * index. Values which are stored in the B-Tree (template parameters to
     * `find_keyvalue_location_for_[read,write]`) must support the following
     * method:
     * store_key_t index(const opaque_definition_t &);
     * Which returns the value of the secondary index.
     */
    typedef std::vector<char> opaque_definition_t;

    /* Whether or not the sindex has complete postconstruction. */
    bool post_construction_complete;

    /* An opaque blob that describes the index */
    opaque_definition_t opaque_definition;

    /* Sindexes contain a uuid_u to prevent a rapid deletion and recreation of
     * a sindex with the same name from tricking a post construction in to
     * switching targets. This issue is described in more detail here:
     * https://github.com/rethinkdb/rethinkdb/issues/657 */
    uuid_u id;

    /* Used in unit tests. */
    bool operator==(const secondary_index_t &other) const {
        return superblock == other.superblock &&
               opaque_definition == other.opaque_definition;
    }

    RDB_DECLARE_ME_SERIALIZABLE;
};

//Secondary Index functions

/* Note if this function is called after secondary indexes have been added it
 * will leak blocks (and also make those secondary indexes unusable.) There's
 * no reason to ever do this. */
void initialize_secondary_indexes(transaction_t *txn, buf_lock_t *superblock);

bool get_secondary_index(transaction_t *txn, buf_lock_t *sindex_block, const std::string &id, secondary_index_t *sindex_out);

bool get_secondary_index(transaction_t *txn, buf_lock_t *sindex_block, uuid_u id, secondary_index_t *sindex_out);

void get_secondary_indexes(transaction_t *txn, buf_lock_t *sindex_block, std::map<std::string, secondary_index_t> *sindexes_out);

/* Overwrites existing values with the same id. */
void set_secondary_index(transaction_t *txn, buf_lock_t *sindex_block, const std::string &id, const secondary_index_t &sindex);

/* Must be used to overwrite an already existing sindex. */
void set_secondary_index(transaction_t *txn, buf_lock_t *sindex_block, uuid_u id, const secondary_index_t &sindex);

//XXX note this just drops the entry. It doesn't cleanup the btree that it
//points to. drop_secondary_index. Does both and should be used publicly.
bool delete_secondary_index(transaction_t *txn, buf_lock_t *sindex_block, const std::string &id);

//XXX note this just drops the enties. It doesn't cleanup the btree that it
//points to. drop_secondary_indexes. Does both and should be used publicly.
void delete_all_secondary_indexes(transaction_t *txn, buf_lock_t *sindex_block);

#endif /* BTREE_SECONDARY_OPERATIONS_HPP_ */
