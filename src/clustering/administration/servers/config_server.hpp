// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_SERVER_HPP_

#include <set>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/servers/server_metadata.hpp"

class server_config_server_t : public home_thread_mixin_t {
public:
    server_config_server_t(
        mailbox_manager_t *_mailbox_manager,
        metadata_file_t *_file);

    server_config_business_card_t get_business_card();

    clone_ptr_t<watchable_t<server_config_t> > get_config() {
        return config.get_watchable();
    }

    /* Returns the actual cache size, not the cache size setting. If the cache size
    setting is "auto", the actual cache size will be some reasonable automatically
    selected value; otherwise, the actual cache size will be the cache size setting. */
    clone_ptr_t<watchable_t<uint64_t> > get_actual_cache_size_bytes() {
        return actual_cache_size_bytes.get_watchable();
    }

private:
    /* `on_set_config()` is a mailbox callback */
    void on_set_config(
        signal_t *interruptor,
        const server_config_t &new_config,
        mailbox_t<void(std::string)>::address_t ack_addr);

    void update_actual_cache_size(const boost::optional<uint64_t> &setting);

    mailbox_manager_t *const mailbox_manager;
    metadata_file_t *const file;
    server_id_t my_server_id;
    watchable_variable_t<server_config_t> my_config;
    watchable_variable_t<uint64_t> actual_cache_size_bytes;

    server_config_business_card_t::set_config_mailbox_t set_config_mailbox;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_SERVER_HPP_ */

