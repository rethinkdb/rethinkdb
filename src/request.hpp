#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__

#include "config/args.hpp"

class btree_fsm_t;
class request_handler_t;

/* This is a pretty horrible hack. Originally there was a type called request_t that
abstracted the idea of starting multiple btree_fsms and waiting for them all to complete.
When we got rid of request_t, we kept request_callback_t because it was playing a
role in the handling of large values. It should be renamed to something large-value-related
or gotten rid of. */

struct request_callback_t {

    request_callback_t(request_handler_t *rh) : rh(rh) {}

    virtual void on_fsm_ready(btree_fsm_t *fsm) {} // XXX Rename this

    request_handler_t *rh;
};

#endif // __REQUEST_HPP__
