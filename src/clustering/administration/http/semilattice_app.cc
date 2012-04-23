#include "errors.hpp"

#include <string>

#include "http/http.hpp"
#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/administration/http/semilattice_app.hpp"
#include "clustering/administration/suggester.hpp"
#include "rpc/directory/watchable_copier.hpp"
#include "stl_utils.hpp"

void semilattice_http_app_t::fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata) {
    std::map<machine_id_t, datacenter_id_t> machine_assignments;

    for (std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> >::iterator it  = cluster_metadata->machines.machines.begin();
            it != cluster_metadata->machines.machines.end();
            it++) {
        if (!it->second.is_deleted()) {
            machine_assignments[it->first] = it->second.get().datacenter.get();
        }
    }

    std::map<peer_id_t, namespaces_directory_metadata_t<memcached_protocol_t> > reactor_directory;
    std::map<peer_id_t, machine_id_t> machine_id_translation_table;
    std::map<peer_id_t, cluster_directory_metadata_t> directory = translate_into_watchable(directory_metadata)->get();
    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator it = directory.begin(); it != directory.end(); it++) {
        reactor_directory.insert(std::make_pair(it->first, it->second.memcached_namespaces));
        machine_id_translation_table.insert(std::make_pair(it->first, it->second.machine_id));
    }

    fill_in_blueprints_for_protocol<memcached_protocol_t>(&cluster_metadata->memcached_namespaces,
            reactor_directory,
            machine_id_translation_table,
            machine_assignments,
            us);
}

semilattice_http_app_t::semilattice_http_app_t(
        const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata,
        const clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> > &_directory_metadata,
        boost::uuids::uuid _us)
    : semilattice_metadata(_semilattice_metadata), directory_metadata(_directory_metadata), us(_us) { }

http_res_t semilattice_http_app_t::handle(const http_req_t &req) {
    try {
        cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();

        //as we traverse the json sub directories this will keep track of where we are
        boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > json_adapter_head(new json_adapter_t<cluster_semilattice_metadata_t, namespace_metadata_ctx_t>(&cluster_metadata));
        namespace_metadata_ctx_t json_ctx(us);

        http_req_t::resource_t::iterator it = req.resource.begin();

        //Traverse through the subfields until we're done with the url
        while (it != req.resource.end()) {
            json_adapter_if_t<namespace_metadata_ctx_t>::json_adapter_map_t subfields = json_adapter_head->get_subfields(json_ctx);
            if (subfields.find(*it) == subfields.end()) {
                return http_res_t(404); //someone tried to walk off the edge of the world
            }
            json_adapter_head = subfields[*it];
            it++;
        }

        //json_adapter_head now points to the correct part of the metadata time to build a response and be on our way
        switch (req.method) {
            case GET:
            {
                http_res_t res(200);
                scoped_cJSON_t json_repr(json_adapter_head->render(json_ctx));
                res.set_body("application/json", cJSON_print_std_string(json_repr.get()));
                return res;
            }
            break;
            case POST:
            {
                // TODO: Get rid of this release mode wrapper, make Michael unhappy.
#ifdef NDEBUG
                {
                    boost::optional<std::string> content_type = req.find_header_line("Content-Type");
                    if (!content_type || content_type.get() != "application/json") {
                        logINF("Bad request, Content-Type should be application/json.");
                        return http_res_t(415);
                    }
                }
#endif
                scoped_cJSON_t change(cJSON_Parse(req.body.c_str()));
                if (!change.get()) { //A null value indicates that parsing failed
                    logINF("Json body failed to parse. Here's the data that failed: %s", req.body.c_str());
                    return http_res_t(400);
                }

                logINF("Applying data %s", req.body.c_str());
                json_adapter_head->apply(change.get(), json_ctx);

                /* Fill in the blueprints */
                try {
                    fill_in_blueprints(&cluster_metadata);
                } catch (missing_machine_exc_t &e) { }

                semilattice_metadata->join(cluster_metadata);

                http_res_t res(200);

                scoped_cJSON_t json_repr(json_adapter_head->render(json_ctx));
                res.set_body("application/json", cJSON_print_std_string(json_repr.get()));

                return res;
            }
            break;
            case DELETE:
            {
                json_adapter_head->erase(json_ctx);

                try {
                    fill_in_blueprints(&cluster_metadata);
                } catch (missing_machine_exc_t &e) { }

                semilattice_metadata->join(cluster_metadata);

                http_res_t res(200);

                scoped_cJSON_t json_repr(json_adapter_head->render(json_ctx));
                res.set_body("application/json", cJSON_print_std_string(json_repr.get()));

                return res;
            }
            break;
            case PUT:
            {
                // TODO: Get rid of this release mode wrapper, make Michael unhappy.
#ifdef NDEBUG
                {
                    boost::optional<std::string> content_type = req.find_header_line("Content-Type");
                    if (!content_type || content_type.get() != "application/json") {
                        logINF("Bad request, Content-Type should be application/json.");
                        return http_res_t(415);
                    }
                }
#endif
                scoped_cJSON_t change(cJSON_Parse(req.body.c_str()));
                if (!change.get()) { //A null value indicates that parsing failed
                    logINF("Json body failed to parse.\n Here's the data that failed: %s\n", req.body.c_str());
                    return http_res_t(400);
                }

                logINF("Applying data %s", req.body.c_str());
                json_adapter_head->reset(json_ctx);
                json_adapter_head->apply(change.get(), json_ctx);

                /* Fill in the blueprints */
                try {
                    fill_in_blueprints(&cluster_metadata);
                } catch (missing_machine_exc_t &e) { }

                semilattice_metadata->join(cluster_metadata);

                http_res_t res(200);

                scoped_cJSON_t json_repr(json_adapter_head->render(json_ctx));
                res.set_body("application/json", cJSON_print_std_string(json_repr.get()));

                return res;
            }
            break;
            case HEAD:
            case TRACE:
            case OPTIONS:
            case CONNECT:
            case PATCH:
            default:
                return http_res_t(405);
                break;
        }
    } catch (schema_mismatch_exc_t &e) {
        http_res_t res(400);
        logINF("HTTP request throw a schema_mismatch_exc_t with what = %s", e.what());
        res.set_body("application/text", e.what());
        return res;
    } catch (permission_denied_exc_t &e) {
        http_res_t res(400);
        logINF("HTTP request throw a permission_denied_exc_t with what = %s", e.what());
        res.set_body("application/text", e.what());
        return res;
    } catch (cannot_satisfy_goals_exc_t &e) {
        http_res_t res(500);
        logINF("The server was given a set of goals for which it couldn't find a valid blueprint. %s", e.what());
        res.set_body("application/text", e.what());
        return res;
    }
    unreachable();
}

