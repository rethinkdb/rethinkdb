#ifndef __MEMCACHED_HANDLER_TCC__
#define __MEMCACHED_HANDLER_TCC__

#include <string.h>
#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "request_handler/memcached_handler.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"

template<class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::parse_request(event_t *event)
{
    if (memcached_protocol == memcached_unknown_protocol)
        determine_protocol(event);

    check("Unset memcached_protocol in memcached_handler_t::parse_request", memcached_protocol == memcached_unknown_protocol && req_handler == NULL);

    parse_result_t res;

    switch (memcached_protocol) {
        case memcached_bin_protocol:
            res = ((bin_memcached_handler_t *) req_handler)->parse_request(event);
            break;
        case memcached_txt_protocol:
            res = ((txt_memcached_handler_t *) req_handler)->parse_request(event);
            break;
        default:
            res = (parse_result_t) -1; //some bullshit to make it compile
            check("Unset memcached_protocol in memcached_handler_t::parse_request", 0);
            break;
    }

    return res;//stfu GCC
}

template<class config_t>
void memcached_handler_t<config_t>::build_response(request_t *request)
{
    check("Unset memcached_protocol in memcached_handler_t::build_response", memcached_protocol == memcached_unknown_protocol && req_handler == NULL);

    switch (memcached_protocol) {
        case memcached_bin_protocol:
            ((bin_memcached_handler_t *) req_handler)->build_response(request);
            break;
        case memcached_txt_protocol:
            ((txt_memcached_handler_t *) req_handler)->build_response(request);
            break;
        default:
            check("Unset memcached_protocol in memcached_handler_t::build_response", 0);
    }
}

template<class config_t>
void memcached_handler_t<config_t>::determine_protocol(const event_t *event)
{
    const conn_fsm_t *fsm = (conn_fsm_t*)event->state;
    if (fsm->nrbuf > sizeof(bin_magic_t)) {
        bin_memcached_handler_t bin_handler;
        
        if (bin_handler.is_valid_magic(*(bin_magic_t*) fsm->rbuf)) {
            //Binary protocol
            memcached_protocol = memcached_bin_protocol;
            req_handler = new bin_memcached_handler_t(cache, req_handler_t::event_queue);
        } else {
            //Textual protocol
            memcached_protocol = memcached_txt_protocol;
            req_handler = new txt_memcached_handler_t(cache, req_handler_t::event_queue);
        } 
    } else {
        memcached_protocol = memcached_unknown_protocol;
    }
}
#endif // __MEMCACHED_HANDLER_TCC__
