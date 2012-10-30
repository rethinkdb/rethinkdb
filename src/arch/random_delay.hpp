// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RANDOM_DELAY_HPP_
#define ARCH_RANDOM_DELAY_HPP_

#include <stdlib.h>

#include "errors.hpp"

/* Functions to create random delays. Internally, they secretly use
the IO layer, but are safe to include from within the IO
layer. */


void random_delay(void (*)(void *), void *);  // NOLINT

template<class cb_t>
void random_delay(cb_t *cb, void (cb_t::*method)());

template<class cb_t, class arg1_t>
void random_delay(cb_t *cb, void (cb_t::*method)(arg1_t), arg1_t arg);

template<class cb_t>
bool maybe_random_delay(cb_t *cb, void (cb_t::*method)());

template<class cb_t, class arg1_t>
bool maybe_random_delay(cb_t *cb, void (cb_t::*method)(arg1_t), arg1_t arg);


/* Functions to create random delays */

template<class cb_t>
struct no_arg_caller_t
{
    cb_t *cb;
    void (cb_t::*method)();
    static void on_timer(void *ctx) {
        no_arg_caller_t *self = static_cast<no_arg_caller_t *>(ctx);
        ((self->cb)->*(self->method))();
        delete self;
    }
};

template<class cb_t>
void random_delay(cb_t *cb, void (cb_t::*method)()) {
    rassert(cb);

    no_arg_caller_t<cb_t> *c = new no_arg_caller_t<cb_t>;
    c->cb = cb;
    c->method = method;

    random_delay(&no_arg_caller_t<cb_t>::on_timer, c);
}

template<class cb_t, class arg1_t>
struct one_arg_caller_t
{
    cb_t *cb;
    void (cb_t::*method)(arg1_t);
    arg1_t arg;
    static void on_timer(void *ctx) {
        one_arg_caller_t *self = static_cast<one_arg_caller_t *>(ctx);
        ((self->cb)->*(self->method))(self->arg);
        delete self;
    }
};

template<class cb_t, class arg1_t>
void random_delay(cb_t *cb, void (cb_t::*method)(arg1_t), arg1_t arg) {
    rassert(cb);

    one_arg_caller_t<cb_t, arg1_t> *c = new one_arg_caller_t<cb_t, arg1_t>;
    c->cb = cb;
    c->method = method;
    c->arg = arg;

    random_delay(&one_arg_caller_t<cb_t, arg1_t>::on_timer, c);
}

template<class cb_t>
bool maybe_random_delay(cb_t *cb, void (cb_t::*method)()) {
    if (randint(2) == 0) {
        return true;
    } else {
        random_delay(cb, method);
        return false;
    }
}

template<class cb_t, class arg1_t>
bool maybe_random_delay(cb_t *cb, void (cb_t::*method)(arg1_t), arg1_t arg) {
    if (randint(2) == 0) {
        return true;
    } else {
        random_delay(cb, method, arg);
        return false;
    }
}




#endif  // ARCH_RANDOM_DELAY_HPP_
