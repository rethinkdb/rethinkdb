#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "http/http.hpp"
#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/administration/http/semilattice_app.hpp"
#include "clustering/administration/suggester.hpp"
#include "stl_utils.hpp"

semilattice_http_app_t::semilattice_http_app_t(
        const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata,
        const clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> > &_directory_metadata,
        boost::uuids::uuid _us)
    : semilattice_metadata(_semilattice_metadata), directory_metadata(_directory_metadata), us(_us) { }

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
typedef tokenizer::iterator tok_iterator;

http_res_t semilattice_http_app_t::handle(const http_req_t &req) {
    try {
        cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();

        //as we traverse the json sub directories this will keep track of where we are
        boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > json_adapter_head(new json_adapter_t<cluster_semilattice_metadata_t, namespace_metadata_ctx_t>(&cluster_metadata));
        namespace_metadata_ctx_t json_ctx(us);

        //setup a tokenizer
        boost::char_separator<char> sep("/");
        tokenizer tokens(req.resource, sep);
        tok_iterator it = tokens.begin();

        if (it != tokens.end() && *it == "propose") {
            /* The user is dropping hints that she wants us to propose.  Bring that
             * bitch some blueprints, bitches love blueprints. */

            if (++it != tokens.end()) {
                /* Whoops dealbreaker */
                return http_res_t(404);
            }

            if (req.method == POST) {
#ifdef NDEBUG
                if (req.find_header_line("Content-Type") != "application/json") {
                    logINF("Bad request, Content-Type should be application/json.\n");
                    return http_res_t(415);
                }
#endif
                scoped_cJSON_t change(cJSON_Parse(req.body.c_str()));
                if (!change.get()) { //A null value indicates that parsing failed
                    logINF("Json body failed to parse.\n Here's the data that failed: %s\n", req.body.c_str());
                    return http_res_t(400);
                }

                json_adapter_head->apply(change.get(), json_ctx);
            }

            std::map<machine_id_t, datacenter_id_t> machine_assignments;

            for (std::map<machine_id_t, machine_semilattice_metadata_t>::iterator it  = cluster_metadata.machines.machines.begin();
                    it != cluster_metadata.machines.machines.end();
                    it++) {
                machine_assignments[it->first] = it->second.datacenter.get();
            }
            fill_in_blueprints_for_protocol<memcached_protocol_t>(&cluster_metadata.memcached_namespaces,
                    directory_metadata->subview(clone_ptr_t<read_lens_t<namespaces_directory_metadata_t<memcached_protocol_t>, cluster_directory_metadata_t> >(field_lens(&cluster_directory_metadata_t::memcached_namespaces))),
                    directory_metadata->subview(clone_ptr_t<read_lens_t<machine_id_t, cluster_directory_metadata_t> >(field_lens(&cluster_directory_metadata_t::machine_id))),
                    machine_assignments,
                    us);

            http_res_t res(200);
            scoped_cJSON_t json_repr(json_adapter_head->render(json_ctx));

            std::set<std::string> returned_fields;
            returned_fields.insert("dummy_namespaces");
            returned_fields.insert("memcached_namespaces");

            project(json_repr.get(), returned_fields);
            res.set_body("application/json", cJSON_print_std_string(json_repr.get()));
            return res;
        }

        //Traverse through the subfields until we're done with the url
        while (it != tokens.end()) {
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
#ifdef NDEBUG
                    if (req.find_header_line("Content-Type") != "application/json") {
                        logINF("Bad request, Content-Type should be application/json.\n");
                        return http_res_t(415);
                    }
#endif
                    scoped_cJSON_t change(cJSON_Parse(req.body.c_str()));
                    if (!change.get()) { //A null value indicates that parsing failed
                        logINF("Json body failed to parse.\n Here's the data that failed: %s\n", req.body.c_str());
                        return http_res_t(400);
                    }

                    logINF("Applying data %s\n", req.body.c_str());
                    json_adapter_head->apply(change.get(), json_ctx);
                    semilattice_metadata->join(cluster_metadata);

                    http_res_t res(200);

                    scoped_cJSON_t json_repr(json_adapter_head->render(json_ctx));
                    res.set_body("application/json", cJSON_print_std_string(json_repr.get()));

                    return res;
                }
                break;

            case HEAD:
            case PUT:
            case DELETE:
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
        logINF("HTTP request throw a schema_mismatch_exc_t with what =:\n %s\n", e.what());
        res.set_body("application/text", e.what());
        return res;
    } catch (permission_denied_exc_t &e) {
        http_res_t res(400);
        logINF("HTTP request throw a permission_denied_exc_t with what =:\n %s\n", e.what());
        res.set_body("application/text", e.what());
        return res;
    }
    unreachable();
}

