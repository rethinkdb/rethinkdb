// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/remote_replicator_metadata.hpp"

// TODO ATN
#ifdef CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_
#error bummer
#endif

#include "containers/archive/boost_types.hpp" // TODO ATN

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    remote_replicator_client_intro_t,
    streaming_begin_timestamp, ready_mailbox);
RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(
    remote_replicator_client_bcard_t,
    server_id, intro_mailbox, write_async_mailbox, write_sync_mailbox,
    dummy_write_mailbox, read_mailbox);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    remote_replicator_server_bcard_t,
    branch, region, registrar);

