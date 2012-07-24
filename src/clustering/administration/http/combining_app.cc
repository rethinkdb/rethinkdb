#include "clustering/administration/http/combining_app.hpp"

#include "http/json.hpp"
#include "logger.hpp"

combining_http_app_t::combining_http_app_t(std::map<std::string, http_json_app_t *> components_)
	: components(components_) { }

http_res_t combining_http_app_t::handle(const http_req_t &req) {
    typedef std::map<std::string, http_json_app_t *>::const_iterator components_iterator;
    std::string resource = req.resource.as_string();
    if (resource != "/" && resource != "") {
        return http_res_t(404);
    }

    scoped_cJSON_t json(cJSON_CreateObject());
    for (components_iterator it = components.begin(); it != components.end(); ++it) {
        scoped_cJSON_t child_json(NULL), req_json(NULL);
        cJSON *subpost;
        switch(req.method) {
        case GET:
            it->second->get_root(&child_json);
            break;

        case POST:
            req_json.reset(cJSON_Parse(req.body.c_str()));
            if (!req_json.get() || req_json.get()->type != cJSON_Object) {
                logINF("JSON body failed to parse as object: %s",
                       req.get_sanitized_body().c_str());
                return http_res_t(400);
            }

            subpost = cJSON_GetObjectItem(req_json.get(), it->first.c_str());
            if (subpost) {
                http_req_t subreq = req;
                subreq.body = cJSON_PrintUnformatted(subpost);
                child_json.reset(cJSON_Parse(it->second->handle(subreq).body.c_str()));
            } else {
                it->second->get_root(&child_json);
            }
            break;

        case PUT: case DELETE:
        case HEAD: case TRACE: case OPTIONS: case CONNECT: case PATCH:
        default: return http_res_t(405);
        }
        cJSON_AddItemToObject(json.get(), it->first.c_str(), child_json.release());
    }

    http_res_t res(200);
    res.set_body("application/json", cJSON_print_std_string(json.get()));
    return res;
}
