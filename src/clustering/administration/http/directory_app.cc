#include "errors.hpp"
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/scoped_ptr.hpp>
#include <sstream>
#include "http/http.hpp"
#include "clustering/administration/http/directory_app.hpp"

directory_http_app_t::directory_http_app_t(clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> >& _directory_metadata)
    : directory_metadata(_directory_metadata) { }

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
typedef tokenizer::iterator tok_iterator;

static const char *any_machine_id_wildcard = "_";

cJSON *directory_http_app_t::get_metadata_json(cluster_directory_metadata_t& metadata, const std::vector<std::string>& path) THROWS_ONLY(schema_mismatch_exc_t) {
    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > json_adapter_head(new json_read_only_adapter_t<cluster_directory_metadata_t, namespace_metadata_ctx_t>(&metadata));
    namespace_metadata_ctx_t json_ctx(metadata.machine_id);

    const std::string separator("/");
    std::string path_so_far;

    //Traverse through the subfields until we're done with the url
    for (std::vector<std::string>::const_iterator it = path.begin(); it != path.end(); ++it) {
        path_so_far += separator + *it;

        json_adapter_if_t<namespace_metadata_ctx_t>::json_adapter_map_t subfields = json_adapter_head->get_subfields(json_ctx);
        if (subfields.find(*it) == subfields.end()) {
            std::stringstream error;
            error << "Unknown component '" << *it << "' in path '" << path_so_far << "'";
            throw schema_mismatch_exc_t(error.str());
        }
        json_adapter_head = subfields[*it];
    }

    return json_adapter_head->render(json_ctx);
}

http_res_t directory_http_app_t::handle(UNUSED const http_req_t &req) {
    try {
        connectivity_service_t *connectivity_service = directory_metadata->get_directory_service()->get_connectivity_service();
        connectivity_service_t::peers_list_freeze_t freeze_peers(connectivity_service);
        std::set<peer_id_t> peers_list = connectivity_service->get_peers_list();

        //setup a tokenizer
        boost::char_separator<char> sep("/");
        tokenizer tokens(req.resource, sep);
        tok_iterator it = tokens.begin();

        boost::optional<std::string> requested_machine_id;
        if (it != tokens.end()) {
            std::string machine_id_token = boost::lexical_cast<std::string>(*it);
            if (any_machine_id_wildcard != machine_id_token) {
                requested_machine_id = machine_id_token;
            }
            ++it;
        }
        std::vector<std::string> path(it, tokens.end());

        if (!requested_machine_id) {
            scoped_cJSON_t body(cJSON_CreateObject());
            for (std::set<peer_id_t>::const_iterator i = peers_list.begin(); i != peers_list.end(); ++i) {
                cluster_directory_metadata_t metadata = directory_metadata->get_value(*i).get();
                std::string machine_id = boost::lexical_cast<std::string>(metadata.machine_id);
                cJSON_AddItemToObject(body.get(), machine_id.c_str(), get_metadata_json(metadata, path));
            }
            http_res_t res(200);
            res.set_body("application/json", cJSON_print_std_string(body.get()));
            return res;
        } else {
            for (std::set<peer_id_t>::const_iterator i = peers_list.begin(); i != peers_list.end(); ++i) {
                cluster_directory_metadata_t metadata = directory_metadata->get_value(*i).get();
                std::string machine_id = boost::lexical_cast<std::string>(metadata.machine_id);
                if (*requested_machine_id == machine_id) {
                    scoped_cJSON_t machine_json = scoped_cJSON_t(get_metadata_json(metadata, path));
                    http_res_t res(200);
                    res.set_body("application/json", cJSON_print_std_string(machine_json.get()));
                    return res;
                }
            }
            http_res_t res(404);
            res.set_body("application/text", "Machine not found");
            return res;
        }
        unreachable();
    } catch (schema_mismatch_exc_t &e) {
        http_res_t res(404);
        logINF("HTTP request threw a schema_mismatch_exc_t with what =:\n %s\n", e.what());
        res.set_body("application/text", e.what());
        return res;
    } catch (permission_denied_exc_t &e) {
        http_res_t res(403); // TODO: should that be 405 Method Not Allowed?
        logINF("HTTP request threw a permission_denied_exc_t with what =:\n %s\n", e.what());
        res.set_body("application/text", e.what());
        return res;
    }
}

