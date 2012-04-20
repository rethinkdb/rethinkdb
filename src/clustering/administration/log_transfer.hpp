#ifndef CLUSTERING_ADMINISTRATION_LOG_TRANSFER_HPP_
#define CLUSTERING_ADMINISTRATION_LOG_TRANSFER_HPP_

#include "clustering/administration/logger.hpp"
#include "clustering/resource.hpp"

class log_server_business_card_t {
public:
    typedef boost::variant<std::vector<log_message_t>, std::string> result_t;
    typedef mailbox_t<void(result_t)> result_mailbox_t;
    typedef mailbox_t<void(int, time_t, time_t, result_mailbox_t::address_t)> request_mailbox_t;

    log_server_business_card_t() { }
    log_server_business_card_t(const request_mailbox_t::address_t &a) : address(a) { }

    request_mailbox_t::address_t address;

    RDB_MAKE_ME_SERIALIZABLE_1(address);
};

class log_server_t {
public:
    log_server_t(mailbox_manager_t *mm, log_writer_t *w);
    log_server_business_card_t get_business_card();
private:
    void handle_request(
        int max_lines, time_t min_timestamp, time_t max_timestamp,
        log_server_business_card_t::result_mailbox_t::address_t cont, auto_drainer_t::lock_t keepalive);
    mailbox_manager_t *mailbox_manager;
    log_writer_t *writer;
    auto_drainer_t drainer;
    log_server_business_card_t::request_mailbox_t request_mailbox;
};

std::vector<log_message_t> fetch_log_file(
    mailbox_manager_t *mailbox_manager,
    const log_server_business_card_t &server_bcard,
    int max_entries, time_t min_timestamp, time_t max_timestamp,
    signal_t *interruptor) THROWS_ONLY(resource_lost_exc_t, std::runtime_error, interrupted_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_LOG_TRANSFER_HPP_ */
