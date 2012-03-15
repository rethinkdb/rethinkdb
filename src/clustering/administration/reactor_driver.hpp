#ifndef CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_
#define CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/reactor.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/mailbox/mailbox.hpp"

/* This files contains the class reactor driver whose job is to create and
 * destroy reactors based on blueprints given to the server. */

namespace reactor_driver_details {

/* The reactor driver is also responsible for the translation from
`persistable_blueprint_t` to `blueprint_t`. */
template<class protocol_t>
blueprint_t<protocol_t> translate_blueprint(const persistable_blueprint_t<protocol_t> &input, const clone_ptr_t<directory_rview_t<machine_id_t> > &translation_table) {
    blueprint_t<protocol_t> output;
    for (typename persistable_blueprint_t<protocol_t>::role_map_t::const_iterator it = input.machines_roles.begin();
            it != input.machines_roles.end(); it++) {
        peer_id_t peer = machine_id_to_peer_id(it->first, translation_table);
        if (peer.is_nil()) {
            /* We can't determine the peer ID that belongs or belonged to this
            peer because we can't reach the peer. So we generate a new peer ID
            for the peer. This works because either way, the `reactor_t` will
            be unable to reach the peer. */
            peer = peer_id_t(generate_uuid());
        }
        output.peers_roles[peer] = it->second;
    }
    return output;
}

    /* This is in part because these types aren't copyable so they can't go in
     * a std::pair. This class is used to hold a reactor and a watchable that
     * it's watching. */
    template <class protocol_t>
    class watchable_and_reactor_t {
    public:
        watchable_and_reactor_t(mailbox_manager_t *_mbox_manager, 
                                clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > _directory_view,
                                namespace_id_t _namespace_id,
                                boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > _branch_history,
                                const blueprint_t<protocol_t> &bp,
                                const std::string &_file_path)
            : watchable(bp),
              mbox_manager(_mbox_manager),
              directory_view(_directory_view),
              namespace_id(_namespace_id),
              branch_history(_branch_history),
              file_path(_file_path)
        {
            coro_t::spawn_sometime(boost::bind(&watchable_and_reactor_t<protocol_t>::initialize_reactor, this));
        }

        ~watchable_and_reactor_t() {
            /* Make sure that the coro we spawn to initialize this things has
             * actually run. */
            reactor_has_been_initialized.wait_lazily_unordered();

            /* The reactor must be destroyed before we remove the entry from
             * the directory map. C'est la vie. */
            reactor.reset();

            {
                directory_write_service_t::our_value_lock_acq_t lock(directory_view->get_directory_service());
                namespaces_directory_metadata_t<protocol_t> namespaces_directory = directory_view->get_our_value(&lock);
                namespaces_directory.master_maps.erase(namespace_id); //delete the entry;
                directory_view->set_our_value(namespaces_directory, &lock);
            }
        }

        std::string get_file_name() {
            return file_path + "/" + uuid_to_str(namespace_id);
        }

        watchable_variable_t<blueprint_t<protocol_t> > watchable;
    private:

        void initialize_reactor() {
            int res = access(get_file_name().c_str(), R_OK | W_OK);
            if (res == 0) {
                /* The file already exists thus we don't create it. */
                store.reset(new typename protocol_t::store_t(get_file_name(), false));
            } else {
                /* The file does not exist, create it. */
                store.reset(new typename protocol_t::store_t(get_file_name(), true));

                //Initialize the metadata in the underlying store
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> token;
                store->new_write_token(token);

                cond_t dummy_interruptor;
                store->set_metainfo(region_map_t<protocol_t, binary_blob_t>(store->get_region(), binary_blob_t(version_range_t(version_t::zero()))), token, &dummy_interruptor);
            }

            {
                directory_write_service_t::our_value_lock_acq_t lock(directory_view->get_directory_service());
                namespaces_directory_metadata_t<protocol_t> namespaces_directory = directory_view->get_our_value(&lock);
                namespaces_directory.master_maps[namespace_id]; //create an entry
                directory_view->set_our_value(namespaces_directory, &lock);
            }

            clone_ptr_t<directory_rwview_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > reactor_directory =
                directory_view->subview(field_lens(&namespaces_directory_metadata_t<protocol_t>::reactor_bcards))->subview(
                    optional_member_lens<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >(namespace_id));
            
            clone_ptr_t<directory_wview_t<std::map<master_id_t, master_business_card_t<protocol_t> > > > master_directory =
                directory_view->subview(field_lens(&namespaces_directory_metadata_t<protocol_t>::master_maps))->subview(
                    assumed_member_lens<namespace_id_t, std::map<master_id_t, master_business_card_t<protocol_t> > >(namespace_id));

            reactor.reset(new reactor_t<protocol_t>(mbox_manager, reactor_directory, master_directory, branch_history, watchable.get_watchable(), store.get()));

            reactor_has_been_initialized.pulse();
        }

        cond_t reactor_has_been_initialized;

        mailbox_manager_t *mbox_manager;
        clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > directory_view;
        namespace_id_t namespace_id;
        boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history;
        std::string file_path;

