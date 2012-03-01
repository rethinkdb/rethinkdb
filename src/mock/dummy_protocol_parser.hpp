#ifndef __MOCK_DUMMY_PROTOCOL_PARSER_HPP__
#define __MOCK_DUMMY_PROTOCOL_PARSER_HPP__

#include "http/http.hpp"
#include "mock/dummy_protocol.hpp"
#include "protocol_api.hpp"

namespace mock {

class http_access_t : public http_server_t {
public:
    http_access_t(namespace_interface_t<dummy_protocol_t> *, 
                  int);
    http_res_t handle(const http_req_t &);

private:
    namespace_interface_t<dummy_protocol_t> *namespace_if; 
    order_source_t order_source;
};

}//namespace mock 

#endif
