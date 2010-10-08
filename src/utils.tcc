

/* Functions to do something on another core in a way that is more convenient than
continue_on_cpu() is. */

template<class callable_t>
struct cpu_doer_t :
    public cpu_message_t,
    public home_cpu_mixin_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, cpu_doer_t<callable_t> >
{
    callable_t callable;
    int cpu;
    enum state_t {
        state_go_to_core,
        state_go_home
    } state;
    
    cpu_doer_t(const callable_t &callable, int cpu)
        : callable(callable), cpu(cpu), state(state_go_to_core) { }
    
    bool run() {
        state = state_go_to_core;
        if (continue_on_cpu(cpu, this)) return do_perform_job();
        else return false;
    }
    
    bool do_perform_job() {
        bool done = callable();
        do_return_home();
        return done;
    }
    
    void do_return_home() {
        state = state_go_home;
        if (continue_on_cpu(home_cpu, this)) delete this;
    }
    
    void on_cpu_switch() {
        switch (state) {
            case state_go_to_core:
                do_perform_job();
                return;
            case state_go_home:
                delete this;
                return;
            default: fail("Bad state.");
        }
    }
};

template<class callable_t>
bool do_on_cpu(int cpu, const callable_t &callable) {
    cpu_doer_t<callable_t> *fsm = new cpu_doer_t<callable_t>(callable, cpu);
    return fsm->run();
}

template<class obj_t>
struct no_arg_method_caller_t {
    obj_t *obj;
    bool (obj_t::*m)();
    no_arg_method_caller_t(obj_t *o, bool (obj_t::*m)()) : obj(o), m(m) { }
    bool operator()() { return (obj->*m)(); }
};
template<class obj_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)()) {
    return do_on_cpu(cpu, no_arg_method_caller_t<obj_t>(obj, on_other_core));
}

template<class obj_t, class arg1_t>
struct one_arg_method_caller_t {
    obj_t *obj;
    bool (obj_t::*m)(arg1_t);
    arg1_t arg1;
    one_arg_method_caller_t(obj_t *o, bool (obj_t::*m)(arg1_t), arg1_t a1) : obj(o), m(m), arg1(a1) { }
    bool operator()() { return (obj->*m)(arg1); }
};
template<class obj_t, class arg1_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)(arg1_t), arg1_t arg1) {
    return do_on_cpu(cpu, one_arg_method_caller_t<obj_t, arg1_t>(obj, on_other_core, arg1));
}

template<class obj_t, class arg1_t, class arg2_t>
struct two_arg_method_caller_t {
    obj_t *obj;
    bool (obj_t::*m)(arg1_t, arg2_t);
    arg1_t arg1;
    arg2_t arg2;
    two_arg_method_caller_t(obj_t *o, bool (obj_t::*m)(arg1_t, arg2_t), arg1_t a1, arg2_t a2)
        : obj(o), m(m), arg1(a1), arg2(a2) { }
    bool operator()() { return (obj->*m)(arg1, arg2); }
};
template<class obj_t, class arg1_t, class arg2_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)(arg1_t, arg2_t), arg1_t arg1, arg2_t arg2) {
    return do_on_cpu(cpu, two_arg_method_caller_t<obj_t, arg1_t, arg2_t>(obj, on_other_core, arg1, arg2));
}

template<class obj_t>
struct deleter_t {
    obj_t *obj;
    deleter_t(obj_t *o) : obj(o) { }
    bool operator()() { delete obj; return true; }
};
template<class obj_t>
void delete_on_cpu(int cpu, obj_t *obj) {
    do_on_cpu(cpu, deleter_t<obj_t>(obj));
}

template<class obj_t>
struct gdeleter_t {
    obj_t *obj;
    gdeleter_t(obj_t *o) : obj(o) { }
    bool operator()() { gdelete(obj); return true; }
};
template<class obj_t>
void gdelete_on_cpu(int cpu, obj_t *obj) {
    do_on_cpu(cpu, gdeleter_t<obj_t>(obj));
}
