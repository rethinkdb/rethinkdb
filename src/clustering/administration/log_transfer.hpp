#ifndef CLUSTERING_ADMINISTRATION_LOG_TRANSFER_HPP_
#define CLUSTERING_ADMINISTRATION_LOG_TRANSFER_HPP_

#include <string>
#include <vector>

#include "clustering/administration/logger.hpp"
#include "clustering/generic/resource.hpp"

class log_server_business_card_t {
public:
    typedef mailbox_t<void(boost::variant<std::vector<log_message_t>, std::string>)> result_mailbox_t;
    typedef mailbox_t<void(int, struct timespec, struct timespec, result_mailbox_t::address_t)> request_mailbox_t;

    log_server_business_card_t() { }
    explicit log_server_business_card_t(const request_mailbox_t::address_t &a) : address(a) { }

    request_mailbox_t::address_t address;
};

RDB_DECLARE_SERIALIZABLE(log_server_business_card_t);

class log_server_t {
public:
    log_server_t(mailbox_manager_t *mm, thread_pool_log_writer_t *w);
    log_server_business_card_t get_business_card();
private:
    void handle_request(
        int max_lines, struct timespec min_timestamp, struct timespec max_timestamp,
        log_server_business_card_t::result_mailbox_t::address_t cont, auto_drainer_t::lock_t keepalive);
    mailbox_manager_t *mailbox_manager;
    thread_pool_log_writer_t *writer;
    auto_drainer_t drainer;
    log_server_business_card_t::request_mailbox_t request_mailbox;

private:
    DISABLE_COPYING(log_server_t);
};

std::vector<log_message_t> fetch_log_file(
    mailbox_manager_t *mailbox_manager,
    const log_server_business_card_t &server_bcard,
    int max_entries, struct timespec min_timestamp, struct timespec max_timestamp,
    signal_t *interruptor) THROWS_ONLY(resource_lost_exc_t, std::runtime_error, interrupted_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_LOG_TRANSFER_HPP_ */
