// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/logs/log_transfer.hpp"

#include <functional>

#include "concurrency/promise.hpp"
#include "containers/archive/boost_types.hpp"

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(log_server_business_card_t, address);


log_server_t::log_server_t(mailbox_manager_t *mm, thread_pool_log_writer_t *lw) :
    mailbox_manager(mm), writer(lw),
    request_mailbox(mailbox_manager, std::bind(&log_server_t::handle_request, this, ph::_1, ph::_2, ph::_3, ph::_4, ph::_5))
    { }

log_server_business_card_t log_server_t::get_business_card() {
    return log_server_business_card_t(request_mailbox.get_address());
}

void log_server_t::handle_request(
        signal_t *interruptor,
        int max_lines,
        timespec min_timestamp,
        timespec max_timestamp,
        log_server_business_card_t::result_mailbox_t::address_t cont) {
    std::string error;
    try {
        std::vector<log_message_t> messages =
            writer->tail(max_lines, min_timestamp, max_timestamp, interruptor);
        send(mailbox_manager, cont, boost::variant<std::vector<log_message_t>, std::string>(messages));
        return;
    } catch (const log_read_exc_t &e) {
        error = e.what();
    } catch (const interrupted_exc_t &) {
        /* don't respond; we'll shut down in a moment */
        return;
    }
    /* Hack around the fact that we can't call a blocking function (e.g.
    `send()` from within a `catch`-block. */
    send(mailbox_manager, cont, boost::variant<std::vector<log_message_t>, std::string>(error));
}  // NOLINT(whitespace/indent)  (cpplint is confused by the 'struct timespec')

std::vector<log_message_t> fetch_log_file(
        mailbox_manager_t *mm,
        const log_server_business_card_t &bcard,
        int max_lines, timespec min_timestamp, timespec max_timestamp,
        signal_t *interruptor) THROWS_ONLY(log_transfer_exc_t, log_read_exc_t, interrupted_exc_t) {
    promise_t<boost::variant<std::vector<log_message_t>, std::string> > promise;
    log_server_business_card_t::result_mailbox_t reply_mailbox(
        mm,
        [&](signal_t *,
                const boost::variant<std::vector<log_message_t>, std::string> &r) {
            promise.pulse(r);
        });
    send(mm, bcard.address, max_lines, min_timestamp, max_timestamp, reply_mailbox.get_address());
    {
        disconnect_watcher_t dw(mm, bcard.address.get_peer());
        wait_any_t waiter(promise.get_ready_signal(), &dw);
        wait_interruptible(&waiter, interruptor);
    }

    boost::variant<std::vector<log_message_t>, std::string> res;
    if (promise.try_get_value(&res)) {
        if (std::vector<log_message_t> *messages = boost::get<std::vector<log_message_t> >(&res)) {
            return *messages;
        } else {
            throw log_read_exc_t(boost::get<std::string>(res));
        }
    } else {
        throw log_transfer_exc_t();
    }
}
