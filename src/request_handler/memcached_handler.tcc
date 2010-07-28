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
    check("Unset req_handler in memcached_handler_t::parse_request", req_handler == NULL);

    return req_handler->parse_request(event);
}

template<class config_t>
void memcached_handler_t<config_t>::build_response(request_t *request)
{
    check("Unset req_handler in memcached_handler_t::build_response", req_handler == NULL);
    req_handler->build_response(request);
}

template<class config_t>
void memcached_handler_t<config_t>::determine_protocol(const event_t *event)
{
    const conn_fsm_t *fsm = (conn_fsm_t*)event->state;
    if (fsm->nrbuf > sizeof(bin_magic_t)) {
        //we need to instantiate to do a check
        bin_memcached_handler_t bin_handler;
        
        if (bin_handler.is_valid_magic(*(bin_magic_t*) fsm->rbuf))
            req_handler = new bin_memcached_handler_t(cache, req_handler_t::event_queue);
        else
            req_handler = new txt_memcached_handler_t(cache, req_handler_t::event_queue);
    }
}

template<class config_t> const char* memcached_handler_t<config_t>::name = "memcached_handler_t";
#endif // __MEMCACHED_HANDLER_TCC__
