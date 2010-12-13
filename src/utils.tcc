
/* Callable objects that encapsulate an object's method. These are almost certainly duplicates
of something in Boost... */

template<class obj_t, class ret_t>
struct no_arg_method_caller_t {
    obj_t *obj;
    ret_t (obj_t::*m)();
    no_arg_method_caller_t(obj_t *o, ret_t (obj_t::*m)()) : obj(o), m(m) { }
    ret_t operator()() { return (obj->*m)(); }
};

template<class obj_t, class arg1_t, class ret_t>
struct one_arg_method_caller_t {
    obj_t *obj;
    ret_t (obj_t::*m)(arg1_t);
    arg1_t arg1;
    one_arg_method_caller_t(obj_t *o, ret_t (obj_t::*m)(arg1_t), arg1_t a1) : obj(o), m(m), arg1(a1) { }
    ret_t operator()() { return (obj->*m)(arg1); }
};

template<class obj_t, class arg1_t, class arg2_t, class ret_t>
struct two_arg_method_caller_t {
    obj_t *obj;
    ret_t (obj_t::*m)(arg1_t, arg2_t);
    arg1_t arg1;
    arg2_t arg2;
    two_arg_method_caller_t(obj_t *o, ret_t (obj_t::*m)(arg1_t, arg2_t), arg1_t a1, arg2_t a2)
        : obj(o), m(m), arg1(a1), arg2(a2) { }
    ret_t operator()() { return (obj->*m)(arg1, arg2); }
};

template<class obj_t, class arg1_t, class arg2_t, class arg3_t, class arg4_t, class ret_t>
struct four_arg_method_caller_t {
    obj_t *obj;
    ret_t (obj_t::*m)(arg1_t, arg2_t, arg3_t, arg4_t);
    arg1_t arg1;
    arg2_t arg2;
    arg3_t arg3;
    arg4_t arg4;
    four_arg_method_caller_t(obj_t *o, ret_t(obj_t::*m)(arg1_t, arg2_t, arg3_t, arg4_t), arg1_t a1, arg2_t a2, arg3_t a3, arg4_t a4)
        : obj(o), m(m), arg1(a1), arg2(a2), arg3(a3), arg4(a4) { }
    ret_t operator()() { return (obj->*m)(arg1, arg2, arg3, arg4); }
};

/* Functions to do something on another core in a way that is more convenient than
continue_on_cpu() is. */

template<class callable_t>
struct cpu_doer_t :
    public cpu_message_t,
    public home_cpu_mixin_t
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
        assert(cpu == get_cpu_id());
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
            default:
                unreachable("Bad state.");
        }
    }
};

template<class callable_t>
bool do_on_cpu(int cpu, const callable_t &callable) {
    cpu_doer_t<callable_t> *fsm = new cpu_doer_t<callable_t>(callable, cpu);
    return fsm->run();
}
template<class obj_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)()) {
    return do_on_cpu(cpu, no_arg_method_caller_t<obj_t, bool>(obj, on_other_core));
}
template<class obj_t, class arg1_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)(arg1_t), arg1_t arg1) {
    return do_on_cpu(cpu, one_arg_method_caller_t<obj_t, arg1_t, bool>(obj, on_other_core, arg1));
}
template<class obj_t, class arg1_t, class arg2_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)(arg1_t, arg2_t), arg1_t arg1, arg2_t arg2) {
    return do_on_cpu(cpu, two_arg_method_caller_t<obj_t, arg1_t, arg2_t, bool>(obj, on_other_core, arg1, arg2));
}
template<class obj_t, class arg1_t, class arg2_t, class arg3_t, class arg4_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)(arg1_t, arg2_t, arg3_t, arg4_t), arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4) {
    return do_on_cpu(cpu, four_arg_method_caller_t<obj_t, arg1_t, arg2_t, arg3_t, arg4_t, bool>(obj, on_other_core, arg1, arg2, arg3, arg4));
}

template<class callable_t>
struct later_doer_t :
    public cpu_message_t
{
    callable_t callable;
    
    later_doer_t(const callable_t &callable)
        : callable(callable) {
        call_later_on_this_cpu(this);
    }
    
    void on_cpu_switch() {
        callable();
        delete this;
    }
};


template<class callable_t>
void do_later(const callable_t &callable) {
    new later_doer_t<callable_t>(callable);
}

template<class obj_t>
void do_later(obj_t *obj, void (obj_t::*later)()) {
    return do_later(no_arg_method_caller_t<obj_t, void>(obj, later));
}

template<class obj_t, class arg1_t>
void do_later(obj_t *obj, void (obj_t::*later)(arg1_t), arg1_t arg1) {
    return do_later(one_arg_method_caller_t<obj_t, arg1_t, void>(obj, later, arg1));
}
