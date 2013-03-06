#include "clustering/generic/multi_throttling_client.hpp"

#include "clustering/generic/registrant.hpp"
#include "containers/archive/boost_types.hpp"

template <class request_type, class inner_client_business_card_type>
multi_throttling_client_t<request_type, inner_client_business_card_type>::ticket_acq_t::ticket_acq_t(multi_throttling_client_t *p) : parent(p) {
    if (parent->free_tickets > 0) {
        state = state_acquired_ticket;
        parent->free_tickets--;
        pulse();
    } else {
        state = state_waiting_for_ticket;
        parent->ticket_queue.push_back(this);
    }
}

template <class request_type, class inner_client_business_card_type>
multi_throttling_client_t<request_type, inner_client_business_card_type>::ticket_acq_t::~ticket_acq_t() {
    switch (state) {
    case state_waiting_for_ticket:
        parent->ticket_queue.remove(this);
        break;
    case state_acquired_ticket:
        parent->free_tickets++;
        break;
    case state_used_ticket:
        break;
    default:
        unreachable();
    }
}

template <class request_type, class inner_client_business_card_type>
multi_throttling_client_t<request_type, inner_client_business_card_type>::multi_throttling_client_t(
        mailbox_manager_t *mm,
        const clone_ptr_t<watchable_t<boost::optional<boost::optional<mt_business_card_t> > > > &server,
        const inner_client_business_card_type &inner_client_business_card,
        signal_t *interruptor) :
    mailbox_manager(mm),
    free_tickets(0),
    give_tickets_mailbox(mailbox_manager,
        boost::bind(&multi_throttling_client_t::on_give_tickets, this, _1),
        mailbox_callback_mode_inline),
    reclaim_tickets_mailbox(mailbox_manager,
        boost::bind(&multi_throttling_client_t::on_reclaim_tickets, this, _1),
        mailbox_callback_mode_inline)
{
    mailbox_t<void(server_business_card_t)> intro_mailbox(
        mailbox_manager,
        boost::bind(&promise_t<server_business_card_t>::pulse, &intro_promise, _1),
        mailbox_callback_mode_inline);

    {
        const client_business_card_t client_business_card(inner_client_business_card,
                                                      intro_mailbox.get_address(),
                                                      give_tickets_mailbox.get_address(),
                                                      reclaim_tickets_mailbox.get_address());

        // We have to type out this type in order to (a) scare you and (b) compile
        // under clang on OS X.  (Clang will crash if you try to pass the
        // server->subview(...) expression directly to the registrant_t
        // constructor.)  I haven't tried using "auto" for this variable, but see
        // reason (a).  It probably helps a lot to have this type plainly visible. sitting
        // here.

        clone_ptr_t<watchable_t<boost::optional<boost::optional<registrar_business_card_t<typename multi_throttling_business_card_t<request_type, inner_client_business_card_type>::client_business_card_t> > > > > registrar_card_view
            = server->subview(&multi_throttling_client_t::extract_registrar_business_card);

        registrant.init(new registrant_t<client_business_card_t>(mailbox_manager,
                                                                 registrar_card_view,
                                                                 client_business_card));
    }

    wait_any_t waiter(intro_promise.get_ready_signal(), registrant->get_failed_signal(), interruptor);
    waiter.wait();
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    if (registrant->get_failed_signal()->is_pulsed()) {
        throw resource_lost_exc_t();
    }
    rassert(intro_promise.get_ready_signal()->is_pulsed());
}

template <class request_type, class inner_client_business_card_type>
multi_throttling_client_t<request_type, inner_client_business_card_type>::~multi_throttling_client_t() { }

template <class request_type, class inner_client_business_card_type>
signal_t *multi_throttling_client_t<request_type, inner_client_business_card_type>::get_failed_signal() {
    return registrant->get_failed_signal();
}

template <class request_type, class inner_client_business_card_type>
void multi_throttling_client_t<request_type, inner_client_business_card_type>::spawn_request(const request_type &request, ticket_acq_t *ticket_acq, signal_t *interruptor) {
    wait_interruptible(ticket_acq, interruptor);
    guarantee(ticket_acq->state == ticket_acq_t::state_acquired_ticket);
    ticket_acq->state = ticket_acq_t::state_used_ticket;
    send(mailbox_manager, intro_promise.wait().request_addr, request);
}

template <class request_type, class inner_client_business_card_type>
boost::optional<boost::optional<registrar_business_card_t<typename multi_throttling_business_card_t<request_type, inner_client_business_card_type>::client_business_card_t> > > multi_throttling_client_t<request_type, inner_client_business_card_type>::extract_registrar_business_card(const boost::optional<boost::optional<mt_business_card_t> > &bcard) {
    if (bcard) {
        if (bcard.get()) {
            return boost::make_optional(boost::make_optional(bcard.get().get().registrar));
        } else {
            return boost::make_optional(boost::optional<registrar_business_card_t<client_business_card_t> >());
        }
    }
    return boost::none;
}

template <class request_type, class inner_client_business_card_type>
void multi_throttling_client_t<request_type, inner_client_business_card_type>::on_give_tickets(int count) {
    while (count > 0 && !ticket_queue.empty()) {
        ticket_acq_t *lucky_winner = ticket_queue.head();
        ticket_queue.remove(lucky_winner);
        lucky_winner->state = ticket_acq_t::state_acquired_ticket;
        lucky_winner->pulse();
        count--;
    }
    free_tickets += count;
}

template <class request_type, class inner_client_business_card_type>
void multi_throttling_client_t<request_type, inner_client_business_card_type>::on_reclaim_tickets(int count) {
    int to_relinquish = std::min(count, free_tickets);
    if (to_relinquish > 0) {
        free_tickets -= to_relinquish;
        coro_t::spawn_sometime(boost::bind(&multi_throttling_client_t<request_type, inner_client_business_card_type>::relinquish_tickets_blocking, this,
                                           to_relinquish,
                                           auto_drainer_t::lock_t(&drainer)));
    }
}

template <class request_type, class inner_client_business_card_type>
void multi_throttling_client_t<request_type, inner_client_business_card_type>::relinquish_tickets_blocking(int count, auto_drainer_t::lock_t keepalive) {
    try {
        wait_interruptible(intro_promise.get_ready_signal(), keepalive.get_drain_signal());
        send(mailbox_manager, intro_promise.wait().relinquish_tickets_addr, count);
    } catch (const interrupted_exc_t &ex) {
        // Abandon all tickets, ship is going down!
    }
}


#include "clustering/immediate_consistency/query/master_access.hpp"

#include "mock/dummy_protocol.hpp"
template class multi_throttling_client_t<
    master_business_card_t<mock::dummy_protocol_t>::request_t,
    master_business_card_t<mock::dummy_protocol_t>::inner_client_business_card_t
    >;

#include "memcached/protocol.hpp"
template class multi_throttling_client_t<
    master_business_card_t<memcached_protocol_t>::request_t,
    master_business_card_t<memcached_protocol_t>::inner_client_business_card_t
    >;

#include "rdb_protocol/protocol.hpp"
template class multi_throttling_client_t<
    master_business_card_t<rdb_protocol_t>::request_t,
    master_business_card_t<rdb_protocol_t>::inner_client_business_card_t
    >;
