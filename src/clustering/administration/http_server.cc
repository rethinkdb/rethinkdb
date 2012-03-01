
#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "clustering/administration/http_server.hpp"
#include "clustering/administration/json_adapters.hpp"

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
typedef tokenizer::iterator tok_iterator;

http_res_t blueprint_http_server_t::handle(const http_req_t &req) {
    cluster_semilattice_metadata_t cluster_metadata = 
        semilattice_metadata->get();

    //as we traverse the json sub directories this will keep track of where we are
    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > json_adapter_head(new json_adapter_t<cluster_semilattice_metadata_t, namespace_metadata_ctx_t>(&cluster_metadata));


    //setup a tokenizer
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator it = tokens.begin();

    //Traverse through the subfields until we're done with the url
    namespace_metadata_ctx_t json_ctx(us);
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
            res.set_body("application/json", cJSON_print_std_string(json_adapter_head->render(json_ctx)));
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
                logINF("json body failed to parse.\n");
                return http_res_t(400);
            }

            try {
                json_adapter_head->apply(change.get(), json_ctx);
                semilattice_metadata->join(cluster_metadata);

                http_res_t res(200);

                res.set_body("application/json", cJSON_print_std_string(json_adapter_head->render(json_ctx)));

                return res;
            } catch (schema_mismatch_exc_t e) {
                http_res_t res(400);
                res.set_body("application/text", e.what());
                return res;
            } catch (permission_denied_exc_t e) {
                http_res_t res(400);
                res.set_body("application/text", e.what());
                return res;
            }
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
    unreachable();
}

