#ifndef __CLUSTERING_MASTER_HPP__
#define __CLUSTERING_MASTER_HPP__

#include "clustering/namespace_metadata.hpp"
#include "clustering/mirror.hpp"
#include "clustering/mirror_dispatcher.hpp"

template<class protocol_t>
class master_t {

public:
    master_t(
            mailbox_cluster_t *c,
            typename protocol_t::store_t *initial_store,
            boost::shared_ptr<metadata_readwrite_view_t<namespace_metadata_t<protocol_t> > > namespace_view,
            signal_t *interruptor,
            mirror_t<protocol_t> **initial_mirror_out)
            THROWS_ONLY(interrupted_exc_t) :

        cluster(c),

        read_mailbox(cluster, boost::bind(&master_t::on_read, this,
            _1, _2, _3, auto_drainer_t::lock_t(&drainer))),
        write_mailbox(cluster, boost::bind(&master_t::on_write, this,
            _1, _2, _3, auto_drainer_t::lock_t(&drainer)))
    {
        rassert(!initial_store->is_backfilling());
        rassert(initial_store->is_coherent());

        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        /* Pick a branch ID */
        branch_id = generate_uuid();

        boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<protocol_t> > > mirror_dispatcher_view =
            metadata_new_member(branch_id, metadata_field(&namespace_metadata_t<protocol_t>::dispatchers, namespace_view));

        /* Set up the mirror dispatcher */
        mirror_dispatcher.reset(new mirror_dispatcher_t<protocol_t>(
            cluster,
            mirror_dispatcher_view,
            initial_store->get_timestamp()));

        /* Set up the first mirror */
        *initial_mirror_out = new mirror_t<protocol_t>(
            initial_store, cluster, mirror_dispatcher_view, interruptor);

        /* Set up the advertisement */
        advertisement.reset(new resource_advertisement_t<master_metadata_t<protocol_t> >(
            cluster,
            metadata_new_member(branch_id, metadata_field(&namespace_metadata_t<protocol_t>::masters, namespace_view)),
            master_metadata_t<protocol_t>(initial_store->get_region(), read_mailbox.get_address(), write_mailbox.get_address())
            ));
    }

private:
    void on_read(typename protocol_t::read_t read, order_token_t otok,
            typename async_mailbox_t<void(boost::variant<typename protocol_t::read_response_t, std::string>)>::address_t response_address,
            auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING
    {
        keepalive.assert_is_holding(&drainer);
        try {
            typename protocol_t::read_response_t response = mirror_dispatcher->read(read, otok);
            send(cluster, response_address, boost::variant<typename protocol_t::read_response_t, std::string>(response));
        } catch (typename mirror_dispatcher_t<protocol_t>::mirror_lost_exc_t e) {
            send(cluster, response_address, boost::variant<typename protocol_t::read_response_t, std::string>(std::string(e.what())));
        } catch (typename mirror_dispatcher_t<protocol_t>::insufficient_mirrors_exc_t e) {
            send(cluster, response_address, boost::variant<typename protocol_t::read_response_t, std::string>(std::string(e.what())));
        }
    }

    void on_write(typename protocol_t::write_t write, order_token_t otok,
            typename async_mailbox_t<void(boost::variant<typename protocol_t::write_response_t, std::string>)>::address_t response_address,
            auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING
    {
        keepalive.assert_is_holding(&drainer);
        try {
            typename protocol_t::write_response_t response = mirror_dispatcher->write(write, otok);
            send(cluster, response_address, boost::variant<typename protocol_t::write_response_t, std::string>(response));
        } catch (typename mirror_dispatcher_t<protocol_t>::mirror_lost_exc_t e) {
            send(cluster, response_address, boost::variant<typename protocol_t::write_response_t, std::string>(std::string(e.what())));
        } catch (typename mirror_dispatcher_t<protocol_t>::insufficient_mirrors_exc_t e) {
            send(cluster, response_address, boost::variant<typename protocol_t::write_response_t, std::string>(std::string(e.what())));
        }
    }

    mailbox_cluster_t *cluster;

    branch_id_t branch_id;
    boost::scoped_ptr<mirror_dispatcher_t<protocol_t> > mirror_dispatcher;

    auto_drainer_t drainer;

    typename master_metadata_t<protocol_t>::read_mailbox_t read_mailbox;
    typename master_metadata_t<protocol_t>::write_mailbox_t write_mailbox;

    boost::scoped_ptr<resource_advertisement_t<master_metadata_t<protocol_t> > > advertisement;
};

#endif /* __CLUSTERING_MASTER_HPP__ */
