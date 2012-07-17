#include "clustering/administration/http/combining_app.hpp"

#include "http/json.hpp"

combining_http_app_t::combining_http_app_t(std::map<std::string, http_json_app_t *> components_)
    : components(components_) { }

http_res_t combining_http_app_t::handle(const http_req_t &req) {
    std::string resource = req.resource.as_string();
    if (resource != "/" && resource != "") {
        return http_res_t(404);
    }

    scoped_cJSON_t json(cJSON_CreateObject());
    for (std::map<std::string, http_json_app_t *>::const_iterator it =  components.begin();
                                                                  it != components.end();
                                                                  it++) {
        scoped_cJSON_t child_json(NULL);
        it->second->get_root(&child_json);
        json.AddItemToObject(it->first.c_str(), child_json.release());
    }

    http_res_t res(200);
    res.set_body("application/json", json.Print());
    return res;
}
