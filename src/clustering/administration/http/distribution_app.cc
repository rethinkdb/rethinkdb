// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/administration/http/distribution_app.hpp"
#include "containers/uuid.hpp"
#include "region/region_json_adapter.hpp"
#include "rdb_protocol/protocol.hpp"
#include "stl_utils.hpp"

#define DEFAULT_DEPTH 1
#define MAX_DEPTH 2
#define DEFAULT_LIMIT 128

distribution_app_t::distribution_app_t(boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > _rdb_namespaces_sl_metadata,
                                       namespace_repo_t *_rdb_ns_repo)
    : rdb_namespaces_sl_metadata(_rdb_namespaces_sl_metadata),
      rdb_ns_repo(_rdb_ns_repo)
{ }

void distribution_app_t::handle(const http_req_t &req, http_res_t *result, signal_t *interruptor) {
    if (req.method != GET) {
        *result = http_res_t(HTTP_METHOD_NOT_ALLOWED);
        return;
    }

    boost::optional<std::string> maybe_n_id = req.find_query_param("namespace");

    if (!maybe_n_id || !is_uuid(*maybe_n_id)) {
        *result = http_error_res("Valid uuid required for query parameter \"namespace\"\n");
        return;
    }
    namespace_id_t n_id = str_to_uuid(*maybe_n_id);

    cow_ptr_t<namespaces_semilattice_metadata_t> rdb_ns_snapshot = rdb_namespaces_sl_metadata->get();

    uint64_t depth = DEFAULT_DEPTH;
    boost::optional<std::string> maybe_depth = req.find_query_param("depth");

    if (maybe_depth) {
        if (!strtou64_strict(maybe_depth.get(), 10, &depth) || depth == 0 || depth > MAX_DEPTH) {
            *result = http_error_res("Invalid depth value.");
            return;
        }
    }

    uint64_t limit = DEFAULT_LIMIT;
    boost::optional<std::string> maybe_limit = req.find_query_param("limit");

    if (maybe_limit) {
        if (!strtou64_strict(maybe_limit.get(), 10, &limit)) {
            *result = http_error_res("Invalid limit value.");
            return;
        }
    }

    if (std_contains(rdb_ns_snapshot->namespaces, n_id)) {
        try {
            namespace_repo_t::access_t rdb_ns_access(rdb_ns_repo, n_id, interruptor);

            distribution_read_t inner_read(depth, limit);
            read_t read(inner_read, profile_bool_t::DONT_PROFILE);
            read_response_t db_res;
            rdb_ns_access.get_namespace_if()->read_outdated(read,
                                                            &db_res,
                                                            interruptor);

            scoped_cJSON_t data(render_as_json(&boost::get<distribution_read_response_t>(db_res.response).key_counts));
            http_json_res(data.get(), result);
        } catch (const cannot_perform_query_exc_t &exc) {
            *result = http_res_t(HTTP_INTERNAL_SERVER_ERROR, "text/plain", exc.what());
        }
    } else {
        *result = http_res_t(HTTP_NOT_FOUND);
    }
}
