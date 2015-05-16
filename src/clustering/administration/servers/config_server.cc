// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/config_server.hpp"

#include "clustering/administration/main/cache_size.hpp"

server_config_server_t::server_config_server_t(
        mailbox_manager_t *_mailbox_manager,
        metadata_file_t *_file) :
    mailbox_manager(_mailbox_manager),
    file(_file),
    my_config(server_config_t()),
    actual_cache_size_bytes(0),
    set_config_mailbox(mailbox_manager,
        std::bind(&server_config_server_t::on_set_config, this,
            ph::_1, ph::_2, ph::_3))
{
    cond_t non_interruptor;
    metadata_file_t::read_txn_t read_txn(file, &non_interruptor);
    my_server_id = read_txn.read(mdkey_server_id(), &non_interruptor);
    my_config.set_value(read_txn.read(mdkey_server_config(), &non_interruptor));
    update_actual_cache_size(my_config.get_ref().cache_size_bytes);
}

server_config_business_card_t server_config_server_t::get_business_card() {
    server_config_business_card_t bcard;
    bcard.set_config_addr = set_config_mailbox.get_address();
    return bcard;
}

void server_config_server_t::on_set_config(
        signal_t *interruptor,
        const server_config_t &new_config,
        const mailbox_t<void(std::string)>::address_t &ack_addr) {
    if (static_cast<bool>(new_config.cache_size) &&
            *new_config.cache_size > get_max_total_cache_size()) {
        send(mailbox_manager, ack_addr,
            strprintf("The proposed cache size of %" PRIu64 " MB is larger than the "
                "maximum legal value for this platform (%" PRIu64 " MB).",
                *new_config.cache_size, get_max_total_cache_size()));
        return;
    }
    if (my_config.get_ref().name != new_config.name) {
        logINF("Changed server's name from `%s` to `%s`.",
            my_config.get_ref().name.c_str(), new_config.name.c_str());
    }
    my_config.set_value(new_config);
    update_actual_cache_size(new_config.cache_size);
    {
        metadata_file_t::write_txn_t write_txn(file, interruptor);
        write_txn.write(mdkey_server_config(), new_config, interruptor);
    }
    send(mailbox_manager, ack_addr, std::string());
}

void server_config_server_t::update_actual_cache_size(
        const boost::optional<uint64_t> &setting) {
    uint64_t actual_size;
    if (!static_cast<bool>(setting)) {
        actual_size = get_default_total_cache_size();
        logINF("Automatically using cache size of %" PRIu64 " MB",
            actual_size / static_cast<uint64_t>(MEGABYTE));
    } else {
        if (*setting > get_max_total_cache_size()) {
            /* Usually this won't happen, because something else will reject the value
            before it gets to here. However, this can happen if a large cache size is
            recorded on disk and then the same data files are opened on a platform with a
            lower limit. I'm not sure if it's currently possible to open 64-bit metadata
            files on 32-bit platform, but better safe than sorry. */
            logWRN("The cache size is set to %" PRIu64 " MB, which is larger than the "
                "maximum legal value for this platform (%" PRIu64 " MB). The maximum "
                "legal value will be used as the actual cache size.",
                *setting, get_max_total_cache_size());
            actual_size = get_max_total_cache_size();
        } else {
            actual_size = *setting;
            logINF("Cache size is set to %" PRIu64 " MB",
                actual_size / static_cast<uint64_t>(MEGABYTE));
        }
    }
    log_warnings_for_cache_size(actual_size);
    actual_cache_size_bytes.set_value(actual_size);
}

