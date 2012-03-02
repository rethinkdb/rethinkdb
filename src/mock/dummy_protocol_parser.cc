#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "mock/dummy_protocol_parser.hpp"
#include "lens.hpp"

namespace mock {
http_access_t::http_access_t(namespace_interface_t<dummy_protocol_t> * _namespace_if, 
                            int _port)
    : http_server_t(_port), namespace_if(_namespace_if)
{ }

http_res_t http_access_t::handle(const http_req_t &req) {
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    typedef tokenizer::iterator tok_iterator;

    switch (req.method) {
    case GET:
        {
            dummy_protocol_t::read_t read;

            //setup a tokenizer
            boost::char_separator<char> sep("/");
            tokenizer tokens(req.resource, sep);
            tok_iterator it = tokens.begin();
            rassert(it != tokens.end());

            read.keys.keys.insert(*it);
            cond_t cond;
            dummy_protocol_t::read_response_t read_res = namespace_if->read(read, order_source.check_in("dummy parser"), &cond);

            http_res_t res;
            if (read_res.values.find(*it) != read_res.values.end()) {
                res.code = 200;
                res.set_body("application/text", read_res.values[*it]);
            } else {
                res.code = 404;
            }
            return res;
        }
        break;
    case PUT:
    case POST:
        {
            dummy_protocol_t::write_t write;

            //setup a tokenizer
            boost::char_separator<char> sep("/");
            tokenizer tokens(req.resource, sep);
            tok_iterator it = tokens.begin();
            rassert(it != tokens.end());

            write.values.insert(std::make_pair(*it, req.body));
            cond_t cond;
            dummy_protocol_t::write_response_t write_res = namespace_if->write(write, order_source.check_in("dummy parser"), &cond);

            return http_res_t(204);
        }
        break;
    case HEAD:
    case DELETE:
    case TRACE:
    case OPTIONS:
    case CONNECT:
    case PATCH:
    default:
        crash("Not implemented\n");
        break;
    }
    crash("Unreachable\n");
}

dummy_protocol_parser_maker_t::dummy_protocol_parser_maker_t(mailbox_manager_t *_mailbox_manager, 
                                                             boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> > > _namespaces_semilattice_metadata,
                                                             clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<mock::dummy_protocol_t> > > _namespaces_directory_metadata)
    : mailbox_manager(_mailbox_manager), 
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata), 
      namespaces_directory_metadata(_namespaces_directory_metadata),
      subscription(boost::bind(&dummy_protocol_parser_maker_t::on_change, this), namespaces_semilattice_metadata)
{
    on_change();
}
void dummy_protocol_parser_maker_t::on_change() {
    namespaces_semilattice_metadata_t<dummy_protocol_t> snapshot = namespaces_semilattice_metadata->get();

    for (namespaces_semilattice_metadata_t<dummy_protocol_t>::namespace_map_t::iterator it  = snapshot.namespaces.begin();
                                                                                        it != snapshot.namespaces.end();
                                                                                        it++) {
        if (parsers.find(it->first) == parsers.end() && !it->second.is_deleted()) {
            int port = 10000 + (random() % 10000);
            //We're feeling lucky
            namespace_id_t tmp = it->first;
            parsers.insert(tmp, new parser_and_namespace_if_t(it->first, this, port));
            logINF("Setup a parser on port: %d\n", port);
        } else if (parsers.find(it->first) != parsers.end() && it->second.is_deleted()) {
            //The namespace has been deleted... get rid of the parser
            parsers.erase(it->first);
        } else {
            //do nothing
        }
    }
}

//We need this typedef for a template below... this sort of sucks
typedef std::map<namespace_id_t, std::map<master_id_t, master_business_card_t<dummy_protocol_t> > > master_map_t;

dummy_protocol_parser_maker_t::parser_and_namespace_if_t::parser_and_namespace_if_t(namespace_id_t id, dummy_protocol_parser_maker_t *parent, int port) 
    : namespace_if(parent->mailbox_manager, 
                   parent->namespaces_directory_metadata->
                       subview<master_map_t>(field_lens(&namespaces_directory_metadata_t<dummy_protocol_t>::master_maps))->
                           subview(default_member_lens<master_map_t::key_type, master_map_t::mapped_type>(id))),
      parser(&namespace_if, port)
{ }

} //namespace mock 
