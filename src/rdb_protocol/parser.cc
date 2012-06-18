#include "rdb_protocol/parser.hpp"

#include <stdexcept>

#include "utils.hpp"


namespace rdb_protocol {

query_http_app_t::query_http_app_t(const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata,
                                   namespace_repo_t<rdb_protocol_t> * _ns_repo)
    : semilattice_metadata(_semilattice_metadata), ns_repo(_ns_repo)
{ }

http_res_t query_http_app_t::handle(const http_req_t &req) {
    try {
        switch (req.method) {
        case GET:
            {
                // /<uuid>/<key>
                rdb_protocol_t::read_t read;

                http_req_t::resource_t::iterator it = req.resource.begin();

                if (it == req.resource.end()) {
                    return http_res_t(400, "application/text", "No namespace specified");
                }

                boost::uuids::uuid namespace_uuid;
                try {
                    namespace_uuid = str_to_uuid(*it);
                } catch (std::runtime_error) {
                    return http_res_t(400, "application/text", "Failed to parse namespace\n");
                }

                cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();

                if (!std_contains(cluster_metadata.rdb_namespaces.namespaces, namespace_uuid)) {
                    return http_res_t(404, "application/text", "Didn't find namespace.");
                }

                ++it;

                if (it == req.resource.end()) {
                    return http_res_t(400, "application/text", "Key not specified");
                }

                cond_t interrupt;
                namespace_repo_t<rdb_protocol_t>::access_t ns_access(ns_repo, namespace_uuid, &interrupt);

                store_key_t key(*it);
                read.read = rdb_protocol_t::point_read_t(key);
                cond_t cond;
                rdb_protocol_t::read_response_t read_res = ns_access.get_namespace_if()->read(read, order_source.check_in("dummy parser"), &cond);

                http_res_t res;

                rdb_protocol_t::point_read_response_t response = boost::get<rdb_protocol_t::point_read_response_t>(read_res.response);
                if (response.data) {
                    boost::scoped_ptr<char> json(cJSON_Print(response.data.get()->get()));
                    res.code = 200;
                    res.set_body("application/json", json.get());
                } else {
                    res.code = 404;
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
                    return http_res_t(400, "application/text", "Namespace not specified");
                }

                boost::uuids::uuid namespace_uuid;
                try {
                    namespace_uuid = str_to_uuid(*it);
                } catch (std::runtime_error) {
                    return http_res_t(400, "application/text", "namespace uuid did not parse as uuid");
                }

                cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();

                if (!std_contains(cluster_metadata.rdb_namespaces.namespaces, namespace_uuid)) {
                    return http_res_t(404);
                }

                ++it;

                if (it == req.resource.end()) {
                    return http_res_t(400, "application/text", "Key not specified");
                }

                store_key_t key(*it);

                boost::shared_ptr<scoped_cJSON_t> doc(new scoped_cJSON_t(cJSON_Parse(req.body.c_str())));

                if (!doc->get()) {
                    return http_res_t(400, "application/text", "Json failed to parse");
                }

                cond_t interrupt;
                namespace_repo_t<rdb_protocol_t>::access_t ns_access(ns_repo, namespace_uuid, &interrupt);

                write.write = rdb_protocol_t::point_write_t(key, doc);

                cond_t cond;
                rdb_protocol_t::write_response_t write_res = ns_access.get_namespace_if()->write(write, order_source.check_in("rdb parser"), &cond);

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
            return http_res_t(400);
        }
        crash("Unreachable\n");
    } catch(cannot_perform_query_exc_t e) {
        http_res_t res;
        res.set_body("text/plain", e.what());
        return http_res_t(500);
    }
}

} //namespace mock
