#include "clustering/generic/multi_client_client.hpp"

#include <functional>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/generic/registrant.hpp"
#include "containers/archive/boost_types.hpp"

template <class request_type>
multi_client_client_t<request_type>::multi_client_client_t(
        mailbox_manager_t *mm,
        const mc_business_card_t &server,
        signal_t *interruptor) :
    mailbox_manager(mm) {

    mailbox_t<void(server_business_card_t)> intro_mailbox(
        mailbox_manager,
        [&](signal_t *, const server_business_card_t &bc) {
            intro_promise.pulse(bc);
        });

    {
        const client_business_card_t client_business_card(intro_mailbox.get_address());

        registrant.init(new registrant_t<client_business_card_t>(mailbox_manager,
                                                                 server.registrar,
                                                                 client_business_card));
    }

    wait_interruptible(intro_promise.get_ready_signal(), interruptor);
}

template <class request_type>
void multi_client_client_t<request_type>::spawn_request(
        const request_type &request) {
    send(mailbox_manager, intro_promise.wait(), request);
}

template <class request_type>
boost::optional<boost::optional<registrar_business_card_t<typename multi_client_business_card_t<
            request_type>::client_business_card_t> > >
multi_client_client_t<request_type>::extract_registrar_business_card(
        const boost::optional<boost::optional<mc_business_card_t> > &bcard) {
    if (bcard) {
        if (bcard.get()) {
            return boost::make_optional(boost::make_optional(bcard.get().get().registrar));
        } else {
            return boost::make_optional(boost::optional<registrar_business_card_t<client_business_card_t> >());
        }
    }
    return boost::none;
}


#include "clustering/query_routing/metadata.hpp"

#include "rdb_protocol/protocol.hpp"
template class multi_client_client_t<primary_query_bcard_t::request_t>;
