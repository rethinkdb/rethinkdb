

/* Functions to do something on another core in a way that is more convenient than
continue_on_cpu() is. */

template<class obj_t>
struct no_arg_cpu_doer_t :
    public cpu_message_t,
    public home_cpu_mixin_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, no_arg_cpu_doer_t<obj_t> >
{
    obj_t *obj;
    bool (obj_t::*on_other_core)();
    int cpu;
    enum state_t {
        state_go_to_core,
        state_go_home
    } state;
    
    bool run() {
        state = state_go_to_core;
        if (continue_on_cpu(cpu, this)) return do_perform_job();
        else return false;
    }
    
    bool do_perform_job() {
        bool done = (obj->*on_other_core)();
        do_return_home();
        return done;
    }
    
    void do_return_home() {
        state = state_go_home;
        if (continue_on_cpu(home_cpu, this)) do_finish();
    }
    
    void do_finish() {
        delete this;
    }
    
    void on_cpu_switch() {
        switch (state) {
            case state_go_to_core:
                do_perform_job();
                return;
            case state_go_home:
                do_finish();
                return;
            default: fail("Bad state.");
        }
    }
};

template<class obj_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)()) {
    
    no_arg_cpu_doer_t<obj_t> *fsm = new no_arg_cpu_doer_t<obj_t>();
    fsm->obj = obj;
    fsm->cpu = cpu;
    fsm->on_other_core = on_other_core;
    return fsm->run();
}

template<class obj_t, class arg1_t>
struct one_arg_cpu_doer_t :
    public cpu_message_t,
    public home_cpu_mixin_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, one_arg_cpu_doer_t<obj_t, arg1_t> >
{
    obj_t *obj;
    arg1_t arg1;
    bool (obj_t::*on_other_core)(arg1_t);
    int cpu;
    enum state_t {
        state_go_to_core,
        state_go_home
    } state;
    
    bool run() {
        state = state_go_to_core;
        if (continue_on_cpu(cpu, this)) return do_perform_job();
        else return false;
    }
    
    bool do_perform_job() {
        bool done = (obj->*on_other_core)(arg1);
        do_return_home();
        return done;
    }
    
    void do_return_home() {
        state = state_go_home;
        if (continue_on_cpu(home_cpu, this)) do_finish();
    }
    
    void do_finish() {
        delete this;
    }
    
    void on_cpu_switch() {
        switch (state) {
            case state_go_to_core:
                do_perform_job();
                return;
            case state_go_home:
                do_finish();
                return;
            default: fail("Bad state.");
        }
    }
};

template<class obj_t, class arg1_t>
bool do_on_cpu(int cpu, obj_t *obj, bool (obj_t::*on_other_core)(arg1_t), arg1_t arg1) {
    
    one_arg_cpu_doer_t<obj_t, arg1_t> *fsm = new one_arg_cpu_doer_t<obj_t, arg1_t>();
    fsm->obj = obj;
    fsm->arg1 = arg1;
    fsm->cpu = cpu;
    fsm->on_other_core = on_other_core;
    return fsm->run();
}
