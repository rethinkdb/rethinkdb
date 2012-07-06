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
        const int num_stores = svs->num_stores();
        boost::scoped_array<boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> > read_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t>[num_stores]);
        svs->new_read_tokens(read_tokens.get(), num_stores);

#ifndef NDEBUG
        class dummy_metainfo_checker_callback_t : public metainfo_checker_callback_t<protocol_t> {
        public:
            void check_metainfo(UNUSED const region_map_t<protocol_t, binary_blob_t>& metainfo,
                                UNUSED const typename protocol_t::region_t& domain) const {
                /* ignore */
            }
        } metainfo_checker_callback;
        metainfo_checker_t<protocol_t> metainfo_checker(&metainfo_checker_callback, svs->get_multistore_joined_region());
#endif

        typename protocol_t::read_response_t response = svs->read(
            DEBUG_ONLY(metainfo_checker, )
            read,
            order_token_t::ignore,
            read_tokens.get(),
            num_stores,
            keepalive.get_drain_signal());

        send(mailbox_manager, cont, response);

    } catch (interrupted_exc_t) {
        /* ignore */
    }
}


#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"

template class direct_reader_t<memcached_protocol_t>;
template class direct_reader_t<mock::dummy_protocol_t>;