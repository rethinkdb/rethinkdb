// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_METADATA_HPP_

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "rdb_protocol/protocol.hpp"

class remote_replicator_client_intro_t {
public:
    typedef mailbox_t<void()> ready_mailbox_t;

    state_timestamp_t streaming_begin_timestamp;
    ready_mailbox_t::address_t ready_mailbox;
};

RDB_DECLARE_SERIALIZABLE(remote_replicator_client_intro_t);

class remote_replicator_client_bcard_t {
public:
    typedef mailbox_t<void(
        remote_replicator_client_intro_t
        )> intro_mailbox_t;
    typedef mailbox_t<void(
        write_t, state_timestamp_t, order_token_t,
        mailbox_t<void()>::address_t
        )> write_async_mailbox_t;
    typedef mailbox_t<void(
        write_t, state_timestamp_t, order_token_t, write_durability_t,
        mailbox_t<void(write_response_t)>::address_t
        )> write_sync_mailbox_t;
    typedef mailbox_t<void(
        mailbox_t<void(write_response_t)>::address_t
        )> dummy_write_mailbox_t;
    typedef mailbox_t<void(
        read_t, state_timestamp_t,
        mailbox_t<void(read_response_t)>::address_t
        )> read_mailbox_t;

    server_id_t server_id;
    intro_mailbox_t::address_t intro_mailbox;
    write_async_mailbox_t::address_t write_async_mailbox;
    write_sync_mailbox_t::address_t write_sync_mailbox;
    dummy_write_mailbox_t::address_t dummy_write_mailbox;
    read_mailbox_t::address_t read_mailbox;
};

RDB_DECLARE_SERIALIZABLE(remote_replicator_client_bcard_t);

class remote_replicator_server_bcard_t {
public:
    branch_id_t branch;
    region_t region;
    registrar_business_card_t<remote_replicator_client_bcard_t> registrar;
};

RDB_DECLARE_SERIALIZABLE(remote_replicator_server_bcard_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_METADATA_HPP_ */

