

/* Functions to create random delays */

template<class cb_t>
struct no_arg_caller_t
{
    cb_t *cb;
    void (cb_t::*method)();
    static void on_timer(void *ctx) {
        no_arg_caller_t *self = (no_arg_caller_t*)ctx;
        ((self->cb)->*(self->method))();
        delete self;
    }
};

template<class cb_t>
void random_delay(cb_t *cb, void (cb_t::*method)()) {
    
    assert(cb);
    
    no_arg_caller_t<cb_t> *c = new no_arg_caller_t<cb_t>;
    c->cb = cb;
    c->method = method;
    
    random_delay(&no_arg_caller_t<cb_t>::on_timer, (void*)c);
}

template<class cb_t, class arg1_t>
struct one_arg_caller_t
{
    cb_t *cb;
    void (cb_t::*method)(arg1_t);
    arg1_t arg;
    static void on_timer(void *ctx) {
        one_arg_caller_t *self = (one_arg_caller_t*)ctx;
        ((self->cb)->*(self->method))(self->arg);
        delete self;
    }
};

template<class cb_t, class arg1_t>
void random_delay(cb_t *cb, void (cb_t::*method)(arg1_t), arg1_t arg) {
    
    assert(cb);
    
    one_arg_caller_t<cb_t, arg1_t> *c = new one_arg_caller_t<cb_t, arg1_t>;
    c->cb = cb;
    c->method = method;
    c->arg = arg;
    
    random_delay(&one_arg_caller_t<cb_t, arg1_t>::on_timer, (void*)c);
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
