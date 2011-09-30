#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_MASTER_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_MASTER_HPP__

#include "clustering/immediate_consistency/broadcaster.hpp"
#include "clustering/immediate_consistency/listener.hpp"
#include "clustering/immediate_consistency/metadata.hpp"

template<class protocol_t>
class master_t {

public:
    /* TODO: This constructor will leave half-constructed entries in the
    metadata during intermediate stages of construction. What can or should be
    done about this? If it's interrupted during construction, it may leave them
    that way forever. */

    master_t(
            mailbox_cluster_t *c,
            boost::shared_ptr<metadata_readwrite_view_t<namespace_metadata_t<protocol_t> > > namespace_metadata,
            store_view_t<protocol_t> *initial_store,
            signal_t *interruptor,
            boost::scoped_ptr<listener_t<protocol_t> > *initial_listener_out)
            THROWS_ONLY(interrupted_exc_t) :

        cluster(c),
        branch_id(generate_uuid())
    {
        /* Snapshot the starting point of the store; we'll need to record this
        and store it in the metadata. */
        region_map_t<protocol_t, version_range_t> origins =
            region_map_transform<protocol_t, binary_blob_t, version_range_t>(
                initial_store->begin_read_transaction(interruptor)->get_metadata(interruptor),
                &binary_blob_t::get<version_range_t>
            );

        /* Determine what the first timestamp of the new branch will be */
        state_timestamp_t initial_timestamp = state_timestamp_t::zero();
        std::vector<std::pair<typename protocol_t::region_t, version_range_t> > pairs = origins.get_as_pairs();
        for (int i = 0; i < (int)pairs.size(); i++) {
            state_timestamp_t part_timestamp = pairs[i].second.latest.timestamp;
            if (part_timestamp > initial_timestamp) {
                initial_timestamp = part_timestamp;
            }
        }

        /* Make an entry for this branch in the global metadata */
        {
            branch_metadata_t<protocol_t> our_metadata;
            our_metadata.region = initial_store->get_region();
            our_metadata.initial_timestamp = initial_timestamp;
            our_metadata.origins = origins;
            /* We'll fill in `master` and `broadcaster_registrar` later */

            std::map<branch_id_t, branch_metadata_t<protocol_t> > singleton;
            singleton[branch_id] = our_metadata;
            metadata_field(&namespace_metadata_t<protocol_t>::branches, namespace_metadata)->join(singleton);
        }

        boost::shared_ptr<metadata_readwrite_view_t<branch_metadata_t<protocol_t> > > branch_metadata =
            metadata_member(branch_id, metadata_field(&namespace_metadata_t<protocol_t>::branches, namespace_metadata));

        /* Create the broadcaster */
        broadcaster.reset(new broadcaster_t<protocol_t>(
            cluster,
            metadata_field(&branch_metadata_t<protocol_t>::broadcaster_registrar, branch_metadata),
            initial_timestamp
            ));

        /* Reset the store metadata. We should do this after making the branch
        entry in the global metadata so that we aren't left in a state where
        the store has been marked as belonging to a branch for which no
        information exists. */
        initial_store->begin_write_transaction(interruptor)->set_metadata(
            region_map_t<protocol_t, binary_blob_t>(
                initial_store->get_region(),
                binary_blob_t(version_range_t(version_t(branch_id, initial_timestamp)))
            ));

        /* Set up the initial listener. Note that we're invoking the special
        private `listener_t` constructor that doesn't perform a backfill. */
        initial_listener_out->reset(new listener_t<protocol_t>(
            cluster,
            namespace_metadata,
            initial_store,
            branch_id,
            interruptor));

        /* Set up the advertisement */
        read_mailbox.reset(new typename master_metadata_t<protocol_t>::read_mailbox_t(
            cluster, boost::bind(&master_t::on_read, this, _1, _2, _3, auto_drainer_t::lock_t(&drainer))));
        write_mailbox.reset(new typename master_metadata_t<protocol_t>::write_mailbox_t(
            cluster, boost::bind(&master_t::on_write, this, _1, _2, _3, auto_drainer_t::lock_t(&drainer))));
        advertisement.reset(new resource_advertisement_t<master_metadata_t<protocol_t> >(
            cluster,
            metadata_field(&branch_metadata_t<protocol_t>::master, branch_metadata),
            master_metadata_t<protocol_t>(read_mailbox->get_address(), write_mailbox->get_address())
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

    boost::scoped_ptr<broadcaster_t<protocol_t> > broadcaster;

    auto_drainer_t drainer;

    boost::scoped_ptr<typename master_metadata_t<protocol_t>::read_mailbox_t> read_mailbox;
    boost::scoped_ptr<typename master_metadata_t<protocol_t>::write_mailbox_t> write_mailbox;
    boost::scoped_ptr<resource_advertisement_t<master_metadata_t<protocol_t> > > advertisement;
};

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_MASTER_HPP__ */
