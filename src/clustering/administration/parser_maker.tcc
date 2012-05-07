#ifndef CLUSTERING_ADMINISTRATION_PARSER_MAKER_TCC_
#define CLUSTERING_ADMINISTRATION_PARSER_MAKER_TCC_

#include "clustering/administration/parser_maker.hpp"

template<class protocol_t, class parser_t>
parser_maker_t<protocol_t, parser_t>::parser_maker_t(mailbox_manager_t *_mailbox_manager,
                               boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> > > _namespaces_semilattice_metadata,
#ifndef NDEBUG
                               boost::shared_ptr<semilattice_read_view_t<machine_semilattice_metadata_t> > _machine_semilattice_metadata,
#endif
                               namespace_repo_t<protocol_t> *_repo)
    : mailbox_manager(_mailbox_manager),
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
#ifndef NDEBUG
      machine_semilattice_metadata(_machine_semilattice_metadata),
#endif
      repo(_repo),
      namespaces_subscription(boost::bind(&parser_maker_t::on_change, this), namespaces_semilattice_metadata),
#ifndef NDEBUG
      machine_subscription(boost::bind(&parser_maker_t::on_change, this), machine_semilattice_metadata)
#endif
{
    on_change();
}

template<class protocol_t>
int get_port(const namespace_semilattice_metadata_t<protocol_t> &ns
#ifndef NDEBUG
    , const machine_semilattice_metadata_t &us
#endif
    ) {
#ifndef NDEBUG
    return ns.port.get() + us.port_offset.get();
#else
    return ns.port.get();
#endif
}

template<class protocol_t, class parser_t>
void parser_maker_t<protocol_t, parser_t>::on_change() {
    ASSERT_NO_CORO_WAITING;

    namespaces_semilattice_metadata_t<protocol_t> snapshot = namespaces_semilattice_metadata->get();
#ifndef NDEBUG
    machine_semilattice_metadata_t machine_metadata_snapshot = machine_semilattice_metadata->get();
#endif

    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::iterator it  = snapshot.namespaces.begin();
                                                                                           it != snapshot.namespaces.end();
                                                                                           it++) {
        typename boost::ptr_map<namespace_id_t, ns_record_t>::iterator h_it = namespaces_being_handled.find(it->first);

        /* Destroy parsers as necessary, by pulsing the `stopper` cond on the
        `ns_record_t` so that the `serve_queries()` coroutine will stop */
        if (h_it != namespaces_being_handled.end() && (
                it->second.is_deleted() ||
                it->second.get().port.in_conflict() ||
                h_it->second->port != get_port(it->second.get()
#ifndef NDEBUG
                                               , machine_metadata_snapshot
#endif
                ))) {
            if (!h_it->second->stopper.is_pulsed()) {
                h_it->second->stopper.pulse();
            }
        }

        /* Create parsers as necessary by spawning instances of
        `serve_queries()` */
        if (h_it == namespaces_being_handled.end() && !it->second.is_deleted() && !it->second.get().port.in_conflict()) {
            int port = get_port(it->second.get()
#ifndef NDEBUG
                                , machine_metadata_snapshot
#endif
                                );
            namespace_id_t tmp = it->first;
            namespaces_being_handled.insert(tmp, new ns_record_t(port));
            coro_t::spawn_sometime(boost::bind(
                &parser_maker_t::serve_queries, this,
                it->first, port, auto_drainer_t::lock_t(&drainer)
                ));
        }
    }
}

template<class protocol_t, class parser_t>
void parser_maker_t<protocol_t, parser_t>::serve_queries(namespace_id_t ns, int port, auto_drainer_t::lock_t keepalive) {
    try {
        wait_any_t interruptor(&namespaces_being_handled.find(ns)->second->stopper, keepalive.get_drain_signal());
        typename namespace_repo_t<protocol_t>::access_t access(repo, ns, &interruptor);
        parser_t parser(port, access.get_namespace_if());
        interruptor.wait_lazily_unordered();
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
