// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_SECONDARY_OPERATIONS_HPP_
#define BTREE_SECONDARY_OPERATIONS_HPP_

#include <map>
#include <string>
#include <vector>

#include "buffer_cache/types.hpp"
#include "containers/archive/archive.hpp"
#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"
#include "serializer/types.hpp"

/* This file contains code for working with the sindex block, which is a child of the
table's primary superblock.

`btree/secondary_operations.*` and `btree/reql_specific.*` are the only files in the
`btree/` directory that know about ReQL-specific concepts such as metainfo and sindexes.
They should probably be moved out of the `btree/` directory. */

class buf_lock_t;

struct secondary_index_t {
    secondary_index_t()
        : superblock(NULL_BLOCK_ID),
          post_construction_complete(false),
          being_deleted(false),
          /* TODO(2014-08): This generate_uuid() is weird. */
          id(generate_uuid()) { }

    /* A virtual superblock. */
    block_id_t superblock;

    /* Whether the index is has completed post construction, and/or is being deleted.
     * Note that an index can be in any combination of those states. */
    bool post_construction_complete;
    bool being_deleted;
    bool is_ready() const {
        return post_construction_complete && !being_deleted;
    }

    /* An opaque blob that describes the index.  See serialize_sindex_info and
     deserialize_sindex_info.  At one point it contained a serialized map_wire_func_t
     and a sindex_multi_bool_t.  Now it also contains reql version info.  (This being
     a std::vector<char> is a holdover from when we had multiple protocols.) */
    std::vector<char> opaque_definition;

    /* Sindexes contain a uuid_u to prevent a rapid deletion and recreation of
     * a sindex with the same name from tricking a post construction in to
     * switching targets. This issue is described in more detail here:
     * https://github.com/rethinkdb/rethinkdb/issues/657 */
    uuid_u id;
};

RDB_DECLARE_SERIALIZABLE(secondary_index_t);

struct sindex_name_t {
    sindex_name_t()
        : name(""), being_deleted(false) { }
    explicit sindex_name_t(const std::string &n)
        : name(n), being_deleted(false) { }

    bool operator<(const sindex_name_t &other) const {
        return (being_deleted && !other.being_deleted) ||
               (being_deleted == other.being_deleted && name < other.name);
    }
    bool operator==(const sindex_name_t &other) const {
        return being_deleted == other.being_deleted && name == other.name;
    }

    std::string name;
    // This additional bool in `sindex_name_t` makes sure that the name
    // of an index that's being deleted can never conflict with any newly created
    // index.
    bool being_deleted;
};

RDB_DECLARE_SERIALIZABLE(sindex_name_t);

//Secondary Index functions

/* Note if this function is called after secondary indexes have been added it
 * will leak blocks (and also make those secondary indexes unusable.) There's
 * no reason to ever do this. */
void initialize_secondary_indexes(buf_lock_t *superblock);

bool get_secondary_index(buf_lock_t *sindex_block,
                         const sindex_name_t &name,
                         secondary_index_t *sindex_out);

bool get_secondary_index(buf_lock_t *sindex_block, uuid_u id,
                         secondary_index_t *sindex_out);

void get_secondary_indexes(buf_lock_t *sindex_block,
                           std::map<sindex_name_t, secondary_index_t> *sindexes_out);

/* Rewrites the secondary index block with up-to-date serialization */
void migrate_secondary_index_block(buf_lock_t *sindex_block,
                                   std::map<sindex_name_t, secondary_index_t> *sindexes_out);

/* Overwrites existing values with the same id. */
void set_secondary_index(buf_lock_t *sindex_block,
                         const sindex_name_t &name, const secondary_index_t &sindex);

/* Must be used to overwrite an already existing sindex. */
void set_secondary_index(buf_lock_t *sindex_block, uuid_u id,
                         const secondary_index_t &sindex);

// XXX note this just drops the entry. It doesn't cleanup the btree that it points
// to. `drop_sindex` Does both and should be used publicly.
bool delete_secondary_index(buf_lock_t *sindex_block, const sindex_name_t &name);

#endif /* BTREE_SECONDARY_OPERATIONS_HPP_ */
