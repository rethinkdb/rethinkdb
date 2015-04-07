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
            //guarantee(in_use_tickets == 0);
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
            try {
                registrant.perform_request(request, interruptor);
            } catch (const interrupted_exc_t &) {
                /* ignore */
            }
        }

        void on_relinquish_tickets(UNUSED signal_t *interruptor, int tickets) {
        }

        void give_tickets_blocking(int tickets, auto_drainer_t::lock_t) {
            //send(parent->mailbox_manager, give_tickets_addr, tickets);
        }

        void reclaim_tickets_blocking(int tickets, auto_drainer_t::lock_t) {
            //send(parent->mailbox_manager, reclaim_tickets_addr, tickets);
        }

        void on_ring() {
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
    }

    void recompute_allocations() {
    }

    void adjust_total_tickets() {
    }

    void return_tickets(int tickets) {
    }

    void redistribute_tickets() {
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
