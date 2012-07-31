#include "errors.hpp"
#include <boost/variant.hpp>

#include "containers/uuid.hpp"
#include "clustering/administration/http/distribution_app.hpp"
#include "memcached/protocol_json_adapter.hpp"
#include "stl_utils.hpp"

#define DEFAULT_DEPTH 1
#define MAX_DEPTH 2

distribution_app_t::distribution_app_t(boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > > _namespaces_sl_metadata,
                                       namespace_repo_t<memcached_protocol_t> *_ns_repo)
    : namespaces_sl_metadata(_namespaces_sl_metadata),
      ns_repo(_ns_repo)
{ }

http_res_t distribution_app_t::handle(const http_req_t &req) {
    boost::optional<std::string> maybe_n_id = req.find_query_param("namespace");

    if (!maybe_n_id || !is_uuid(*maybe_n_id)) {
        return http_error_res("Valid uuid required for query parameter \"namespace\"\n");
    }
    namespace_id_t n_id = str_to_uuid(*maybe_n_id);

    namespaces_semilattice_metadata_t<memcached_protocol_t> ns_snapshot = namespaces_sl_metadata->get();

    if (!std_contains(ns_snapshot.namespaces, n_id)) {
        return http_res_t(HTTP_NOT_FOUND);
    }

    uint64_t depth = DEFAULT_DEPTH;
    boost::optional<std::string> maybe_depth = req.find_query_param("depth");

    if (maybe_depth) {
        if (!strtou64_strict(maybe_depth.get(), 10, &depth) || depth == 0 || depth > MAX_DEPTH) {
            return http_error_res("Invalid depth value.");
        }
    }

    try {
        cond_t interrupt;
        namespace_repo_t<memcached_protocol_t>::access_t ns_access(ns_repo, n_id, &interrupt);

        memcached_protocol_t::read_t read(distribution_get_query_t(depth), time(NULL));
        memcached_protocol_t::read_response_t db_res = ns_access.get_namespace_if()->read_outdated(
            read,
            &interrupt);

        scoped_cJSON_t data(render_as_json(&boost::get<distribution_result_t>(db_res.result).key_counts, 0));
        return http_json_res(data.get());
    } catch (cannot_perform_query_exc_t &) {
        return http_res_t(HTTP_INTERNAL_SERVER_ERROR);
    }
}
