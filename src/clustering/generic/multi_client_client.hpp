// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MULTI_CLIENT_CLIENT_HPP_
#define CLUSTERING_GENERIC_MULTI_CLIENT_CLIENT_HPP_

#include <algorithm>

#include "clustering/generic/multi_client_metadata.hpp"
#include "concurrency/promise.hpp"
#include "rpc/mailbox/typed.hpp"

template <class> class registrant_t;
template <class> class clone_ptr_t;
template <class> class watchable_t;

/* `multi_client_client_t` works with `multi_client_server_t`. On creation, it
obtains the address of a request mailbox from the server, which can be different
for each connecting client.
Requests can then be spawned through the `spawn_request` method on the client.

Since multi_client_client_t and multi_client_server_t are little more than wrappers
around a registrant_t and registrar_t respectively, they will likely be removed in
the future. */

template <class request_type>
class multi_client_client_t {
private:
    typedef multi_client_business_card_t<request_type>
        mc_business_card_t;
    typedef typename mc_business_card_t::server_business_card_t server_business_card_t;
    typedef typename mc_business_card_t::client_business_card_t client_business_card_t;

public:
    multi_client_client_t(
            mailbox_manager_t *mm,
            const clone_ptr_t<watchable_t<boost::optional<boost::optional<mc_business_card_t> > > > &server,
            signal_t *interruptor);

    signal_t *get_failed_signal();

    void spawn_request(const request_type &request);

private:
    static boost::optional<boost::optional<registrar_business_card_t<client_business_card_t> > >
    extract_registrar_business_card(
            const boost::optional<boost::optional<mc_business_card_t> > &bcard);

    mailbox_manager_t *const mailbox_manager;

    promise_t<server_business_card_t> intro_promise;

    auto_drainer_t drainer;

    scoped_ptr_t<registrant_t<typename multi_client_business_card_t<
        request_type>::client_business_card_t> > registrant;

    DISABLE_COPYING(multi_client_client_t);
};

#endif /* CLUSTERING_GENERIC_MULTI_CLIENT_CLIENT_HPP_ */
