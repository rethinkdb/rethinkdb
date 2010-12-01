#include <string.h>
#include <string>
#include "arch/arch.hpp"
#include "request_handler/memcached_handler.hpp"
#include "request_handler/bin_memcached_handler.hpp"
#include "request_handler/txt_memcached_handler.hpp"
#include "conn_fsm.hpp"

memcached_handler_t::parse_result_t memcached_handler_t::parse_request(event_t *event)
{
    if (req_handler == NULL)
        determine_protocol(event);
    
    if (req_handler != NULL)
        return req_handler->parse_request(event);
    else
        return request_handler_t::op_partial_packet;
}

void memcached_handler_t::determine_protocol(const event_t *event)
{
    assert(!req_handler);
    req_handler = new txt_memcached_handler_t(server);
    req_handler->conn_fsm = conn_fsm;
    /*
    if (conn_fsm->nrbuf > sizeof(bin_memcached_handler_t::bin_magic_t)) {
        
        if (bin_memcached_handler_t::is_valid_magic(*(bin_memcached_handler_t::bin_magic_t*)conn_fsm->rbuf)) {
            //Binary protocol
            req_handler = new bin_memcached_handler_t(server);
        } else {
            //Textual protocol
            req_handler = new txt_memcached_handler_t(server);
        }
        
        req_handler->conn_fsm = conn_fsm;
    }*/
}
