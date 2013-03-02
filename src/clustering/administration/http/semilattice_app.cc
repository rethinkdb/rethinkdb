// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <string>

#include "errors.hpp"
#include <boost/algorithm/string/predicate.hpp>

#include "http/http.hpp"
#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/administration/http/semilattice_app.hpp"
#include "clustering/administration/suggester.hpp"
#include "stl_utils.hpp"

semilattice_http_app_t::semilattice_http_app_t(
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        uuid_u _us)
    : metadata_change_handler(_metadata_change_handler), directory_metadata(_directory_metadata), us(_us) { }

void semilattice_http_app_t::get_root(scoped_cJSON_t *json_out) {
    // keep this in sync with handle's behavior for getting the root
    cluster_semilattice_metadata_t cluster_metadata = metadata_change_handler->get();
    vclock_ctx_t json_ctx(us);
    json_ctx_adapter_t<cluster_semilattice_metadata_t, vclock_ctx_t> json_adapter(&cluster_metadata, json_ctx);
    json_out->reset(json_adapter.render());
}

http_res_t semilattice_http_app_t::handle(const http_req_t &req) {
    try {
        cluster_semilattice_metadata_t cluster_metadata = metadata_change_handler->get();

        //as we traverse the json sub directories this will keep track of where we are
        vclock_ctx_t json_ctx(us);
        boost::shared_ptr<json_adapter_if_t> json_adapter_head(new json_ctx_adapter_t<cluster_semilattice_metadata_t, vclock_ctx_t>(&cluster_metadata, json_ctx));

        http_req_t::resource_t::iterator it = req.resource.begin();

        //Traverse through the subfields until we're done with the url
        while (it != req.resource.end()) {
            json_adapter_if_t::json_adapter_map_t subfields = json_adapter_head->get_subfields();
            if (subfields.find(*it) == subfields.end()) {
                return http_res_t(HTTP_NOT_FOUND); //someone tried to walk off the edge of the world
            }
            json_adapter_head = subfields[*it];
            it++;
        }

        //json_adapter_head now points to the correct part of the metadata time to build a response and be on our way
        switch (req.method) {
            case GET:
            {
                scoped_cJSON_t json_repr(json_adapter_head->render());
                return http_json_res(json_repr.get());
            }
            break;
            case POST:
            {
                // TODO: Get rid of this release mode wrapper, make Michael unhappy.
#ifdef NDEBUG
                if (!verify_content_type(req, "application/json")) {
                    return http_res_t(HTTP_UNSUPPORTED_MEDIA_TYPE);
                }
#endif
                scoped_cJSON_t change(cJSON_Parse(req.body.c_str()));
                if (!change.get()) { //A null value indicates that parsing failed
                    logINF("Json body failed to parse. Here's the data that failed: %s",
                           req.get_sanitized_body().c_str());
                    return http_res_t(HTTP_BAD_REQUEST);
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

                /* Fill in the blueprints */
                try {
                    fill_in_blueprints(&cluster_metadata, directory_metadata->get(), us, (req.find_query_param("prefer_distribution") ? true : false)); //RSI this needs to be a parameter that we get from the request
                } catch (const missing_machine_exc_t &e) { }

                metadata_change_handler->update(cluster_metadata);

                scoped_cJSON_t json_repr(json_adapter_head->render());
                return http_json_res(json_repr.get());
            }
            break;
            case DELETE:
            {
                json_adapter_head->erase();

                logINF("Deleting %s", req.resource.as_string().c_str());

                try {
                    fill_in_blueprints(&cluster_metadata, directory_metadata->get(), us, false);
                } catch (const missing_machine_exc_t &e) { }

                metadata_change_handler->update(cluster_metadata);

                scoped_cJSON_t json_repr(json_adapter_head->render());
                return http_json_res(json_repr.get());
            }
            break;
            case PUT:
            {
                // TODO: Get rid of this release mode wrapper, make Michael unhappy.
#ifdef NDEBUG
                if (!verify_content_type(req, "application/json")) {
                    return http_res_t(HTTP_UNSUPPORTED_MEDIA_TYPE);
                }
#endif
                scoped_cJSON_t change(cJSON_Parse(req.body.c_str()));
                if (!change.get()) { //A null value indicates that parsing failed
                    logINF("Json body failed to parse. Here's the data that failed: %s",
                           req.get_sanitized_body().c_str());
                    return http_res_t(HTTP_BAD_REQUEST);
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

                /* Fill in the blueprints */
                try {
                    fill_in_blueprints(&cluster_metadata, directory_metadata->get(), us, false);
                } catch (const missing_machine_exc_t &e) { }

                metadata_change_handler->update(cluster_metadata);

                scoped_cJSON_t json_repr(json_adapter_head->render());
                return http_json_res(json_repr.get());
            }
            break;
            case HEAD:
            case TRACE:
            case OPTIONS:
            case CONNECT:
            case PATCH:
            default:
                return http_res_t(HTTP_METHOD_NOT_ALLOWED);
                break;
        }
    } catch (const schema_mismatch_exc_t &e) {
        logINF("HTTP request throw a schema_mismatch_exc_t with what = %s", e.what());
        return http_error_res(e.what());
    } catch (const permission_denied_exc_t &e) {
        logINF("HTTP request throw a permission_denied_exc_t with what = %s", e.what());
        return http_error_res(e.what());
    } catch (const cannot_satisfy_goals_exc_t &e) {
        logINF("The server was given a set of goals for which it couldn't find a valid blueprint. %s", e.what());
        return http_error_res(e.what(), HTTP_INTERNAL_SERVER_ERROR);
    } catch (const gone_exc_t & e) {
        logINF("HTTP request throw a gone_exc_t with what = %s", e.what());
        return http_error_res(e.what(), HTTP_GONE);
    }
    unreachable();
}

bool semilattice_http_app_t::verify_content_type(const http_req_t &req, const std::string &expected_content_type) const {
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

