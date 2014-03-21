// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/algorithm/string/predicate.hpp>

#include "http/http.hpp"
#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/administration/http/semilattice_app.hpp"
#include "clustering/administration/suggester.hpp"
#include "stl_utils.hpp"

template <class metadata_t>
semilattice_http_app_t<metadata_t>::semilattice_http_app_t(
        metadata_change_handler_t<metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        uuid_u _us) :
    directory_metadata(_directory_metadata),
    us(_us),
    metadata_change_handler(_metadata_change_handler) {
    // Do nothing
}

template <class metadata_t>
semilattice_http_app_t<metadata_t>::~semilattice_http_app_t() {
    // Do nothing
}

template <class metadata_t>
void semilattice_http_app_t<metadata_t>::get_root(scoped_cJSON_t *json_out) {
    // keep this in sync with handle's behavior for getting the root
    metadata_t metadata = metadata_change_handler->get();
    vclock_ctx_t json_ctx(us);
    json_ctx_adapter_t<metadata_t, vclock_ctx_t> json_adapter(&metadata, json_ctx);
    json_out->reset(json_adapter.render());
}

template <class metadata_t>
void semilattice_http_app_t<metadata_t>::handle(const http_req_t &req, http_res_t *result, signal_t *) {
    try {
        metadata_t metadata = metadata_change_handler->get();

        //as we traverse the json sub directories this will keep track of where we are
        vclock_ctx_t json_ctx(us);
        boost::shared_ptr<json_adapter_if_t> json_adapter_head(new json_ctx_adapter_t<metadata_t, vclock_ctx_t>(&metadata, json_ctx));

        http_req_t::resource_t::iterator it = req.resource.begin();

        //Traverse through the subfields until we're done with the url
        while (it != req.resource.end()) {
            json_adapter_if_t::json_adapter_map_t subfields = json_adapter_head->get_subfields();
            if (subfields.find(*it) == subfields.end()) {
                *result = http_res_t(HTTP_NOT_FOUND); //someone tried to walk off the edge of the world
                return;
            }
            json_adapter_head = subfields[*it];
            it++;
        }

        //json_adapter_head now points to the correct part of the metadata time to build a response and be on our way
        switch (req.method) {
            case GET:
            {
                scoped_cJSON_t json_repr(json_adapter_head->render());
                http_json_res(json_repr.get(), result);
            }
            break;
            case POST:
            {
                // TODO: Get rid of this release mode wrapper, make Michael unhappy.
#ifdef NDEBUG
                if (!verify_content_type(req, "application/json")) {
                    *result = http_res_t(HTTP_UNSUPPORTED_MEDIA_TYPE);
                    return;
                }
#endif
                scoped_cJSON_t change(cJSON_Parse(req.body.c_str()));
                if (!change.get()) { //A null value indicates that parsing failed
                    logINF("Json body failed to parse. Here's the data that failed: %s",
                           req.get_sanitized_body().c_str());
                    *result = http_res_t(HTTP_BAD_REQUEST);
                    return;
                }

                // Determine for which namespaces we should prioritize distribution
                // Default: none
                boost::optional<namespace_id_t> prioritize_distr_for_ns;
                const boost::optional<std::string> prefer_distribution_param =
                    req.find_query_param("prefer_distribution_for");
                if (prefer_distribution_param) {
                    try {
                        const namespace_id_t ns_id =
                            str_to_uuid(prefer_distribution_param.get());
                        prioritize_distr_for_ns.reset(ns_id);
                    } catch (...) {
                        logINF("Invalid value for prefer_distribution_for argument: %s",
                           prefer_distribution_param.get().c_str());
                        *result = http_res_t(HTTP_BAD_REQUEST);
                        return;
                    }
                }

                json_adapter_head->apply(change.get());

                {
                    scoped_cJSON_t absolute_change(change.release());
                    std::vector<std::string> parts(req.resource.begin(), req.resource.end());
                    for (std::vector<std::string>::reverse_iterator jt = parts.rbegin(); jt != parts.rend(); ++jt) {
                        scoped_cJSON_t inner(absolute_change.release());
                        absolute_change.reset(cJSON_CreateObject());
                        absolute_change.AddItemToObject(jt->c_str(), inner.release());
                    }
                    logINF("Applying data %s", absolute_change.PrintUnformatted().c_str());
                }

                metadata_change_callback(&metadata, prioritize_distr_for_ns);
                metadata_change_handler->update(metadata);

                scoped_cJSON_t json_repr(json_adapter_head->render());
                http_json_res(json_repr.get(), result);
            }
            break;
            case DELETE:
            {
                json_adapter_head->erase();

                logINF("Deleting %s", req.resource.as_string().c_str());

                metadata_change_callback(&metadata,
                                         boost::optional<namespace_id_t>());
                metadata_change_handler->update(metadata);

                scoped_cJSON_t json_repr(json_adapter_head->render());
                http_json_res(json_repr.get(), result);
            }
            break;
            case PUT:
            {
                // TODO: Get rid of this release mode wrapper, make Michael unhappy.
#ifdef NDEBUG
                if (!verify_content_type(req, "application/json")) {
                    *result = http_res_t(HTTP_UNSUPPORTED_MEDIA_TYPE);
                    return;
                }
#endif
                scoped_cJSON_t change(cJSON_Parse(req.body.c_str()));
                if (!change.get()) { //A null value indicates that parsing failed
                    logINF("Json body failed to parse. Here's the data that failed: %s",
                           req.get_sanitized_body().c_str());
                    *result = http_res_t(HTTP_BAD_REQUEST);
                    return;
                }

                {
                    scoped_cJSON_t absolute_change(change.release());
                    std::vector<std::string> parts(req.resource.begin(), req.resource.end());
                    for (std::vector<std::string>::reverse_iterator jt = parts.rbegin(); jt != parts.rend(); ++jt) {
                        scoped_cJSON_t inner(absolute_change.release());
                        absolute_change.reset(cJSON_CreateObject());
                        absolute_change.AddItemToObject(jt->c_str(), inner.release());
                    }
                    logINF("Applying data %s", absolute_change.PrintUnformatted().c_str());
                }

                json_adapter_head->reset();
                json_adapter_head->apply(change.get());

                metadata_change_callback(&metadata,
                                         boost::optional<namespace_id_t>());
                metadata_change_handler->update(metadata);

                scoped_cJSON_t json_repr(json_adapter_head->render());
                http_json_res(json_repr.get(), result);
            }
            break;
            case HEAD:
            case TRACE:
            case OPTIONS:
            case CONNECT:
            case PATCH:
            default:
                *result = http_res_t(HTTP_METHOD_NOT_ALLOWED);
                break;
        }
    } catch (const schema_mismatch_exc_t &e) {
        logINF("HTTP request throw a schema_mismatch_exc_t with what = %s", e.what());
        *result =  http_error_res(e.what());
    } catch (const permission_denied_exc_t &e) {
        logINF("HTTP request throw a permission_denied_exc_t with what = %s", e.what());
        *result = http_error_res(e.what());
    } catch (const cannot_satisfy_goals_exc_t &e) {
        logINF("The server was given a set of goals for which it couldn't find a valid blueprint. %s", e.what());
        *result = http_error_res(e.what(), HTTP_INTERNAL_SERVER_ERROR);
    } catch (const gone_exc_t &e) {
        logINF("HTTP request throw a gone_exc_t with what = %s", e.what());
        *result = http_error_res(e.what(), HTTP_GONE);
    } catch (const multiple_choices_exc_t &e) {
        logINF("HTTP request failed to change semilattice data. %s", e.what());
        *result = http_error_res(e.what(), HTTP_METHOD_NOT_ALLOWED);
    }
}

