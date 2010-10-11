#include "perfmon.hpp"
#include "arch/arch.hpp"

/* The var map keeps track of all of the perfmon_watcher_t objects on each core. */

typedef std::map<std_string_t, perfmon_watcher_t*, std::less<std_string_t>,
        gnew_alloc<std::pair<std_string_t, perfmon_watcher_t*> > > var_map_t;

__thread var_map_t *var_map = NULL;

perfmon_watcher_t::perfmon_watcher_t(const char *name)
    : name(name)
{
    if (!var_map) var_map = gnew<var_map_t>();
    
    assert(var_map->find(name) == var_map->end());
    (*var_map)[name] = this;
}

perfmon_watcher_t::~perfmon_watcher_t() {
    
    assert((*var_map)[name] == this);
    var_map->erase(name);
    
    if (var_map->empty()) {
        gdelete(var_map);
        var_map = NULL;
    }
}

/* The perfmon_fsm_t goes to all the cores, collects stats for each one, and then goes back. */

struct perfmon_fsm_t :
    public home_cpu_mixin_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, perfmon_fsm_t>
{
    perfmon_stats_t *dest;
    perfmon_callback_t *callback;
    
    perfmon_fsm_t(perfmon_stats_t *dest) : dest(dest) { }
    ~perfmon_fsm_t() { }
    
    bool run(perfmon_callback_t *cb) {
        callback = NULL;
        if (do_on_cpu(0, this, &perfmon_fsm_t::gather_data)) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    bool gather_data() {
        perfmon_stats_t *local_stats = gnew<perfmon_stats_t>();
        if (var_map) {
            for (var_map_t::iterator it = var_map->begin(); it != var_map->end(); it++) {
                const std_string_t &name = (*it).first;
                const std_string_t &value = (*it).second->get_value()
                if (dest->find(name) == dest->end()) {
                    dest[name] = value;
                } else {
                    dest[name] = (*it).second->combine(value, dest[name]);
                }
            }
        }
        if (get_cpu_id() == get_num_cpus() - 1) {
            return do_on_cpu(home_cpu, this, &perfmon_fsm_t::deliver_data);
        } else {
            return do_on_cpu(get_cpu_id()+1, this, &perfmon_fsm_t::gather_data);
        }
    }
    
    bool deliver_data() {
        if (callback) callback->on_perfmon_stats();
        delete this;
        return true;
    }
};

bool perfmon_get_stats(perfmon_stats_t *dest, perfmon_callback_t *cb) {
    
    perfmon_fsm_t *fsm = new perfmon_fsm_t(dest);
    return fsm->run(cb);
}
