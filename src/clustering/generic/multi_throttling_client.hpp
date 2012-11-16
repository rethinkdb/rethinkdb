// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MULTI_THROTTLING_CLIENT_HPP_
#define CLUSTERING_GENERIC_MULTI_THROTTLING_CLIENT_HPP_

#include <algorithm>

#include "clustering/generic/multi_throttling_metadata.hpp"
#include "concurrency/promise.hpp"
#include "rpc/mailbox/typed.hpp"

template <class> class registrant_t;
template <class> class clone_ptr_t;
template <class> class watchable_t;

template <class request_type, class inner_client_business_card_type>
class multi_throttling_client_t {
private:
    typedef multi_throttling_business_card_t<request_type, inner_client_business_card_type> mt_business_card_t;
    typedef typename mt_business_card_t::server_business_card_t server_business_card_t;
    typedef typename mt_business_card_t::client_business_card_t client_business_card_t;

public:
    class ticket_acq_t : public signal_t, public intrusive_list_node_t<ticket_acq_t> {
    public:
        explicit ticket_acq_t(multi_throttling_client_t *p);
        ~ticket_acq_t();
    private:
        friend class multi_throttling_client_t;
        enum state_t {
            state_waiting_for_ticket,
            state_acquired_ticket,
            state_used_ticket
        };
        multi_throttling_client_t *parent;
        state_t state;

        DISABLE_COPYING(ticket_acq_t);
    };

    multi_throttling_client_t(
            mailbox_manager_t *mm,
            const clone_ptr_t<watchable_t<boost::optional<boost::optional<mt_business_card_t> > > > &server,
            const inner_client_business_card_type &inner_client_business_card,
            signal_t *interruptor);
    ~multi_throttling_client_t();

    signal_t *get_failed_signal();

    void spawn_request(const request_type &request, ticket_acq_t *ticket_acq, signal_t *interruptor);

private:
    static boost::optional<boost::optional<registrar_business_card_t<client_business_card_t> > > extract_registrar_business_card(const boost::optional<boost::optional<mt_business_card_t> > &bcard);

    void on_give_tickets(int count);

    void on_reclaim_tickets(int count);

    void relinquish_tickets_blocking(int count, UNUSED auto_drainer_t::lock_t keepalive);

    mailbox_manager_t *const mailbox_manager;

    promise_t<server_business_card_t> intro_promise;

    int free_tickets;
    intrusive_list_t<ticket_acq_t> ticket_queue;

    auto_drainer_t drainer;

    mailbox_t<void(int)> give_tickets_mailbox;
    mailbox_t<void(int)> reclaim_tickets_mailbox;

    scoped_ptr_t< registrant_t<
        typename multi_throttling_business_card_t<request_type, inner_client_business_card_type>::client_business_card_t
        > > registrant;

    DISABLE_COPYING(multi_throttling_client_t);
};

#endif /* CLUSTERING_GENERIC_MULTI_THROTTLING_CLIENT_HPP_ */
