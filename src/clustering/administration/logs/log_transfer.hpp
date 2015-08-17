// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_LOGS_LOG_TRANSFER_HPP_
#define CLUSTERING_ADMINISTRATION_LOGS_LOG_TRANSFER_HPP_

#include <string>
#include <vector>

#include "clustering/administration/logs/log_writer.hpp"

class log_server_business_card_t {
public:
    typedef mailbox_t<void(boost::variant<std::vector<log_message_t>, std::string>)> result_mailbox_t;
    typedef mailbox_t<void(int, struct timespec, struct timespec, result_mailbox_t::address_t)> request_mailbox_t;

    log_server_business_card_t() { }
    explicit log_server_business_card_t(const request_mailbox_t::address_t &a) : address(a) { }

    request_mailbox_t::address_t address;
};

RDB_DECLARE_SERIALIZABLE(log_server_business_card_t);

RDB_MAKE_EQUALITY_COMPARABLE_1(log_server_business_card_t, address);

class log_server_t {
public:
    log_server_t(mailbox_manager_t *mm, thread_pool_log_writer_t *w);
    log_server_business_card_t get_business_card();
private:
    void handle_request(
        signal_t *interruptor,
        int max_lines, struct timespec min_timestamp, struct timespec max_timestamp,
        log_server_business_card_t::result_mailbox_t::address_t cont);
    mailbox_manager_t *mailbox_manager;
    thread_pool_log_writer_t *writer;
    log_server_business_card_t::request_mailbox_t request_mailbox;

private:
    DISABLE_COPYING(log_server_t);
};

/* Fetches a block of entries from another server's log file. It examines the last
`max_entries` entries from the server's log, and filters out those whose timestamps are
not between `min_timestamp` and `max_timestamp`, inclusive. It returns the results in
reverse chronological order. Throws `log_transfer_exc_t` if the server disconnected or
`log_read_exc_t` if there's a problem with reading the log file. */
class log_transfer_exc_t : public std::runtime_error {
public:
    log_transfer_exc_t() : std::runtime_error(
        "Lost contact with server while trying to transfer log messages") { }
};
std::vector<log_message_t> fetch_log_file(
    mailbox_manager_t *mailbox_manager,
    const log_server_business_card_t &server_bcard,
    int max_entries,
    struct timespec min_timestamp,
    struct timespec max_timestamp,
    signal_t *interruptor)
    THROWS_ONLY(log_transfer_exc_t, log_read_exc_t, interrupted_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_LOGS_LOG_TRANSFER_HPP_ */
