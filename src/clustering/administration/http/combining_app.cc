#include "clustering/administration/http/combining_app.hpp"

#include "http/json.hpp"
#include "logger.hpp"

combining_http_app_t::combining_http_app_t(std::map<std::string, http_json_app_t *> components_)
    : components(components_) { }

http_res_t combining_http_app_t::handle(const http_req_t &req) {
    if (req.method != GET && req.method != POST) {
        return http_res_t(HTTP_METHOD_NOT_ALLOWED);
    }

    typedef std::map<std::string, http_json_app_t *>::const_iterator components_iterator;

    std::string resource = req.resource.as_string();
    if (resource != "/" && resource != "") return http_res_t(HTTP_NOT_FOUND);

    if (req.method == POST) {
        scoped_cJSON_t req_json(cJSON_Parse(req.body.c_str()));
        if (!req_json.get() || req_json.get()->type != cJSON_Object) {
            logINF("JSON body failed to parse as object: %s",
                   req.get_sanitized_body().c_str());
            return http_res_t(HTTP_BAD_REQUEST);
        }
        for (components_iterator it = components.begin(); it != components.end(); ++it) {
            cJSON *subpost = cJSON_GetObjectItem(req_json.get(), it->first.c_str());
            if (subpost) {
                http_req_t subreq = req;
                subreq.body = cJSON_PrintUnformatted(subpost);
                /* We discard the result of [handle] because it might change
                   when we handle a later request. */
                it->second->handle(subreq);
            }
        }
    }

    /* Throwing away the results of [handle] above and calling [get_root] after
       everything has been handled allows changes to one app that affect another
       app in the combiner to propagate. */
    scoped_cJSON_t json(cJSON_CreateObject());
    for (components_iterator it = components.begin(); it != components.end(); ++it) {
        scoped_cJSON_t child_json(NULL);
        it->second->get_root(&child_json);
        json.AddItemToObject(it->first.c_str(), child_json.release());
    }

    return http_json_res(json.get());
}
