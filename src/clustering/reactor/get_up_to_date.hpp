#ifndef __CLUSTERING_REACTOR_GET_UP_TO_DATE_HPP__
#define __CLUSTERING_REACTOR_GET_UP_TO_DATE_HPP__

template<class protocol_t>
class get_up_to_date_t {
public:
    get_up_to_date_t(
             mailbox_manager_t *mm,
             store_view_t<protocol_t> *sv,
             clone_ptr_t<boost::optional<reactor_business_card_t<protocol_t> > > rbcs,
             region_mutex_t::acq_t *acq,
             signal_t *interruptor)
};

#endif /* __CLUSTERING_REACTOR_GET_UP_TO_DATE_HPP__ */
