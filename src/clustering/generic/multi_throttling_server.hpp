// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MULTI_THROTTLING_SERVER_HPP_
#define CLUSTERING_GENERIC_MULTI_THROTTLING_SERVER_HPP_

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "arch/timing.hpp"
#include "containers/priority_queue.hpp"
#include "clustering/generic/multi_throttling_metadata.hpp"
#include "clustering/generic/registrar.hpp"
#include "math.hpp"
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
        goal_capacity(capacity), total_tickets(capacity), free_tickets(capacity),
        reallocate_timer(reallocate_interval_ms, this),
        registrar(mailbox_manager, this)
        { }

    multi_throttling_business_card_t<request_type, inner_client_business_card_type> get_business_card() {
        return multi_throttling_business_card_t<request_type, inner_client_business_card_type>(
            registrar.get_business_card());
    }

private:
    static const int reallocate_interval_ms = 1000;
    static const int fair_fraction_denom = 5;

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
                std::bind(&client_t::on_request, this, ph::_1, ph::_2))),
            relinquish_tickets_mailbox(new mailbox_t<void(int)>(parent->mailbox_manager,
                std::bind(&client_t::on_relinquish_tickets, this, ph::_1, ph::_2)))
        {
            send(parent->mailbox_manager, client_bc.intro_addr,
                 server_business_card_t(request_mailbox->get_address(),
                                        relinquish_tickets_mailbox->get_address()));
            parent->clients.push_back(this);
            parent->adjust_total_tickets();
            parent->recompute_allocations();
        }

        ~client_t() {
            parent->clients.remove(this);
            parent->adjust_total_tickets();
            parent->recompute_allocations();
            request_mailbox.reset();
            relinquish_tickets_mailbox.reset();
            drainer.reset();
            guarantee(in_use_tickets == 0);
            parent->return_tickets(held_tickets);
        }

        void give_tickets(int tickets) {
            ASSERT_FINITE_CORO_WAITING;
            held_tickets += tickets;
            coro_t::spawn_sometime(std::bind(
                &client_t::give_tickets_blocking, this,
                tickets,
                auto_drainer_t::lock_t(drainer.get())));
        }

        void set_target_tickets(int new_target) {
            if (target_tickets > new_target) {
                coro_t::spawn_sometime(std::bind(
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
            ticks_t time_span = std::max<ticks_t>(get_ticks() - time_of_last_qps_sample, 1);
            return running_qps_estimate / 2 +
                requests_since_last_qps_sample * secs_to_ticks(1) / time_span;
        }

    private:
        void on_request(signal_t *interruptor, const request_type &request) {
            guarantee(held_tickets > 0);
            held_tickets--;
            in_use_tickets++;

            requests_since_last_qps_sample++;
            try {
                registrant.perform_request(request, interruptor);
            } catch (const interrupted_exc_t &) {
                /* ignore */
            }
            in_use_tickets--;
            parent->return_tickets(1);
        }

        void on_relinquish_tickets(UNUSED signal_t *interruptor, int tickets) {
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
        int fair_tickets = std::max(static_cast<int>(clients.size()),
                total_tickets / fair_fraction_denom);
        int qps_tickets = total_tickets - fair_tickets;
        int total_qps = 0;
        for (client_t *c = clients.head(); c != NULL; c = clients.next(c)) {
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
        for (client_t *c = clients.head(); c != NULL; c = clients.next(c)) {
            /* This math isn't exact, but it's OK if the target tickets of all
            the clients don't add up to `total_tickets`. */
            c->set_target_tickets(fair_tickets / clients.size() +
                                  qps_tickets * c->estimate_qps() / total_qps);
        }
        redistribute_tickets();
    }

    void adjust_total_tickets() {
        /* If new clients connect, we adapt the total_tickets number, rather than
           just leaving it at goal_capacity.
           This serves two purposes:
           1. It makes sure that when a new client connect, we always have some
           free_ticket available to give to that client (note that clients here mean
           cluster nodes, not application clients).
           Otherwise new clients would have to wait until we send a relinquish_tickets
           message to one of the existing clients, then wait until that other client
           returns some of its tickets to us, which we could only then pass on to the
           newly connected client. The result would be a delay until a new client
           could actually process any query which we would like to avoid.
           2. If we have more clients than total_tickets/fair_fraction_denom, we would
           end up assigning 0 tickets to some clients. Those clients could never
           process any query. */

        /* So fair_tickets in recompute_allocations() is at least 1 per client. */
        int per_client_capacity = fair_fraction_denom;
        int new_total_tickets = goal_capacity + clients.size() * per_client_capacity;
        /* Note: This can temporarily make free_tickets negative */
        int diff = new_total_tickets - total_tickets;
        free_tickets += diff;
        total_tickets = new_total_tickets;
    }

    void return_tickets(int tickets) {
        free_tickets += tickets;
        redistribute_tickets();
    }

    void redistribute_tickets() {
        if (free_tickets <= 0 || clients.empty()) {
            return;
        }

        const int min_chunk_size = ceil_divide(100, static_cast<int>(clients.size()));
        const int min_reasonable_tickets = 10;

        {
            /* We cannot risk a client disconnecting while we are in here. That would
               invalidate the pointers in tickets_to_give. */
            ASSERT_NO_CORO_WAITING;
            std::map<client_t *, int> tickets_to_give;

            /* First, look for clients with a critically low number of tickets.
               They get priority in tickets. This prevents starvation. */
            std::vector<client_t *> critical_clients;
            critical_clients.reserve(clients.size());
            for (client_t *c = clients.head(); c != NULL; c = clients.next(c)) {
                if (c->get_current_tickets() < min_reasonable_tickets
                    && c->get_current_tickets() < c->get_target_tickets()) {
                    critical_clients.push_back(c);
                }
            }
            /* Distribute the available tickets among critical clients, up to a
               gift size of `min_reasonable_tickets`. As a consequence of the
               `ceil_divide()` in here we still set gift_size to 1 even if we don't
               have enough free tickets to give at least 1 to every critical client.
               That way we will at least give something to the first couple
               of clients.*/
            if (!critical_clients.empty()) {
                int gift_size_for_critical_clients = std::min(min_reasonable_tickets,
                        ceil_divide(free_tickets, critical_clients.size()));
                for (auto itr = critical_clients.begin(); itr != critical_clients.end(); ++itr) {
                    int tickets_client_actually_wants = std::max(0,
                        (*itr)->get_target_tickets() - (*itr)->get_current_tickets());
                    int gift_size = std::min(free_tickets,
                        std::min(tickets_client_actually_wants, gift_size_for_critical_clients));
                    free_tickets -= gift_size;
                    tickets_to_give[*itr] += gift_size;
                }
            }

            /* Next, look for clients with a large difference between their target
               number of tickets and their current number of tickets. But if the
               difference is less than `min_chunk_size`, don't send any tickets at all
               to avoid flooding the network with many small ticket updates. */
            priority_queue_t<std::pair<int, client_t *> > needy_clients;
            for (client_t *c = clients.head(); c != NULL; c = clients.next(c)) {
                int need_size = c->get_target_tickets()
                        - c->get_current_tickets()
                        - tickets_to_give[c];
                if (need_size >= min_chunk_size) {
                    needy_clients.push(std::pair<int, client_t *>(need_size, c));
                }
            }
            while (free_tickets >= min_chunk_size && !needy_clients.empty()) {
                std::pair<int, client_t *> neediest = needy_clients.pop();
                free_tickets -= min_chunk_size;
                tickets_to_give[neediest.second] += min_chunk_size;
                neediest.first -= min_chunk_size;
                if (neediest.first >= min_chunk_size) {
                    /* Re-insert the client so it gets more tickets later */
                    needy_clients.push(neediest);
                }
            }

            /* Now actually send the tickets to the clients */
            for (auto itr = tickets_to_give.begin(); itr != tickets_to_give.end(); ++itr) {
                if (itr->second > 0) {
                    itr->first->give_tickets(itr->second);
                }
            }
        }
    }

    mailbox_manager_t *const mailbox_manager;
    user_data_type user_data;

    intrusive_list_t<client_t> clients;
    int goal_capacity, total_tickets, free_tickets;

    repeating_timer_t reallocate_timer;

    registrar_t<client_business_card_t, multi_throttling_server_t *, client_t> registrar;

private:
    DISABLE_COPYING(multi_throttling_server_t);
};

#endif /* CLUSTERING_GENERIC_MULTI_THROTTLING_SERVER_HPP_ */
