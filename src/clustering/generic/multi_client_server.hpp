// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MULTI_CLIENT_SERVER_HPP_
#define CLUSTERING_GENERIC_MULTI_CLIENT_SERVER_HPP_

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "clustering/generic/multi_client_metadata.hpp"
#include "clustering/generic/registrar.hpp"
#include "rpc/mailbox/typed.hpp"

template <class request_type, class user_data_type, class registrant_type>
class multi_client_server_t {
private:
    typedef typename multi_client_business_card_t<request_type>::server_business_card_t server_business_card_t;
    typedef typename multi_client_business_card_t<request_type>::client_business_card_t client_business_card_t;

public:
    multi_client_server_t(
            mailbox_manager_t *mm,
            user_data_type ud) :
        mailbox_manager(mm),
        user_data(ud),
        registrar(mailbox_manager, this)
        { }

    multi_client_business_card_t<request_type> get_business_card() {
        return multi_client_business_card_t<request_type>(
            registrar.get_business_card());
    }

private:
    class client_t : public intrusive_list_node_t<client_t> {
    public:
        client_t(multi_client_server_t *p,
                 const client_business_card_t &client_bc) :
            parent(p),
            registrant(p->user_data),
            drainer(new auto_drainer_t),
            request_mailbox(new mailbox_t<void(request_type)>(parent->mailbox_manager,
                std::bind(&client_t::on_request, this, ph::_1, ph::_2)))
        {
            send(parent->mailbox_manager, client_bc.intro_addr,
                 server_business_card_t(request_mailbox->get_address()));
            parent->clients.push_back(this);
        }

        ~client_t() {
            parent->clients.remove(this);
            request_mailbox.reset();
            drainer.reset();
        }

    private:
        void on_request(signal_t *interruptor, const request_type &request) {
            try {
                registrant.perform_request(request, interruptor);
            } catch (const interrupted_exc_t &) {
                /* ignore */
            }
        }

        multi_client_server_t *parent;

        registrant_type registrant;

        scoped_ptr_t<auto_drainer_t> drainer;
        scoped_ptr_t<mailbox_t<void(request_type)> > request_mailbox;
    };

    mailbox_manager_t *const mailbox_manager;
    user_data_type user_data;

    intrusive_list_t<client_t> clients;

    registrar_t<client_business_card_t, multi_client_server_t *, client_t> registrar;

private:
    DISABLE_COPYING(multi_client_server_t);
};

#endif /* CLUSTERING_GENERIC_MULTI_CLIENT_SERVER_HPP_ */
