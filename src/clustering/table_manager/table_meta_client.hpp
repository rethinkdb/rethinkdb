// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_

#include "clustering/table_manager/table_metadata.hpp"
#include "concurrency/watchable_map.hpp"

/* `table_meta_client_t` is responsible for submitting client requests over the network
to the `table_meta_manager_t`. It doesn't have any real state of its own; it's just a
convenient way of bundling together all of the objects that are necessary for submitting
a client request. */
class table_meta_client_t {
public:
    table_meta_client_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, table_meta_manager_business_card_t>
            *_table_meta_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_business_card_t>
            *_table_meta_directory);
    bool create(
        const table_config_t &new_config,
        signal_t *interruptor,
        namespace_id_t *table_id_out);
    bool drop(
        const namespace_id_t &table_id,
        signal_t *interruptor);
    bool get_config(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        table_config_t *config_out);
    bool set_config(
        const namespace_id_t &table_id,
        const table_config_t &new_config,
        signal_t *interruptor);
private:
    mailbox_manager_t *mailbox_manager;
    watchable_map_t<peer_id_t, table_meta_manager_business_card_t>
        *table_meta_manager_directory;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_business_card_t>
        *table_meta_directory;
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_ */

