#ifndef CLUSTERING_GENERIC_MULTI_THROTTLING_CLIENT_HPP_
#define CLUSTERING_GENERIC_MULTI_THROTTLING_CLIENT_HPP_

#include <algorithm>

#include "clustering/generic/multi_throttling_metadata.hpp"
#include "clustering/generic/registrant.hpp"
#include "concurrency/promise.hpp"
#include "rpc/mailbox/typed.hpp"

template <class request_type, class inner_client_business_card_type>
class multi_throttling_client_t {
private:
    typedef multi_throttling_business_card_t<request_type, inner_client_business_card_type> mt_business_card_t;
    typedef typename mt_business_card_t::server_business_card_t server_business_card_t;
    typedef typename mt_business_card_t::client_business_card_t client_business_card_t;

public:
    class ticket_acq_t : public signal_t, public intrusive_list_node_t<ticket_acq_t> {
    public:
        explicit ticket_acq_t(multi_throttling_client_t *p) : parent(p) {
            if (parent->free_tickets > 0) {
                state = state_acquired_ticket;
                parent->free_tickets--;
                pulse();
            } else {
                state = state_waiting_for_ticket;
                parent->ticket_queue.push_back(this);
            }
        }
        ~ticket_acq_t() {
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
    private:
        friend class multi_throttling_client_t;
        enum state_t {
            state_waiting_for_ticket,
            state_acquired_ticket,
            state_used_ticket
        };
        multi_throttling_client_t *parent;
        state_t state;
    };

    multi_throttling_client_t(
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
        promise_t<server_business_card_t> intro_promise;
        mailbox_t<void(server_business_card_t)> intro_mailbox(
            mailbox_manager,
            boost::bind(&promise_t<server_business_card_t>::pulse, &intro_promise, _1),
            mailbox_callback_mode_inline);
        registrant.init(new registrant_t<client_business_card_t>(
            mailbox_manager,
            server->subview(&multi_throttling_client_t::extract_registrar_business_card),
            client_business_card_t(
                inner_client_business_card,
                intro_mailbox.get_address(),
                give_tickets_mailbox.get_address(),
                reclaim_tickets_mailbox.get_address())));
        wait_any_t waiter(intro_promise.get_ready_signal(), registrant->get_failed_signal(), interruptor);
        waiter.wait();
        if (registrant->get_failed_signal()->is_pulsed()) {
            throw resource_lost_exc_t();
        }
        request_addr = intro_promise.get_value().request_addr;
        relinquish_tickets_addr = intro_promise.get_value().relinquish_tickets_addr;
    }

    signal_t *get_failed_signal() {
        return registrant->get_failed_signal();
    }

    void spawn_request(const request_type &request, ticket_acq_t *ticket_acq, signal_t *interruptor) {
        wait_interruptible(ticket_acq, interruptor);
        rassert_unreviewed(ticket_acq->state == ticket_acq_t::state_acquired_ticket);
        ticket_acq->state = ticket_acq_t::state_used_ticket;
        send(mailbox_manager, request_addr, request);
    }

private:
    static boost::optional<boost::optional<registrar_business_card_t<client_business_card_t> > > extract_registrar_business_card(
            const boost::optional<boost::optional<mt_business_card_t> > &bcard) {
        if (bcard) {
            if (bcard.get()) {
                return boost::make_optional(boost::make_optional(bcard.get().get().registrar));
            } else {
                return boost::make_optional(boost::optional<registrar_business_card_t<client_business_card_t> >());
            }
        }
        return boost::none;
    }

    void on_give_tickets(int count) {
        while (count > 0 && !ticket_queue.empty()) {
            ticket_acq_t *lucky_winner = ticket_queue.head();
            ticket_queue.remove(lucky_winner);
            lucky_winner->state = ticket_acq_t::state_acquired_ticket;
            lucky_winner->pulse();
            count--;
        }
        free_tickets += count;
    }

    void on_reclaim_tickets(int count) {
        int to_relinquish = std::min(count, free_tickets);
        if (to_relinquish > 0) {
            free_tickets -= to_relinquish;
            coro_t::spawn_sometime(boost::bind(
                &multi_throttling_client_t<request_type, inner_client_business_card_type>::relinquish_tickets_blocking, this,
                to_relinquish,
                auto_drainer_t::lock_t(&drainer)));
        }
    }

    void relinquish_tickets_blocking(int count, UNUSED auto_drainer_t::lock_t keepalive) {
        send(mailbox_manager, relinquish_tickets_addr, count);
    }

    mailbox_manager_t *mailbox_manager;

    mailbox_addr_t<void(request_type)> request_addr;
    mailbox_addr_t<void(int)> relinquish_tickets_addr;

    int free_tickets;
    intrusive_list_t<ticket_acq_t> ticket_queue;

    auto_drainer_t drainer;

    mailbox_t<void(int)> give_tickets_mailbox;
    mailbox_t<void(int)> reclaim_tickets_mailbox;

    scoped_ptr_t< registrant_t<
        typename multi_throttling_business_card_t<request_type, inner_client_business_card_type>::client_business_card_t
        > > registrant;
};

#endif /* CLUSTERING_GENERIC_MULTI_THROTTLING_CLIENT_HPP_ */
