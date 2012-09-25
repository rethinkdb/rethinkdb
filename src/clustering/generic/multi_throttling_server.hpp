#ifndef CLUSTERING_GENERIC_MULTI_THROTTLING_SERVER_HPP_
#define CLUSTERING_GENERIC_MULTI_THROTTLING_SERVER_HPP_

#include <algorithm>

#include "arch/timing.hpp"
#include "clustering/generic/multi_throttling_metadata.hpp"
#include "clustering/generic/registrar.hpp"
#include "rpc/mailbox/typed.hpp"

template <class request_type, class inner_client_business_card_type, class user_data_type, class registrant_type>
class multi_throttling_server_t :
        private repeating_timer_callback_t {
private:
    typedef typename multi_throttling_business_card_t<request_type, inner_client_business_card_type>::server_business_card_t server_business_card_t;
    typedef typename multi_throttling_business_card_t<request_type, inner_client_business_card_type>::client_business_card_t client_business_card_t;

public:
    multi_throttling_server_t(
            mailbox_manager_t *mm,
            user_data_type ud,
            int capacity) :
        mailbox_manager(mm),
        user_data(ud),
        total_tickets(capacity), free_tickets(capacity),
        reallocate_timer(reallocate_interval_ms, this),
        registrar(mailbox_manager, this)
        { }

    multi_throttling_business_card_t<request_type, inner_client_business_card_type> get_business_card() {
        return multi_throttling_business_card_t<request_type, inner_client_business_card_type>(
            registrar.get_business_card());
    }

private:
    static const int reallocate_interval_ms = 1000;

    class client_t :
            public intrusive_list_node_t<client_t>,
            private repeating_timer_callback_t {
    public:
        client_t(multi_throttling_server_t *p,
                const client_business_card_t &client_bc) :
            parent(p),
            target_tickets(0), held_tickets(0), in_use_tickets(0),

            time_of_last_qps_sample(get_ticks()),
            requests_since_last_qps_sample(0),
            running_qps_estimate(0),
            qps_sample_timer(reallocate_interval_ms, this),

            give_tickets_addr(client_bc.give_tickets_addr),
            reclaim_tickets_addr(client_bc.reclaim_tickets_addr),

            registrant(p->user_data, client_bc.inner_client_business_card),

            drainer(new auto_drainer_t),

            request_mailbox(new mailbox_t<void(request_type)>(parent->mailbox_manager,
                boost::bind(&client_t::on_request, this, _1),
                mailbox_callback_mode_inline)),
            relinquish_tickets_mailbox(new mailbox_t<void(int)>(parent->mailbox_manager,
                boost::bind(&client_t::on_relinquish_tickets, this, _1),
                mailbox_callback_mode_inline))
        {
            send(parent->mailbox_manager, client_bc.intro_addr,
                 server_business_card_t(request_mailbox->get_address(),
                                        relinquish_tickets_mailbox->get_address()));
            parent->clients.push_back(this);
            parent->recompute_allocations();
        }

        ~client_t() {
            parent->clients.remove(this);
            parent->recompute_allocations();
            request_mailbox.reset();
            relinquish_tickets_mailbox.reset();
            drainer.reset();
            guarantee_reviewed(in_use_tickets == 0);
            parent->return_tickets(held_tickets);
        }

        void give_tickets(int tickets) {
            ASSERT_FINITE_CORO_WAITING;
            held_tickets += tickets;
            coro_t::spawn_sometime(boost::bind(
                &client_t::give_tickets_blocking, this,
                tickets,
                auto_drainer_t::lock_t(drainer.get())));
        }

        void set_target_tickets(int new_target) {
            if (target_tickets > new_target) {
                coro_t::spawn_sometime(boost::bind(
                    &client_t::reclaim_tickets_blocking, this,
                    target_tickets - new_target,
                    auto_drainer_t::lock_t(drainer.get())));
            }
            target_tickets = new_target;
        }

        int get_target_tickets() {
            return target_tickets;
        }

        int get_current_tickets() {
            return held_tickets + in_use_tickets;
        }

        int estimate_qps() {
            ticks_t time_span = get_ticks() - time_of_last_qps_sample;
            return running_qps_estimate / 2 +
                requests_since_last_qps_sample * secs_to_ticks(1) / time_span;
        }

    private:
        void on_request(const request_type &request) {
            guarantee_reviewed(held_tickets > 0);
            held_tickets--;
            in_use_tickets++;
            coro_t::spawn_sometime(boost::bind(
                &client_t::perform_request, this,
                request,
                auto_drainer_t::lock_t(drainer.get())));
        }

        void perform_request(const request_type &request, auto_drainer_t::lock_t keepalive) {
            requests_since_last_qps_sample++;
            try {
                registrant.perform_request(request, keepalive.get_drain_signal());
            } catch (interrupted_exc_t) {
                /* ignore */
            }
            in_use_tickets--;
            parent->return_tickets(1);
        }

        void on_relinquish_tickets(int tickets) {
            held_tickets -= tickets;
            parent->return_tickets(tickets);
        }

        void give_tickets_blocking(int tickets, auto_drainer_t::lock_t) {
            send(parent->mailbox_manager, give_tickets_addr, tickets);
        }

        void reclaim_tickets_blocking(int tickets, auto_drainer_t::lock_t) {
            send(parent->mailbox_manager, reclaim_tickets_addr, tickets);
        }

        void on_ring() {
            /* Take a sample of the recent average QPS */
            running_qps_estimate = estimate_qps();
            time_of_last_qps_sample = get_ticks();
            requests_since_last_qps_sample = 0;
        }

        multi_throttling_server_t *parent;

        int target_tickets, held_tickets, in_use_tickets;

        ticks_t time_of_last_qps_sample;
        int requests_since_last_qps_sample;
        int running_qps_estimate;
        repeating_timer_t qps_sample_timer;

        mailbox_addr_t<void(int)> give_tickets_addr;
        mailbox_addr_t<void(int)> reclaim_tickets_addr;

        registrant_type registrant;

        scoped_ptr_t<auto_drainer_t> drainer;

        scoped_ptr_t<mailbox_t<void(request_type)> > request_mailbox;
        scoped_ptr_t<mailbox_t<void(int)> > relinquish_tickets_mailbox;
    };

    void on_ring() {
        recompute_allocations();
    }

    void recompute_allocations() {
        /* We divide the total number of tickets into two pools. The first pool
        is distributed evenly among all the clients. The second pool is
        distributed in proportion to the clients' QPS. */
        static const float fair_fraction = 0.1;
        int fair_tickets = static_cast<int>(total_tickets * fair_fraction);
        int qps_tickets = total_tickets - fair_tickets;
        int total_qps = 0;
        for (client_t *c = clients.head(); c; c = clients.next(c)) {
            total_qps += c->estimate_qps();
        }
        if (clients.size() == 0) {
            return;
        }
        if (total_qps == 0) {
            /* None of the clients did any queries recently. Not all of the
            tickets will be distributed, but that's OK. */
            total_qps = 1;
        }
        for (client_t *c = clients.head(); c; c = clients.next(c)) {
            /* This math isn't exact, but it's OK if the target tickets of all
            the clients don't add up to `total_tickets`. */
            c->set_target_tickets(fair_tickets / clients.size() +
                                  qps_tickets * c->estimate_qps() / total_qps);
        }
        redistribute_tickets();
    }

    void return_tickets(int tickets) {
        free_tickets += tickets;
        guarantee_reviewed(free_tickets <= total_tickets);
        redistribute_tickets();
    }

    void redistribute_tickets() {
        static const int chunk_size = 100;
        static const int min_reasonable_tickets = 10;
        client_t *neediest;
        int gift_size;

        /* First, look for a client with a critically low number of tickets.
           They get priority in tickets. This prevents starvation. */
        while (free_tickets > 0) {
            gift_size = -1;
            neediest = NULL;
            for (client_t *c = clients.head(); c; c = clients.next(c)) {
                if (c->get_current_tickets() < min_reasonable_tickets && c->get_current_tickets() < c->get_target_tickets()) {
                    if (!neediest || c->get_current_tickets() < neediest->get_current_tickets()) {
                        neediest = c;
                        gift_size = std::min(c->get_target_tickets() - c->get_current_tickets(), free_tickets);
                    }
                }
            }

            if (!neediest) {
                break;
            }
            guarantee_reviewed(gift_size >= 0);
            free_tickets -= gift_size;
            neediest->give_tickets(gift_size);
        }

        /* Next, look for clients with a large difference between their target
           number of tickets and their current number of tickets. But if the 
           difference is less than `chunk_size`, don't send any tickets at all
           to avoid flooding the network with many small ticket updates. */
        while (free_tickets > chunk_size) {
            gift_size = -1;
            neediest = NULL;
            for (client_t *c = clients.head(); c; c = clients.next(c)) {
                int need_size = c->get_target_tickets() - c->get_current_tickets();
                if (need_size > chunk_size && (!neediest || need_size > neediest->get_target_tickets() - neediest->get_current_tickets())) {
                    neediest = c;
                    gift_size = chunk_size;
                }
            }

            if (!neediest) {
                break;
            }
            guarantee_reviewed(gift_size >= 0);
            free_tickets -= gift_size;
            neediest->give_tickets(gift_size);
        }
    }

    mailbox_manager_t *mailbox_manager;
    user_data_type user_data;

    intrusive_list_t<client_t> clients;
    int total_tickets, free_tickets;

    repeating_timer_t reallocate_timer;

    registrar_t<client_business_card_t, multi_throttling_server_t *, client_t> registrar;

private:
    DISABLE_COPYING(multi_throttling_server_t);
};

#endif /* CLUSTERING_GENERIC_MULTI_THROTTLING_SERVER_HPP_ */
