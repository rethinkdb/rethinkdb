#include "perfmon.hpp"
#include "arch/arch.hpp"
#include "utils.hpp"

/* The var list keeps track of all of the perfmon_watcher_t objects on each core. */

__thread intrusive_list_t<perfmon_watcher_t> *var_list = NULL;

perfmon_watcher_t::perfmon_watcher_t(const char *name, perfmon_combiner_t *combiner)
    : name(name), combiner(combiner)
{
    if (!var_list) var_list = gnew<intrusive_list_t<perfmon_watcher_t> >();
    var_list->push_back(this);
}

perfmon_watcher_t::~perfmon_watcher_t() {
    
    var_list->remove(this);
    if (var_list->empty()) {
        gdelete(var_list);
        var_list = NULL;
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
        if (var_list) {
            for (perfmon_watcher_t *var = var_list->head(); var; var = var_list->next(var)) {
                const std_string_t &value = var->get_value();
                if (dest->find(var->name) == dest->end()) {
                    (*dest)[var->name] = value;
                } else {
                    if (var->combiner) {
                        (*dest)[var->name] = var->combiner(value, (*dest)[var->name]);
                    } else {
                        fail("Two perfmon watchers are both called %s and one lacks a combiner "
                            "function.", var->name);
                    }
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

std_string_t perfmon_combiner_sum(std_string_t v1, std_string_t v2) {
    
    std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > s;
    s << (atoll(v1.c_str()) + atoll(v2.c_str()));
    return s.str();
}

static void read_average(const char *s, long long int *num, int *count) {
    switch (sscanf(s, "%lld (average of %d)", num, count)) {
        case 0: fail("Bad input to perfmon_average_combiner: %s", s);
        case 1: *count = 1; break;
        case 2: break;
        default: fail("But there were only two format specifiers in the string!");
    }
}

std_string_t perfmon_combiner_average(std_string_t v1, std_string_t v2) {
    
    long long int num1, num2;
    int count1, count2;
    read_average(v1.c_str(), &num1, &count1);
    read_average(v2.c_str(), &num2, &count2);
    
    char buf[20];
    sprintf(buf, "%lld (average of %d)", (num1*count1+num2*count2)/(count1+count2), count1+count2);
    return buf;
}
