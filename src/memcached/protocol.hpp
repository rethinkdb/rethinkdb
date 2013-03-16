// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MEMCACHED_PROTOCOL_HPP_
#define MEMCACHED_PROTOCOL_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "backfill_progress.hpp"
#include "btree/btree_store.hpp"
#include "buffer_cache/types.hpp"
#include "containers/archive/stl_types.hpp"
#include "memcached/memcached_btree/backfill.hpp"
#include "memcached/queries.hpp"
#include "memcached/region.hpp"
#include "hash_region.hpp"
#include "protocol_api.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"
#include "perfmon/types.hpp"
#include "repli_timestamp.hpp"

class io_backender_t;
class real_superblock_t;
class traversal_progress_combiner_t;

write_message_t &operator<<(write_message_t &msg, const intrusive_ptr_t<data_buffer_t> &buf);
archive_result_t deserialize(read_stream_t *s, intrusive_ptr_t<data_buffer_t> *buf);
write_message_t &operator<<(write_message_t &msg, const rget_result_t &iter);
archive_result_t deserialize(read_stream_t *s, rget_result_t *iter);

RDB_DECLARE_SERIALIZABLE(get_query_t);
RDB_DECLARE_SERIALIZABLE(rget_query_t);
RDB_DECLARE_SERIALIZABLE(distribution_get_query_t);
RDB_DECLARE_SERIALIZABLE(get_result_t);
RDB_DECLARE_SERIALIZABLE(key_with_data_buffer_t);
RDB_DECLARE_SERIALIZABLE(rget_result_t);
RDB_DECLARE_SERIALIZABLE(distribution_result_t);
RDB_DECLARE_SERIALIZABLE(get_cas_mutation_t);
RDB_DECLARE_SERIALIZABLE(sarc_mutation_t);
RDB_DECLARE_SERIALIZABLE(delete_mutation_t);
RDB_DECLARE_SERIALIZABLE(incr_decr_mutation_t);
RDB_DECLARE_SERIALIZABLE(incr_decr_result_t);
RDB_DECLARE_SERIALIZABLE(append_prepend_mutation_t);
RDB_DECLARE_SERIALIZABLE(backfill_atom_t);

#endif /* MEMCACHED_PROTOCOL_HPP_ */
