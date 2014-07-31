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

class buf_lock_t;

struct secondary_index_t {
    explicit secondary_index_t()
        : superblock(NULL_BLOCK_ID), post_construction_complete(false),
          being_deleted(false),
          /* These default values _should_ be overwritten, but we default them to
             LATEST_DISK because a newly created secondary_index_t would use the
             latest ReQL version. */
          original_reql_version(valgrind_undefined(cluster_version_t::LATEST_DISK)),
          latest_compatible_reql_version(valgrind_undefined(cluster_version_t::LATEST_DISK)),
          latest_checked_reql_version(valgrind_undefined(cluster_version_t::LATEST_DISK)),
          /* TODO(2014-08): This generate_uuid() is weird. */
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

    /* Whether the index is has completed post construction, and/or is being deleted.
     * Note that an index can be in any combination of those states. */
    bool post_construction_complete;
    bool being_deleted;
    bool is_ready() const {
        return post_construction_complete && !being_deleted;
    }

    /* opaque_definition below describes a ReQL function that uses Terms that might
       be subject to bug fixes.  We want to maintain the old ReQL behavior for sindex
       functions that were created with old versions of RethinkDB (so that we don't
       crash and sindexes maintain their consistency so that the user can manually
       migrate).  ReQL evaluation is versioned so that we can continue to run old
       buggy implementations of ReQL functions after bugfix updates have been
       applied.  ReQL evaluation versions are the same thing as disk format versions,
       of course. */

    /* Generally speaking, original_reql_version <= latest_compatible_reql_version <=
       latest_checked_reql_version.  When a new sindex is created, the values are the
       same.  When a new version of RethinkDB gets run, latest_checked_reql_version
       will get updated, and latest_compatible_reql_version will get updated if the
       sindex function is compatible with a later version than the original value of
       `latest_checked_reql_version`. */

    /* The original ReQL version of the sindex function.  The value here never
       changes.  This might become useful for tracking down some bugs or fixing them
       in-place, or performing a desparate reverse migration. */
    cluster_version_t original_reql_version;

    /* This is the latest version for which evaluation of the sindex function remains
       compatible. */
    cluster_version_t latest_compatible_reql_version;

    /* If this is less than the current server version, we'll re-check
       opaque_definition for compatibility and update this value and
       `latest_compatible_reql_version` accordingly. */
    cluster_version_t latest_checked_reql_version;


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
};

template <cluster_version_t W>
void serialize(write_message_t *wm, const secondary_index_t &sindex);

template <cluster_version_t W>
typename std::enable_if<W == cluster_version_t::v1_13 || W == cluster_version_t::v1_13_2,
               archive_result_t>::type
deserialize(read_stream_t *s, secondary_index_t *out);
template <cluster_version_t W>
typename std::enable_if<W == cluster_version_t::v1_14_is_latest_disk,
                        archive_result_t>::type
deserialize(read_stream_t *s, secondary_index_t *out);

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
