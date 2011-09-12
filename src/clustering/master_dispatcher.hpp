#ifndef __CLUSTERING_MASTER_DISPATCHER_HPP__
#define __CLUSTERING_MASTER_DISPATCHER_HPP__

template<class protocol_t>
class master_dispatcher_t {

public:
    typedef std::map<branch_id_t, resource_metadata_t<master_metadata_t<protocol_t> > > master_map_t;

    master_dispatcher_t(
            mailbox_cluster_t *c,
            boost::shared_ptr<metadata_read_view_t<master_map_t> > masters) :
        cluster(c),
        masters_view(masters)
        { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, order_token_t order_token) {
        std::vector<typename protocol_t::region_t> regions;
        std::vector<boost::shared_ptr<resource_access_t<master_metadata_t<protocol_t> > > > master_accesses;
        std::vector<typename protocol_t::read_t> subreads;
        {
            ASSERT_FINITE_CORO_WAITING;
            master_map_t masters = masters_view->get();
            for (master_map_t::iterator it = masters.begin(); it != masters.end(); it++) {
                try {
                    boost::shared_ptr<resource_access_t<master_metadata_t<protocol_t> > > access =
                        boost::make_shared<resource_access_t<master_metadata_t<protocol_t> > >(
                            cluster, metadata_member((*it).first, masters_view));
                } catch (resource_lost_exc_t) {
                    /* Ignore masters that have been shut down or are currently
                    inaccessible. */
                }
            }
        }
    }

private:
    mailbox_cluster_t *cluster;
    boost::shared_ptr<metadata_read_view_t<master_map_t> > masters_view;
};

#endif /* __CLUSTERING_MASTER_DISPATCHER_HPP__ */
