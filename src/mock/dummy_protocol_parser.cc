#include "mock/dummy_protocol_parser.hpp"

#include "errors.hpp"

namespace mock {

query_http_app_t::query_http_app_t(namespace_interface_t<dummy_protocol_t> * _namespace_if)
    : namespace_if(_namespace_if)
{ }

http_res_t query_http_app_t::handle(const http_req_t &req) {
    try {
        switch (req.method) {
            case GET:
            {
                dummy_protocol_t::read_t read;

                http_req_t::resource_t::iterator it = req.resource.begin();
                rassert(it != req.resource.end());

                read.keys.keys.insert(*it);
                cond_t cond;
                dummy_protocol_t::read_response_t read_res;
                namespace_if->read(read, &read_res, order_source.check_in("dummy parser"), &cond);

                http_res_t res;
                if (read_res.values.find(*it) != read_res.values.end()) {
                    res.code = 200;
                    res.set_body("application/text", read_res.values[*it]);
                } else {
                    res.code = 404;
                }
                return res;
            }
            break;
            case PUT:
            case POST:
            {
                dummy_protocol_t::write_t write;

                http_req_t::resource_t::iterator it = req.resource.begin();
                rassert(it != req.resource.end());

                write.values.insert(std::make_pair(*it, req.body));
                cond_t cond;
                dummy_protocol_t::write_response_t write_res;
                namespace_if->write(write, &write_res, order_source.check_in("dummy parser"), &cond);

                return http_res_t(HTTP_NO_CONTENT);
            }
            break;
            case HEAD:
            case DELETE:
            case TRACE:
            case OPTIONS:
            case CONNECT:
            case PATCH:
            default:
                crash("Not implemented\n");
                break;
        }
        crash("Unreachable\n");
    } catch(cannot_perform_query_exc_t) {
        return http_res_t(HTTP_INTERNAL_SERVER_ERROR);
    }
}

} //namespace mock
