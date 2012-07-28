#include "rdb_protocol/parser.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>

namespace rdb_protocol {

query_http_app_t::query_http_app_t(namespace_interface_t<rdb_protocol_t> * _namespace_if)
    : namespace_if(_namespace_if)
{ }

http_res_t query_http_app_t::handle(const http_req_t &req) {
    try {
        switch (req.method) {
        case GET:
            {
                rdb_protocol_t::read_t read;

                http_req_t::resource_t::iterator it = req.resource.begin();
                rassert(it != req.resource.end());

                store_key_t key(*it);
                read.read = rdb_protocol_t::point_read_t(key);
                cond_t cond;
                rdb_protocol_t::read_response_t read_res = namespace_if->read(read, order_source.check_in("dummy parser"), &cond);

                http_res_t res;

                rdb_protocol_t::point_read_response_t response = boost::get<rdb_protocol_t::point_read_response_t>(read_res.response);
                if (response.data) {
                    res.code = 200;
                    res.set_body("application/json", response.data.get()->Print());
                } else {
                    res.code = 404;
                }
                return res;
            }
            break;
        case PUT:
        case POST:
            {
                rdb_protocol_t::write_t write;

                rassert(req.resource.begin() == req.resource.end());

                cJSON *doc = cJSON_Parse(req.body.c_str());

                if (doc == NULL) {
                    return http_res_t(400);
                }

                cJSON *id = cJSON_GetObjectItem(doc, "id");

                if (id == NULL || id->type != cJSON_String) {
                    cJSON_Delete(doc);
                    return http_res_t(400);
                }

                store_key_t key(id->valuestring);
                write.write = rdb_protocol_t::point_write_t(key, boost::make_shared<scoped_cJSON_t>(doc));

                cond_t cond;
                rdb_protocol_t::write_response_t write_res = namespace_if->write(write, order_source.check_in("dummy parser"), &cond);

                return http_res_t(204);
            }
            break;

        case HEAD:
        case DELETE:
        case TRACE:
        case OPTIONS:
        case CONNECT:
        case PATCH:
        default:
            return http_res_t(400);
        }
        crash("Unreachable\n");
    } catch(cannot_perform_query_exc_t e) {
        http_res_t res;
        res.set_body("text/plain", e.what());
        return http_res_t(500);
    }
}

} //namespace mock
