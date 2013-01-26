// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BTREE_SECONDARY_OPERATIONS_HPP
#define BTREE_SECONDARY_OPERATIONS_HPP

#include <vector>

#include "buffer_cache/mirrored/writeback.hpp"
#include "containers/archive/archive.hpp"
#include "rpc/serialize_macros.hpp"
#include "serializer/types.hpp"

namespace sindex_details {
enum sindex_state_t {
    UNCREATED,
    INCOMPLETE,
    CREATED,
    DELETED
};
} //namespace sindex_details

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(sindex_details::sindex_state_t, int8_t, sindex_details::UNCREATED, sindex_details::DELETED);

struct secondary_index_t {
    secondary_index_t()
        : superblock(NULL_BLOCK_ID)
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

    /* An opaque blob that describes the index */
    opaque_definition_t opaque_definition;

    /* Another opaque blob that describes which parts of the index are up to
     * data. (Much like metainfo does for the main B-Tree) */
    std::vector<char> metainfo;

    /* Used in unit tests. */
    bool operator==(const secondary_index_t & other) const {
        return superblock == other.superblock &&
               opaque_definition == other.opaque_definition;
    }

    RDB_MAKE_ME_SERIALIZABLE_3(superblock, opaque_definition, metainfo);
};

//Secondary Index functions

/* Note if this function is called after secondary indexes have been added it
 * will leak blocks (and also make those secondary indexes unusable.) There's
 * no reason to ever do this. */
void initialize_secondary_indexes(transaction_t *txn, buf_lock_t *superblock);

bool get_secondary_index(transaction_t *txn, buf_lock_t *sindex_block, uuid_u uuid, secondary_index_t *sindex_out);

void get_secondary_indexes(transaction_t *txn, buf_lock_t *sindex_block, std::map<uuid_u, secondary_index_t> *sindexes_out);

/* Overwrites existing values with the same id. */
void set_secondary_index(transaction_t *txn, buf_lock_t *sindex_block, uuid_u uuid, const secondary_index_t &sindex);

//XXX note this just drops the entry. It doesn't cleanup the btree that it
//points to. drop_secondary_index. Does both and should be used publicly.
bool delete_secondary_index(transaction_t *txn, buf_lock_t *sindex_block, uuid_u uuid);

#include "btree/secondary_operations.tcc"

#endif
