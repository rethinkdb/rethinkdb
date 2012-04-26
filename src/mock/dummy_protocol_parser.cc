#include "mock/dummy_protocol_parser.hpp"

#include "errors.hpp"
#include "containers/archive/order_token.hpp"
#include "lens.hpp"

namespace mock {

query_http_app_t::query_http_app_t(namespace_interface_t<dummy_protocol_t> * _namespace_if)
    : namespace_if(_namespace_if)
{ }

http_res_t query_http_app_t::handle(const http_req_t &req) {
    try {
        switch (req.method) {
            case GET:
            {
                dummy_protocol_t::read_t read;

                http_req_t::resource_t::iterator it = req.resource.begin();
                rassert(it != req.resource.end());

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

                http_req_t::resource_t::iterator it = req.resource.begin();
                rassert(it != req.resource.end());

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
    } catch(cannot_perform_query_exc_t) {
        return http_res_t(500);
    }
}

dummy_protocol_parser_maker_t::dummy_protocol_parser_maker_t(mailbox_manager_t *_mailbox_manager,
                                                             boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> > > _namespaces_semilattice_metadata,
                                                             namespace_repo_t<dummy_protocol_t> *_repo)
    : mailbox_manager(_mailbox_manager),
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
      repo(_repo),
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
            logINF("Setup a parser on %d", port);
        } else if (parsers.find(it->first) != parsers.end() && it->second.is_deleted()) {
            //The namespace has been deleted... get rid of the parser
            parsers.erase(it->first);
        } else {
            //do nothing
        }
    }
}

dummy_protocol_parser_maker_t::parser_and_namespace_if_t::parser_and_namespace_if_t(namespace_id_t id, dummy_protocol_parser_maker_t *parent, int port)
    : namespace_access(parent->repo, id),
      parser(namespace_access.get_namespace_if()),
      server(port, &parser)
{ }

} //namespace mock
