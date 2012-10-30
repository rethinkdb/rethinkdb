// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/log_transfer.hpp"

#include "concurrency/promise.hpp"
#include "containers/archive/boost_types.hpp"

RDB_IMPL_SERIALIZABLE_1(log_server_business_card_t, address);


log_server_t::log_server_t(mailbox_manager_t *mm, thread_pool_log_writer_t *lw) :
    mailbox_manager(mm), writer(lw),
    request_mailbox(mailbox_manager, boost::bind(&log_server_t::handle_request, this, _1, _2, _3, _4, auto_drainer_t::lock_t(&drainer)))
    { }

log_server_business_card_t log_server_t::get_business_card() {
    return log_server_business_card_t(request_mailbox.get_address());
}

void log_server_t::handle_request(int max_lines, struct timespec min_timestamp,
        struct timespec max_timestamp, log_server_business_card_t::result_mailbox_t::address_t cont,
        auto_drainer_t::lock_t keepalive) {
    std::string error;
    try {
        std::vector<log_message_t> messages =
            writer->tail(max_lines, min_timestamp, max_timestamp, keepalive.get_drain_signal());
        send(mailbox_manager, cont, boost::variant<std::vector<log_message_t>, std::string>(messages));
        return;
    } catch (std::runtime_error e) {
        error = e.what();
    } catch (interrupted_exc_t) {
        /* don't respond; we'll shut down in a moment */
        return;
    }
    /* Hack around the fact that we can't call a blocking function (e.g.
    `send()` from within a `catch`-block. */
    send(mailbox_manager, cont, boost::variant<std::vector<log_message_t>, std::string>(error));
}

std::vector<log_message_t> fetch_log_file(
        mailbox_manager_t *mm,
        const log_server_business_card_t &bcard,
        int max_lines, struct timespec min_timestamp, struct timespec max_timestamp,
        signal_t *interruptor) THROWS_ONLY(resource_lost_exc_t, std::runtime_error, interrupted_exc_t) {
    promise_t<boost::variant<std::vector<log_message_t>, std::string> > promise;
    log_server_business_card_t::result_mailbox_t reply_mailbox(
        mm,
        boost::bind(&promise_t<boost::variant<std::vector<log_message_t>, std::string> >::pulse, &promise, _1),
        mailbox_callback_mode_inline);
    disconnect_watcher_t dw(mm->get_connectivity_service(), bcard.address.get_peer());
    send(mm, bcard.address, max_lines, min_timestamp, max_timestamp, reply_mailbox.get_address());
    wait_any_t waiter(promise.get_ready_signal(), &dw);
    wait_interruptible(&waiter, interruptor);

    boost::variant<std::vector<log_message_t>, std::string> res;
    if (promise.try_get_value(&res)) {
        if (std::vector<log_message_t> *messages = boost::get<std::vector<log_message_t> >(&res)) {
            return *messages;
        } else {
            throw std::runtime_error(boost::get<std::string>(res));
        }
    } else {
        throw resource_lost_exc_t();
    }
}
