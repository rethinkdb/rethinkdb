#ifndef CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_
#define CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_

#include <map>

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "concurrency/cross_thread_watchable.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/reactor/namespace_interface.hpp"

/* `namespace_repo_t` is responsible for constructing and caching
`cluster_namespace_interface_t` objects for all of the namespaces in the cluster
for a given protocol. Caching `cluster_namespace_interface_t` objects is
important because every time a new `cluster_namespace_interface_t` is created,
it must perform a handshake with every `master_t`, which means several network
round-trips. */

template <class protocol_t>
class namespace_repo_t : public home_thread_mixin_t {
    struct namespace_data_t {
        namespace_data_t() : refs(0), thread_data(get_num_threads()) { }

        ~namespace_data_t() {
            for (int i = 0; (size_t)i < thread_data.size(); ++i) {
                if (thread_data[i].ns_if != NULL) {
                    on_thread_t rethreader(i);
                    delete thread_data[i].ns_if;
                }
                delete thread_data[i].ct_subview;
            }
        }

        struct thread_data_t {
            thread_data_t() : ns_if(NULL), ct_subview(NULL) { }
            cluster_namespace_interface_t<protocol_t>* ns_if;
            cross_thread_watchable_variable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > >* ct_subview;
        };

        uint32_t refs;
        std::vector<thread_data_t> thread_data;
    };

public:
    namespace_repo_t(mailbox_manager_t *,
                     clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > >);

    struct access_t {
        access_t(namespace_repo_t *_parent, namespace_id_t _ns_id, signal_t *interruptor);
        ~access_t();

        cluster_namespace_interface_t<protocol_t> *get_namespace_if();

    private:
        void initialize_namespace_interface(clone_ptr_t<watchable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > > > subview,
                                            mailbox_manager_t *mailbox_manager,
                                            int thread);

        namespace_data_t *ns_data;
        namespace_repo_t *parent;
        namespace_id_t ns_id;
    };

private:
    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > namespaces_directory_metadata;

    std::map<namespace_id_t, namespace_data_t> interface_map;

    DISABLE_COPYING(namespace_repo_t);
};

#endif /* CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_ */
