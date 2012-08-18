#include "clustering/immediate_consistency/query/direct_reader.hpp"

#include "protocol_api.hpp"

template <class protocol_t>
direct_reader_t<protocol_t>::direct_reader_t(
        mailbox_manager_t *mm,
        multistore_ptr_t<protocol_t> *svs_) :
    mailbox_manager(mm),
    svs(svs_),
    read_mailbox(mm, boost::bind(&direct_reader_t<protocol_t>::on_read, this, _1, _2), mailbox_callback_mode_inline)
    { }

template <class protocol_t>
direct_reader_business_card_t<protocol_t> direct_reader_t<protocol_t>::get_business_card() {
    return direct_reader_business_card_t<protocol_t>(read_mailbox.get_address());
}

template <class protocol_t>
void direct_reader_t<protocol_t>::on_read(
        const typename protocol_t::read_t &read,
        const mailbox_addr_t<void(typename protocol_t::read_response_t)> &cont) {
    coro_t::spawn_sometime(boost::bind(
        &direct_reader_t<protocol_t>::perform_read, this,
        read, cont,
        auto_drainer_t::lock_t(&drainer)
        ));
}

template <class protocol_t>
void direct_reader_t<protocol_t>::perform_read(
        const typename protocol_t::read_t &read,
        const mailbox_addr_t<void(typename protocol_t::read_response_t)> &cont,
        auto_drainer_t::lock_t keepalive) {
    try {
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
        svs->new_read_token(&read_token);

#ifndef NDEBUG
        trivial_metainfo_checker_callback_t<protocol_t> metainfo_checker_callback;
        metainfo_checker_t<protocol_t> metainfo_checker(&metainfo_checker_callback, svs->get_multistore_joined_region());
#endif

        typename protocol_t::read_response_t response;
        svs->read(DEBUG_ONLY(metainfo_checker, )
                  read,
                  &response,
                  order_source.check_in("direct_reader_t::perform_read").with_read_mode(),
                  &read_token,
                  keepalive.get_drain_signal());

        send(mailbox_manager, cont, response);

    } catch (interrupted_exc_t) {
        /* ignore */
    }
}


#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class direct_reader_t<memcached_protocol_t>;
template class direct_reader_t<mock::dummy_protocol_t>;
template class direct_reader_t<rdb_protocol_t>;
