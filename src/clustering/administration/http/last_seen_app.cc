#include "clustering/administration/http/last_seen_app.hpp"

last_seen_http_app_t::last_seen_http_app_t(
        last_seen_tracker_t *lst) :
    last_seen_tracker(lst) { }

struct dummy_t {
};


void last_seen_http_app_t::get_root(scoped_cJSON_t *json_out) {
    std::map<machine_id_t, time_t> last_seen_times = last_seen_tracker->get_last_seen_times();
    json_read_only_adapter_t<std::map<machine_id_t, time_t>, dummy_t> json_adapter(&last_seen_times);
    json_out->reset(json_adapter.render(dummy_t()));
}

http_res_t last_seen_http_app_t::handle(const http_req_t &req) {
    std::string resource = req.resource.as_string();
    if (resource != "/" && resource != "") {
        return http_res_t(HTTP_NOT_FOUND);
    }

    scoped_cJSON_t json(NULL);
    get_root(&json);

    return http_json_res(json.get());
}