template <class metadata_t>
bool semilattice_http_app_t<metadata_t>::verify_content_type(const http_req_t &req,
                                                             const std::string &expected_content_type) const {
    boost::optional<std::string> content_type = req.find_header_line("Content-Type");
    // Only compare the beginning of the content-type. Some browsers may add additional
    // information, and e.g. send "application/json; charset=UTF-8" instead of "application/json"
    if (!content_type || !boost::istarts_with(content_type.get(), expected_content_type)) {
        std::string actual_content_type = (content_type ? content_type.get() : "<NONE>");
        logINF("Bad request, Content-Type should be %s, but is %s.", expected_content_type.c_str(), actual_content_type.c_str());
        return false;
    }

    return true;
}

cluster_semilattice_http_app_t::cluster_semilattice_http_app_t(
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        uuid_u _us) :
    semilattice_http_app_t<cluster_semilattice_metadata_t>(_metadata_change_handler, _directory_metadata, _us) {
    // Do nothing
}

cluster_semilattice_http_app_t::~cluster_semilattice_http_app_t() {
    // Do nothing
}

void cluster_semilattice_http_app_t::metadata_change_callback(cluster_semilattice_metadata_t *new_metadata,
        const boost::optional<namespace_id_t> &prioritize_distr_for_ns) {

    try {
        fill_in_blueprints(new_metadata,
                           directory_metadata->get().get_inner(),
                           us,
                           prioritize_distr_for_ns);
    } catch (const missing_machine_exc_t &e) { }
}

auth_semilattice_http_app_t::auth_semilattice_http_app_t(
        metadata_change_handler_t<auth_semilattice_metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        uuid_u _us) :
    semilattice_http_app_t<auth_semilattice_metadata_t>(_metadata_change_handler, _directory_metadata, _us) {
    // Do nothing
}

auth_semilattice_http_app_t::~auth_semilattice_http_app_t() {
    // Do nothing
}

void auth_semilattice_http_app_t::metadata_change_callback(auth_semilattice_metadata_t *,
        const boost::optional<namespace_id_t> &) {

    // Do nothing
}

template class semilattice_http_app_t<cluster_semilattice_metadata_t>;
template class semilattice_http_app_t<auth_semilattice_metadata_t>;
