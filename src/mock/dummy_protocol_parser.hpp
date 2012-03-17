#ifndef MOCK_DUMMY_PROTOCOL_PARSER_HPP_
#define MOCK_DUMMY_PROTOCOL_PARSER_HPP_

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "http/http.hpp"
#include "mock/dummy_protocol.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/view.hpp"

namespace mock {

class query_http_app_t : public http_app_t {
public:
    explicit query_http_app_t(namespace_interface_t<dummy_protocol_t> * _namespace_if);
    http_res_t handle(const http_req_t &);

private:
    namespace_interface_t<dummy_protocol_t> *namespace_if; 
    order_source_t order_source;
};

class dummy_protocol_parser_maker_t {
public:
    dummy_protocol_parser_maker_t(mailbox_manager_t *, 
                                  boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> > >,
                                  clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<mock::dummy_protocol_t> > >);

private:
    void on_change();

    mailbox_manager_t *mailbox_manager;
    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> > > namespaces_semilattice_metadata;
    clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<mock::dummy_protocol_t> > > namespaces_directory_metadata;

    struct parser_and_namespace_if_t {
        parser_and_namespace_if_t(namespace_id_t, dummy_protocol_parser_maker_t *parent, int port);

        cluster_namespace_interface_t<dummy_protocol_t> namespace_if;
        query_http_app_t parser;
        http_server_t server;
    };
    boost::ptr_map<namespace_id_t, parser_and_namespace_if_t> parsers;

    semilattice_read_view_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::subscription_t subscription;
};

}//namespace mock 

#endif
