// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "mock/dummy_protocol_parser.hpp"

#include "errors.hpp"

namespace mock {

query_http_app_t::query_http_app_t(namespace_interface_t<dummy_protocol_t> * _namespace_if)
    : namespace_if(_namespace_if)
{ }

void query_http_app_t::handle(const http_req_t &req, http_res_t *result, signal_t *) {
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

                if (read_res.values.find(*it) != read_res.values.end()) {
                    result->code = 200;
                    result->set_body("application/text", read_res.values[*it]);
                } else {
                    result->code = 404;
                }
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

                *result = http_res_t(HTTP_NO_CONTENT);
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
    } catch (const cannot_perform_query_exc_t &) {
        *result = http_res_t(HTTP_INTERNAL_SERVER_ERROR);
    }
}

} //namespace mock
