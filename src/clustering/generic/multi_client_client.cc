#include "clustering/generic/multi_client_client.hpp"

#include <functional>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/generic/registrant.hpp"
#include "containers/archive/boost_types.hpp"

template <class request_type>
multi_client_client_t<request_type>::multi_client_client_t(
        mailbox_manager_t *mm,
        const clone_ptr_t<watchable_t<boost::optional<boost::optional<mc_business_card_t> > > > &server,
        signal_t *interruptor) :
    mailbox_manager(mm) {

    mailbox_t<void(server_business_card_t)> intro_mailbox(
        mailbox_manager,
        [&](signal_t *, const server_business_card_t &bc) {
            intro_promise.pulse(bc);
        });

    {
        const client_business_card_t client_business_card(intro_mailbox.get_address());

        // We have to type out this type in order to (a) scare you and (b) compile
        // under clang on OS X.  (Clang will crash if you try to pass the
        // server->subview(...) expression directly to the registrant_t
        // constructor.)  I haven't tried using "auto" for this variable, but see
        // reason (a).  It probably helps a lot to have this type plainly visible. sitting
        // here.

        clone_ptr_t<watchable_t<boost::optional<boost::optional<registrar_business_card_t<typename multi_client_business_card_t<request_type>::client_business_card_t> > > > > registrar_card_view
            = server->subview(&multi_client_client_t::extract_registrar_business_card);

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

template <class request_type>
signal_t *multi_client_client_t<request_type>::get_failed_signal() {
    return registrant->get_failed_signal();
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


#include "clustering/immediate_consistency/query/master_access.hpp"

#include "rdb_protocol/protocol.hpp"
template class multi_client_client_t<master_business_card_t::request_t>;
