#include "perfmon.hpp"
#include "arch/arch.hpp"

perfmon_watcher_t::perfmon_watcher_t(const char *name)
    : name(name)
{
    perfmon_controller_t *c = perfmon_controller_t::controller;
    perfmon_controller_t::var_map_t *&map = c->var_maps[get_cpu_id()];
    
    if (!map) map = gnew<perfmon_controller_t::var_map_t>();
    
    assert(map->find(name) == map->end());
    (*map)[name] = this;
}

perfmon_watcher_t::~perfmon_watcher_t() {
    
    perfmon_controller_t *c = perfmon_controller_t::controller;
    perfmon_controller_t::var_map_t *&map = c->var_maps[get_cpu_id()];
    
    assert((*map)[name] == this);
    map->erase(name);
    
    if (map->empty()) {
        gdelete(map);
        map = NULL;
    }
}

perfmon_controller_t::perfmon_controller_t() {
    
    assert(controller == NULL);
    controller = this;
    
    for (int i = 0; i < get_num_cpus(); i++) {
        var_maps[i] = NULL;
    }
}

perfmon_controller_t::~perfmon_controller_t() {

#ifndef NDEBUG
    for (int i = 0; i < get_num_cpus(); i++) {
        if (var_maps[i] != NULL) {
            const std_string_t &name = (*var_maps[i]->begin()).first;
            fail("A perfmon variable was never deregistered: %s", name.c_str());
        }
    }
#endif
    
    assert(controller == this);
    controller = NULL;
}

perfmon_controller_t *perfmon_controller_t::controller = NULL;

struct perfmon_fsm_t :
    public home_cpu_mixin_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, perfmon_fsm_t>
{
    int messages_out;
    perfmon_controller_t *controller;
    perfmon_stats_t *dest;
    perfmon_callback_t *callback;
    
    perfmon_fsm_t(perfmon_controller_t *controller, perfmon_stats_t *dest)
        : controller(controller), dest(dest) {
        
        assert(controller);
    }
    
    ~perfmon_fsm_t() { }
    
    bool run(perfmon_callback_t *cb) {
        callback = NULL;
        messages_out = 0;
        for (int i = 0; i < get_num_cpus(); i++) {
            do_on_cpu(i, this, &perfmon_fsm_t::gather_data);
        }
        if (messages_out == 0) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    bool gather_data() {
        perfmon_stats_t *local_stats = gnew<perfmon_stats_t>();
        if (controller->var_maps[get_cpu_id()]) {
            for (perfmon_controller_t::var_map_t::iterator it = controller->var_maps[get_cpu_id()]->begin();
                 it != controller->var_maps[get_cpu_id()]->end();
                 it++) {
                local_stats->insert(std::pair<std_string_t, std_string_t>((*it).first, (*it).second->get_value()));
            }
        }
        return do_on_cpu(home_cpu, this, &perfmon_fsm_t::deliver_data, get_cpu_id(), local_stats);
    }
    
    bool deliver_data(int cpu, perfmon_stats_t *local_stats) {
        
        for (perfmon_stats_t::iterator it = local_stats->begin();
             it != local_stats->end();
             it++) {
            dest->insert(*it);
        }
        
        gdelete_on_cpu(cpu, local_stats);
        messages_out--;
        if (messages_out == 0) {
            if (callback) callback->on_perfmon_stats();
            delete this;
        }
        return true;
    }
};

bool perfmon_controller_t::get_stats(perfmon_stats_t *dest, perfmon_callback_t *cb) {
    
    perfmon_fsm_t *fsm = new perfmon_fsm_t(this, dest);
    return fsm->run(cb);
}
