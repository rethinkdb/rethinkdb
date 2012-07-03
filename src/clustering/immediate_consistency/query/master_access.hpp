#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_ACCESS_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_ACCESS_HPP_

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/immediate_consistency/query/metadata.hpp"
#include "clustering/registrant.hpp"
#include "protocol_api.hpp"

template <class protocol_t>
class master_access_t : public home_thread_mixin_t {
public:
    master_access_t(
            mailbox_manager_t *mm,
            const clone_ptr_t<watchable_t<boost::optional<boost::optional<master_business_card_t<protocol_t> > > > > &master,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);

    typename protocol_t::region_t get_region() {
        return region;
    }

    signal_t *get_failed_signal() {
        return registrant->get_failed_signal();
    }

    void new_read_token(fifo_enforcer_sink_t::exit_read_t *out);

    typename protocol_t::read_response_t read(
            const typename protocol_t::read_t &read,
            order_token_t otok,
            fifo_enforcer_sink_t::exit_read_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t);

    void new_write_token(fifo_enforcer_sink_t::exit_write_t *out);

    typename protocol_t::write_response_t write(
            const typename protocol_t::write_t &write,
            order_token_t otok,
            fifo_enforcer_sink_t::exit_write_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t, cannot_perform_query_exc_t);

private:
    static boost::optional<boost::optional<registrar_business_card_t<master_access_business_card_t> > > extract_registrar_business_card(
            const boost::optional<boost::optional<master_business_card_t<protocol_t> > > &bcard) {
        if (bcard) {
            if (bcard.get()) {
                return boost::optional<boost::optional<registrar_business_card_t<master_access_business_card_t> > >(
                    boost::optional<registrar_business_card_t<master_access_business_card_t> >(
                        bcard.get().get().master_access_registration_business_card
                        ));
            } else {
                return boost::optional<boost::optional<registrar_business_card_t<master_access_business_card_t> > >(
                    boost::optional<registrar_business_card_t<master_access_business_card_t> >());
            }
        } else {
            return boost::optional<boost::optional<registrar_business_card_t<master_access_business_card_t> > >();
        }
    }

    void on_allocation(int);

    mailbox_manager_t *mailbox_manager;

    typename protocol_t::region_t region;
    typename master_business_card_t<protocol_t>::read_mailbox_t::address_t read_address;
    typename master_business_card_t<protocol_t>::write_mailbox_t::address_t write_address;

    fifo_enforcer_source_t internal_fifo_source;
    fifo_enforcer_sink_t internal_fifo_sink;

    master_access_id_t master_access_id;
    fifo_enforcer_source_t source_for_master;
    int allocated_writes;

    master_access_business_card_t::allocation_mailbox_t allocation_mailbox;
    boost::scoped_ptr<registrant_t<master_access_business_card_t> > registrant;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_ACCESS_HPP_ */
