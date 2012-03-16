#include "errors.hpp"
#include <boost/variant/get.hpp>

#include "memcached/clustering.hpp"
#include "memcached/tcp_conn.hpp"
#include "memcached/store.hpp"

memcached_parser_maker_t::memcached_parser_maker_t(mailbox_manager_t *_mailbox_manager, 
                                                   boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > > _namespaces_semilattice_metadata,
                                                   clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<memcached_protocol_t> > > _namespaces_directory_metadata)
    : mailbox_manager(_mailbox_manager), 
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata), 
      namespaces_directory_metadata(_namespaces_directory_metadata),
      subscription(boost::bind(&memcached_parser_maker_t::on_change, this), namespaces_semilattice_metadata)
{
    on_change();
}

void memcached_parser_maker_t::on_change() {
    namespaces_semilattice_metadata_t<memcached_protocol_t> snapshot = namespaces_semilattice_metadata->get();

    for (namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator it  = snapshot.namespaces.begin();
                                                                                        it != snapshot.namespaces.end();
                                                                                        it++) {
        if (parsers.find(it->first) == parsers.end() && !it->second.is_deleted()) {
            int port = 10000 + rng_t().randint(55535);
            //We're feeling lucky
            namespace_id_t tmp = it->first;
            parsers.insert(tmp, new parser_and_namespace_if_t(it->first, this, port));
            logINF("Setup an mc parser on %d\n", port);
        } else if (parsers.find(it->first) != parsers.end() && it->second.is_deleted()) {
            //The namespace has been deleted... get rid of the parser
            parsers.erase(it->first);
        } else {
            //do nothing
        }
    }
}

//We need this typedef for a template below... this sort of sucks
typedef std::map<namespace_id_t, std::map<master_id_t, master_business_card_t<memcached_protocol_t> > > master_map_t;

memcached_parser_maker_t::parser_and_namespace_if_t::parser_and_namespace_if_t(namespace_id_t id, memcached_parser_maker_t *parent, int port) 
    : namespace_if(parent->mailbox_manager, 
                   parent->namespaces_directory_metadata->
                       subview<master_map_t>(field_lens(&namespaces_directory_metadata_t<memcached_protocol_t>::master_maps))->
                           subview(default_member_lens<master_map_t::key_type, master_map_t::mapped_type>(id))),
      parser(port, &namespace_if)
{ }