        boost::scoped_ptr<typename protocol_t::store_t> store;
        boost::scoped_ptr<reactor_t<protocol_t> > reactor;
    };
} //namespace reactor_driver_details

template <class protocol_t>
class reactor_driver_t {
public:
    reactor_driver_t(mailbox_manager_t *_mbox_manager,
                     clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > _directory_view,
                     boost::shared_ptr<semilattice_readwrite_view_t<namespaces_semilattice_metadata_t<protocol_t> > > _namespaces_view,
                     const clone_ptr_t<directory_rview_t<machine_id_t> > &_machine_id_translation_table,
                     std::string _file_path)
        : mbox_manager(_mbox_manager),
          directory_view(_directory_view), 
          branch_history(metadata_field(&namespaces_semilattice_metadata_t<protocol_t>::branch_history, _namespaces_view)),
          machine_id_translation_table(_machine_id_translation_table),
          namespaces_view(_namespaces_view),
          file_path(_file_path),
          semilattice_subscription(boost::bind(&reactor_driver_t<protocol_t>::on_change, this), namespaces_view),
          connectivity_subscription(boost::bind(&reactor_driver_t<protocol_t>::on_change, this), boost::bind(&reactor_driver_t<protocol_t>::on_change, this))
    {
        {
            /* We have to watch for peers connecting or disconnecting because
            that might change the translation from machine IDs to peer IDs. */
            connectivity_service_t::peers_list_freeze_t freeze(directory_view->get_directory_service()->get_connectivity_service());
            connectivity_subscription.reset(directory_view->get_directory_service()->get_connectivity_service(), &freeze);
        }
        on_change();
    }

private:
    typedef boost::ptr_map<namespace_id_t, reactor_driver_details::watchable_and_reactor_t<protocol_t> > reactor_map_t;
 
    void delete_reactor_data(auto_drainer_t::lock_t lock, typename reactor_map_t::auto_type *thing_to_delete) {
        lock.assert_is_holding(&drainer);
        delete thing_to_delete;
    }

    void on_change() {
        typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t namespaces = namespaces_view->get().namespaces;

        for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::const_iterator it =  namespaces.begin();
                                                                                                     it != namespaces.end();
                                                                                                     it++) {
            if (it->second.is_deleted() && std_contains(reactor_data, it->first)) {
                /* on_change cannot block because it is called as part of
                 * semilattice subscription, however the
                 * watchable_and_reactor_t destructor can block... therefore
                 * bullshit takes place. We must release a value from the
                 * ptr_map into this bullshit auto_type so that it's not in the
                 * map but the destructor hasn't been called... then this needs
                 * to be heap allocated so that it can be safely passed to a
                 * coroutine for destruction. */
                coro_t::spawn_sometime(boost::bind(&reactor_driver_t<protocol_t>::delete_reactor_data, this, auto_drainer_t::lock_t(&drainer), new typename reactor_map_t::auto_type(reactor_data.release(reactor_data.find(it->first)))));
            } else {
                persistable_blueprint_t<protocol_t> pbp;

                try {
                    pbp = it->second.get().blueprint.get();
                } catch (in_conflict_exc_t) {
                    //Nothing to do for this namespaces, its blueprint is in
                    //conflict.
                    continue;
                }

                blueprint_t<protocol_t> bp = reactor_driver_details::translate_blueprint(pbp, machine_id_translation_table);

                if (std_contains(bp.peers_roles, mbox_manager->get_connectivity_service()->get_me())) {
                    /* Either construct a new reactor (if this is a namespace we
                     * haven't seen before). Or send the new blueprint to the
                     * existing reactor. */
                    if (!std_contains(reactor_data, it->first)) {
                        namespace_id_t tmp = it->first;
                        reactor_data.insert(tmp, new reactor_driver_details::watchable_and_reactor_t<protocol_t>(mbox_manager,
                                    directory_view,
                                    it->first,
                                    branch_history,
                                    bp,
                                    file_path));
                    } else {
                        reactor_data.find(it->first)->second->watchable.set_value(bp);
                    }
                } else {
                    /* The blueprint does not mentions us so we destroy the
                     * reactor. */
                    if (std_contains(reactor_data, it->first)) {
                        coro_t::spawn_sometime(boost::bind(&reactor_driver_t<protocol_t>::delete_reactor_data, this, auto_drainer_t::lock_t(&drainer), new typename reactor_map_t::auto_type(reactor_data.release(reactor_data.find(it->first)))));
                    }
                }
            }
        }
    }

    mailbox_manager_t *mbox_manager;
    clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > directory_view;
    boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history;
    clone_ptr_t<directory_rview_t<machine_id_t> > machine_id_translation_table;
    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> > > namespaces_view;
    std::string file_path;

    boost::ptr_map<namespace_id_t, reactor_driver_details::watchable_and_reactor_t<protocol_t> > reactor_data;

    auto_drainer_t drainer;

    typename semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> >::subscription_t semilattice_subscription;
    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
};

#endif
