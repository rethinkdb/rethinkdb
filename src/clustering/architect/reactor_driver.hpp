#ifndef __CLUSTERING_ARCHITECT_REACTOR_DRIVER_HPP__
#define __CLUSTERING_ARCHITECT_REACTOR_DRIVER_HPP__

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/architect/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/reactor.hpp"
#include "concurrency/watchable.hpp"
#include "mock/dummy_protocol.hpp"
#include "rpc/mailbox/mailbox.hpp"

/* This files contains the class reactor driver whose job is to create and
 * destroy reactors based on blueprints given to the server. */

namespace reactor_driver_details {
    /* This is in part because these types aren't copyable so they can't go in
     * a std::pair. This class is used to hold a reactor and a watchable that
     * it's watching. */
    template <class protocol_t>
    class watchable_and_reactor_t {
    public:
        watchable_and_reactor_t(mailbox_manager_t *mbox_manager, 
                                clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > _directory_view,
                                namespace_id_t _namespace_id,
                                boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history,
                                const blueprint_t<protocol_t> &bp)
            : watchable(bp),
              directory_view(_directory_view),
              namespace_id(_namespace_id)
        { 
            directory_write_service_t::our_value_lock_acq_t lock(directory_view->get_directory_service());
            namespaces_directory_metadata_t<protocol_t> namespaces_directory = directory_view->get_our_value(&lock);
            namespaces_directory.mastermaps[namespace_id]; //create an entry
            directory_view->set_our_value(namespaces_directory, &lock);

            clone_ptr_t<directory_rwview_t<std::map<namespace_id_t, reactor_business_card_t<protocol_t> > > > reactor_directory =
                directory_view->subview(field_lens(&namespaces_directory_metadata_t<protocol_t>::reactor_bcards)->subview(
                    optional_member_lens<namespace_id_t, reactor_business_card_t<protocol_t> >(namespace_id)));
            
            clone_ptr_t<directory_rwview_t<std::map<namespace_id_t, reactor_business_card_t<protocol_t> > > > master_directory =
                directory_view->subview(field_lens(&namespaces_directory_metadata_t<protocol_t>::master_maps)->subview(
                    assumed_member_lens<namespace_id_t, std::map<master_id_t, master_business_card_t<protocol_t> > >(namespace_id)));

            reactor.reset(new reactor_t<protocol_t>(mbox_manager, reactor_directory, master_directory, branch_history, &watchable, &store));
        }

        ~watchable_and_reactor_t() {
            /* The reactor must be destroyed before we remove the entry from
             * the directory map. C'est la vie. */
            reactor.reset();

            directory_write_service_t::our_value_lock_acq_t lock(directory_view->get_directory_service());
            namespaces_directory_metadata_t<protocol_t> namespaces_directory = directory_view->get_our_value(&lock);
            namespaces_directory.master_maps.erase(namespace_id); //delete the entry;
            directory_view->set_our_value(namespaces_directory, &lock);
        }

        watchable_impl_t<blueprint_t<protocol_t> > watchable;
    private:
        clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > directory_view;
        namespace_id_t namespace_id;
        mock::dummy_underlying_store_t store;
        boost::scoped_ptr<reactor_t<protocol_t> > reactor;
    };
} //namespace reactor_driver_details

template <class protocol_t>
class reactor_driver_t {
public:
    reactor_driver_t(mailbox_manager_t *_mbox_manager,
                     clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > _directory_view,
                     boost::shared_ptr<semilattice_readwrite_view_t<namespaces_semilattice_metadata_t<protocol_t> > > _namespaces_view)
        : mbox_manager(_mbox_manager),
          directory_view(_directory_view), 
          branch_history(metadata_field(&namespaces_semilattice_metadata_t<protocol_t>::branch_history, _namespaces_view)),
          namespaces_view(_namespaces_view) 
    { }

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
                coro_t::spawn_sometime(boost::bind(&reactor_driver_t<protocol_t>::delete_reactor_data, this, auto_drainer_t::lock_t(&drainer), new typename reactor_map_t::auto_type(reactor_data.release(it->first))));
            }
            if (reactor_data.find(it->first) == reactor_data.end()) {
                namespace_id_t tmp = it->first;
                reactor_data.insert(tmp, new reactor_driver_details::watchable_and_reactor_t<protocol_t>(mbox_manager,
                                                                                                         directory_view,
                                                                                                         it->first,
                                                                                                         branch_history,
                                                                                                         it->second));
            } else {
                reactor_data[it->first].watchable.set_value(it->second);
            }
        }
    }

    mailbox_manager_t *mbox_manager;
    clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > directory_view;
    boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history;

    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> > > namespaces_view;
    boost::ptr_map<namespace_id_t, reactor_driver_details::watchable_and_reactor_t<protocol_t> > reactor_data;

    auto_drainer_t drainer;

};

#endif
