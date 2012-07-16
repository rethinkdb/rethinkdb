#ifndef CLUSTERING_ADMINISTRATION_PARSER_MAKER_TCC_
#define CLUSTERING_ADMINISTRATION_PARSER_MAKER_TCC_

#include "clustering/administration/parser_maker.hpp"

#include <string>

template<class protocol_t, class parser_t>
parser_maker_t<protocol_t, parser_t>::parser_maker_t(mailbox_manager_t *_mailbox_manager,
                               boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> > > _namespaces_semilattice_metadata,
                               DEBUG_ONLY(int _port_offset, )
                               namespace_repo_t<protocol_t> *_repo,
                               perfmon_collection_repo_t *_perfmon_collection_repo)
    : mailbox_manager(_mailbox_manager),
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
      DEBUG_ONLY(port_offset(_port_offset), )
      repo(_repo),
      namespaces_subscription(boost::bind(&parser_maker_t::on_change, this), namespaces_semilattice_metadata),
      perfmon_collection_repo(_perfmon_collection_repo)
{
    on_change();
}

template<class protocol_t>
int get_port(const namespace_semilattice_metadata_t<protocol_t> &ns
             DEBUG_ONLY(, int port_offset)
            ) {
#ifndef NDEBUG
    if (ns.port.get() == 0)
        return 0;

    return ns.port.get() + port_offset;
#else
    return ns.port.get();
#endif
}

static const char * ns_name_in_conflict = "<in conflict>";

template<class protocol_t, class parser_t>
void parser_maker_t<protocol_t, parser_t>::on_change() {
    ASSERT_NO_CORO_WAITING;

    namespaces_semilattice_metadata_t<protocol_t> snapshot = namespaces_semilattice_metadata->get();

    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::iterator it  = snapshot.namespaces.begin();
                                                                                           it != snapshot.namespaces.end();
                                                                                           it++) {
        typename boost::ptr_map<namespace_id_t, ns_record_t>::iterator handled_ns = namespaces_being_handled.find(it->first);

        /* Destroy parsers as necessary, by pulsing the `stopper` cond on the
        `ns_record_t` so that the `serve_queries()` coroutine will stop */
        if (handled_ns != namespaces_being_handled.end() && (
                          it->second.is_deleted() ||
                          it->second.get().port.in_conflict() ||
                          handled_ns->second->port != get_port(it->second.get()
                                                               DEBUG_ONLY(, port_offset))
                          )) {

            if (!handled_ns->second->stopper.is_pulsed()) {
                handled_ns->second->stopper.pulse();
            }
        }

        /* Create parsers as necessary by spawning instances of
        `serve_queries()` */
        if (handled_ns == namespaces_being_handled.end() && !it->second.is_deleted() && !it->second.get().port.in_conflict()) {
            vclock_t<std::string> v_ns_name = it->second.get().name;
            std::string ns_name(v_ns_name.in_conflict() ? ns_name_in_conflict : v_ns_name.get());

            int port = get_port(it->second.get()
                                DEBUG_ONLY(, port_offset)
                               );
            namespace_id_t tmp = it->first;
            namespaces_being_handled.insert(tmp, new ns_record_t(port));
            coro_t::spawn_sometime(boost::bind(
                &parser_maker_t::serve_queries, this,
                ns_name, it->first, port, auto_drainer_t::lock_t(&drainer)
                ));
        }
    }
}

template<class protocol_t, class parser_t>
void parser_maker_t<protocol_t, parser_t>::serve_queries(std::string ns_name, namespace_id_t ns, int port, auto_drainer_t::lock_t keepalive) {
    try {
        printf("Starting to serve queries for the namespace '%s' %s on port %d...\n", ns_name.c_str(), uuid_to_str(ns).c_str(), port);

        wait_any_t interruptor(&namespaces_being_handled.find(ns)->second->stopper, keepalive.get_drain_signal());
        typename namespace_repo_t<protocol_t>::access_t access(repo, ns, &interruptor);
        parser_t parser(port, access.get_namespace_if(), &perfmon_collection_repo->get_perfmon_collections_for_namespace(ns)->namespace_collection);
        interruptor.wait_lazily_unordered();

        printf("Stopping serving queries for the namespace '%s' %s on port %d...\n", ns_name.c_str(), uuid_to_str(ns).c_str(), port);
    } catch (interrupted_exc_t) {
        /* pass */
    }

    namespaces_being_handled.erase(ns);

    if (!keepalive.get_drain_signal()->is_pulsed()) {
        /* If we were stopped due to a port change, make sure that a new
        instance of `serve_queries()` gets started with the correct port */
        on_change();
    }
}

#endif /* CLUSTERING_ADMINISTRATION_PARSER_MAKER_TCC_ */
