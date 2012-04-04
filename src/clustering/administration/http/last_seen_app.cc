#include "clustering/administration/http/last_seen_app.hpp"

last_seen_http_app_t::last_seen_http_app_t(
        last_seen_tracker_t *lst) :
    last_seen_tracker(lst) { }

struct dummy_t {
};

http_res_t last_seen_http_app_t::handle(const http_req_t &req) {
    std::string resource = req.resource.as_string();
    if (resource != "/" && resource != "") {
        return http_res_t(404);
    }

    std::map<machine_id_t, time_t> last_seen_times = last_seen_tracker->get_last_seen_times();
    boost::shared_ptr<json_adapter_if_t<dummy_t> > json_adapter_head(new json_read_only_adapter_t<std::map<machine_id_t, time_t>, dummy_t>(&last_seen_times));
    scoped_cJSON_t json(json_adapter_head->render(dummy_t()));

    http_res_t res(200);
    res.set_body("application/json", cJSON_print_std_string(json.get()));
    return res;
}
