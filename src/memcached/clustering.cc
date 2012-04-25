#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/administration/machine_metadata.hpp"
#include "containers/archive/order_token.hpp"
#include "memcached/clustering.hpp"
#include "memcached/tcp_conn.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"

memcached_parser_maker_t::memcached_parser_maker_t(mailbox_manager_t *_mailbox_manager,
                                                   boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > > _namespaces_semilattice_metadata,
#ifndef NDEBUG
                                                   boost::shared_ptr<semilattice_read_view_t<machine_semilattice_metadata_t> > _machine_semilattice_metadata,
#endif
                                                   clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<memcached_protocol_t> > > _namespaces_directory_metadata,
                                                   namespace_registry_t<memcached_protocol_t> *_namespace_repo)
    : mailbox_manager(_mailbox_manager),
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
#ifndef NDEBUG
      machine_semilattice_metadata(_machine_semilattice_metadata),
#endif
      namespaces_directory_metadata(_namespaces_directory_metadata),
#ifndef NDEBUG
      machines_subscription(boost::bind(&memcached_parser_maker_t::on_change, this), machine_semilattice_metadata),
#endif
      namespaces_subscription(boost::bind(&memcached_parser_maker_t::on_change, this), namespaces_semilattice_metadata),
      namespace_repo(_namespace_repo)
{
    on_change();
}

int get_port(const namespace_semilattice_metadata_t<memcached_protocol_t> &ns
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

void memcached_parser_maker_t::on_change() {
    namespaces_semilattice_metadata_t<memcached_protocol_t> snapshot = namespaces_semilattice_metadata->get();
#ifndef NDEBUG
    machine_semilattice_metadata_t machine_metadata_snapshot = machine_semilattice_metadata->get();
#endif

    for (namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator it  = snapshot.namespaces.begin();
                                                                                        it != snapshot.namespaces.end();
                                                                                        it++) {
        if (parsers.find(it->first) == parsers.end() && !it->second.is_deleted()) {
            int port = get_port(it->second.get()
#ifndef NDEBUG
                , machine_metadata_snapshot
#endif
                );

            //We're feeling lucky
            namespace_id_t tmp = it->first;
            parsers.insert(tmp, new parser_and_namespace_if_t(it->first, this, port));
            logINF("Setup an mc parser on %d", port);
        } else if (parsers.find(it->first) != parsers.end() && !it->second.is_deleted() &&
                   parsers.find(it->first)->second->parser.port != get_port(it->second.get()
#ifndef NDEBUG
                   , machine_metadata_snapshot
#endif
                   )) {
            coro_t::spawn_sometime(boost::bind(&delete_a_thing<memcached_parser_maker_t::parser_map_t::auto_type>, new memcached_parser_maker_t::parser_map_t::auto_type(parsers.release(parsers.find(it->first)))));

            int port = get_port(it->second.get()
#ifndef NDEBUG
                , machine_metadata_snapshot
#endif
                );

            //We're feeling lucky
            namespace_id_t tmp = it->first;
            parsers.insert(tmp, new parser_and_namespace_if_t(it->first, this, port));
            logINF("Setup an mc parser on %d", port);
        } else if (parsers.find(it->first) != parsers.end() && it->second.is_deleted()) {
            //The namespace has been deleted... get rid of the parser
            coro_t::spawn_sometime(boost::bind(&delete_a_thing<memcached_parser_maker_t::parser_map_t::auto_type>, new memcached_parser_maker_t::parser_map_t::auto_type(parsers.release(parsers.find(it->first)))));
        } else {
            //do nothing
        }
    }
}

//We need this typedef for a template below... this sort of sucks
typedef std::map<namespace_id_t, std::map<master_id_t, master_business_card_t<memcached_protocol_t> > > master_map_t;

memcached_parser_maker_t::parser_and_namespace_if_t::parser_and_namespace_if_t(namespace_id_t id, memcached_parser_maker_t *parent, int port)
    : namespace_if_access(parent->namespace_repo->get_namespace_if(id)),
      parser(port, namespace_if_access.namespace_if)
{ }

