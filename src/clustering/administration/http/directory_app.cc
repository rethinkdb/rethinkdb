// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "containers/uuid.hpp"
#include "http/http.hpp"
#include "clustering/administration/http/directory_app.hpp"

directory_http_app_t::directory_http_app_t(const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >& _directory_metadata)
    : directory_metadata(_directory_metadata) { }

static const char *any_machine_id_wildcard = "_";

cJSON *directory_http_app_t::get_metadata_json(
    cluster_directory_metadata_t *metadata,
    http_req_t::resource_t::iterator path_begin,
    http_req_t::resource_t::iterator path_end) THROWS_ONLY(schema_mismatch_exc_t) {
    boost::shared_ptr<json_adapter_if_t> json_adapter_head(
        new json_read_only_adapter_t<cluster_directory_metadata_t>(metadata));

    // Traverse through the subfields until we're done with the url
    for (http_req_t::resource_t::iterator it = path_begin; it != path_end; ++it) {
        json_adapter_if_t::json_adapter_map_t subfields
            = json_adapter_head->get_subfields();
        if (subfields.find(*it) == subfields.end()) {
            // format an error message
            std::string error;
            error += "Unknown component '";
            error += *it;
            error += "' in path '";
            for (auto pre_it = path_begin; pre_it != it; ++pre_it) {
                error += *pre_it;
                error += '/';
            }
            error += *it;
            error += "'";
            throw schema_mismatch_exc_t(error);
        }
        json_adapter_head = subfields[*it];
    }

    return json_adapter_head->render();
}

void directory_http_app_t::get_root(scoped_cJSON_t *json_out) {
    // keep this in sync with handle's behavior for getting the root
    std::map<peer_id_t, cluster_directory_metadata_t> md = directory_metadata->get();

    json_out->reset(cJSON_CreateObject());

    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator i = md.begin(); i != md.end(); ++i) {
        cluster_directory_metadata_t metadata = i->second;
        std::string machine_id = uuid_to_str(metadata.machine_id);

        json_read_only_adapter_t<cluster_directory_metadata_t> json_adapter(&metadata);

        json_out->AddItemToObject(machine_id.c_str(), json_adapter.render());
    }
}

http_res_t directory_http_app_t::handle(const http_req_t &req) {
    if (req.method != GET) {
        return http_res_t(HTTP_METHOD_NOT_ALLOWED);
    }
    try {
        std::map<peer_id_t, cluster_directory_metadata_t> md = directory_metadata->get();

        http_req_t::resource_t::iterator it = req.resource.begin();

        boost::optional<std::string> requested_machine_id;
        if (it != req.resource.end()) {
            std::string machine_id_token = *it;
            if (any_machine_id_wildcard != machine_id_token) {
                requested_machine_id = machine_id_token;
            }
            ++it;
        }

        if (!requested_machine_id) {
            scoped_cJSON_t body(cJSON_CreateObject());
            for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator i = md.begin(); i != md.end(); ++i) {
                cluster_directory_metadata_t metadata = i->second;
                std::string machine_id = uuid_to_str(metadata.machine_id);
                body.AddItemToObject(machine_id.c_str(), get_metadata_json(&metadata, it, req.resource.end()));
            }
            return http_json_res(body.get());
        } else {
            for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator i = md.begin(); i != md.end(); ++i) {
                cluster_directory_metadata_t metadata = i->second;
                std::string machine_id = uuid_to_str(metadata.machine_id);
                if (*requested_machine_id == machine_id) {
                    scoped_cJSON_t machine_json(get_metadata_json(&metadata, it, req.resource.end()));
                    return http_json_res(machine_json.get());
                }
            }
            return http_error_res("Machine not found", HTTP_NOT_FOUND);
        }
        unreachable();
    } catch (const schema_mismatch_exc_t &e) {
        logINF("HTTP request threw a schema_mismatch_exc_t with what = %s", e.what());
        return http_error_res(e.what(), HTTP_NOT_FOUND);
    } catch (const permission_denied_exc_t &e) {
        logINF("HTTP request threw a permission_denied_exc_t with what = %s", e.what());
        // TODO: should that be 405 Method Not Allowed?
        return http_error_res(e.what(), HTTP_FORBIDDEN);
    }
}

