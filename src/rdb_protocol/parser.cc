// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/parser.hpp"

#include <stdexcept>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
// #include "clustering/administration/namespace_metadata.hpp"
#include "rpc/semilattice/view.hpp"

namespace rdb_protocol {

query_http_app_t::query_http_app_t(const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata,
                                   namespace_repo_t<rdb_protocol_t> * _ns_repo)
    : semilattice_metadata(_semilattice_metadata), ns_repo(_ns_repo)
{ }

query_http_app_t::~query_http_app_t() {
    on_destruct.pulse();
}

http_res_t query_http_app_t::handle(const http_req_t &req) {
    try {
        switch (req.method) {
        case GET:
            {
                // /<uuid>/<key>
                rdb_protocol_t::read_t read;

                http_req_t::resource_t::iterator it = req.resource.begin();

                if (it == req.resource.end()) {
                    return http_res_t(HTTP_BAD_REQUEST, "text/plain", "No namespace specified");
                }

                uuid_u namespace_uuid;
                if (!str_to_uuid(*it, &namespace_uuid)) {
                    return http_res_t(HTTP_BAD_REQUEST, "text/plain", "Failed to parse namespace\n");
                }

                cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();

                if (!std_contains(cluster_metadata.rdb_namespaces->namespaces, namespace_uuid)) {
                    return http_res_t(HTTP_NOT_FOUND, "text/plain", "Didn't find namespace.");
                }

                ++it;

                if (it == req.resource.end()) {
                    return http_res_t(HTTP_BAD_REQUEST, "text/plain", "Key not specified");
                }

                rdb_protocol_t::read_response_t read_res;

                try {
                    namespace_repo_t<rdb_protocol_t>::access_t ns_access(ns_repo, namespace_uuid, &on_destruct);

                    store_key_t key(*it);
                    read.read = rdb_protocol_t::point_read_t(key);
                    ns_access.get_namespace_if()->read(read, &read_res, order_source.check_in("dummy parser"), &on_destruct);
                } catch (const interrupted_exc_t &) {
                    return http_res_t(HTTP_INTERNAL_SERVER_ERROR);
                }

                http_res_t res;

                rdb_protocol_t::point_read_response_t response = boost::get<rdb_protocol_t::point_read_response_t>(read_res.response);
                if (response.data) {
                    res.code = HTTP_OK;
                    res.set_body("application/json", response.data->as_json().Print());
                } else {
                    res.code = HTTP_NOT_FOUND;
                }
                return res;
            }
            break;
        case PUT:
        case POST:
            {
                rdb_protocol_t::write_t write;

                http_req_t::resource_t::iterator it = req.resource.begin();

                if (it == req.resource.end()) {
                    return http_res_t(HTTP_BAD_REQUEST, "text/plain", "Namespace not specified");
                }

                uuid_u namespace_uuid;
                if (!str_to_uuid(*it, &namespace_uuid)) {
                    return http_res_t(HTTP_BAD_REQUEST, "text/plain", "namespace uuid did not parse as uuid");
                }

                cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();

                if (!std_contains(cluster_metadata.rdb_namespaces->namespaces, namespace_uuid)) {
                    return http_res_t(HTTP_BAD_REQUEST);
                }

                ++it;

                if (it == req.resource.end()) {
                    return http_res_t(HTTP_BAD_REQUEST, "text/plain", "Key not specified");
                }

                store_key_t key(*it);

                std::shared_ptr<const scoped_cJSON_t> doc(new scoped_cJSON_t(cJSON_Parse(req.body.c_str())));

                if (!doc->get()) {
                    return http_res_t(HTTP_BAD_REQUEST, "text/plain", "Json failed to parse");
                }

                rdb_protocol_t::write_response_t write_res;

                try {
                    namespace_repo_t<rdb_protocol_t>::access_t ns_access(ns_repo, namespace_uuid, &on_destruct);

                    write.write = rdb_protocol_t::point_write_t(key, make_counted<ql::datum_t>(*doc));

                    ns_access.get_namespace_if()->write(write, &write_res, order_source.check_in("rdb parser"), &on_destruct);
                } catch (const interrupted_exc_t &) {
                    return http_res_t(HTTP_INTERNAL_SERVER_ERROR);
                }

                return http_res_t(HTTP_NO_CONTENT);
            }
            break;

        case HEAD:
        case DELETE:
        case TRACE:
        case OPTIONS:
        case CONNECT:
        case PATCH:
        default:
            return http_res_t(HTTP_BAD_REQUEST);
        }
        crash("Unreachable\n");
    } catch (const cannot_perform_query_exc_t &e) {
        http_res_t res;
        res.set_body("text/plain", e.what());
        return http_res_t(HTTP_INTERNAL_SERVER_ERROR);
    }
}

} // namespace rdb_protocol
