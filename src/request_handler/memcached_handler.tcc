#ifndef __MEMCACHED_HANDLER_TCC__
#define __MEMCACHED_HANDLER_TCC__

#include <string.h>
#include <string>
#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "request_handler/memcached_handler.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"

template<class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::parse_request(event_t *event)
{
    if (req_handler == NULL)
        determine_protocol(event);
    
    if (req_handler != NULL)
        return req_handler->parse_request(event);
    else
        return req_handler_t::op_partial_packet;
}

template<class config_t>
void memcached_handler_t<config_t>::determine_protocol(const event_t *event)
{
    assert(!req_handler);
    
    if (conn_fsm->nrbuf > sizeof(bin_magic_t)) {
        
        if (bin_memcached_handler_t::is_valid_magic(*(bin_magic_t*)conn_fsm->rbuf)) {
            //Binary protocol
            req_handler = new bin_memcached_handler_t(event_queue, conn_fsm);
        } else {
            //Textual protocol
            req_handler = new txt_memcached_handler_t(event_queue, conn_fsm);
        } 
    }
}

#endif // __MEMCACHED_HANDLER_TCC__
