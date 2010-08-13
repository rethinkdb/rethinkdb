#include "perfmon.hpp"
#include "config/code.hpp"
#include "cpu_context.hpp"
#include "worker_pool.hpp"

/* Variable monitors */
void var_monitor_t::combine(const var_monitor_t* val) {
    (*combiner)(this, val);
}

int var_monitor_t::print(char *buf, int max_size) {
    switch(type) {
    case vt_int:
        return snprintf(buf, max_size, "%d", *(int*)value);
        break;
    case vt_long_int:
        return snprintf(buf, max_size, "%ld", *(long int*)value);
        break;
    case vt_float:
        return snprintf(buf, max_size, "%f", *(float*)value);
        break;
    case vt_int_function:
        return snprintf(buf, max_size, "%d", (*((int_function) value))());
        break;
    case vt_float_function:
        return snprintf(buf, max_size, "%f", (*((float_function) value))());
        break;
    case vt_undefined:
        check("Trying to print var_monitor_T of type vt_undefined", 1);
        break;
    default:
        check("Trying to print var_monitor_T of unrecognized type", 1);
        break;
    };
    return 0;
}

void var_monitor_t::freeze_state() {
    switch(type) {
    case vt_int:
        memcpy(value_copy, value, sizeof(int));
        value = value_copy;
        break;
    case vt_long_int:
        memcpy(value_copy, value, sizeof(long int));
        value = value_copy;
        break;
    case vt_float:
        memcpy(value_copy, value, sizeof(int));
        value = value_copy;
        break;
    case vt_int_function:
        //keep it thawed
        break;
    case vt_float_function:
        //keep it thawed
        break;
    case vt_undefined:
        check("Trying to freeze var_monitor_T of type vt_undefined", 1);
        break;
    default:
        check("Trying to freeze var_monitor_T of unrecognized type", 1);
        break;
    };
}

/* Perfmon module */
void perfmon_t::monitor(var_monitor_t monitor)
{
    assert(registry.count(monitor.name) == 0);
    registry[monitor.name] = monitor;
}

void perfmon_t::accumulate(perfmon_t *s)
{
    perfmon_map_t *s_registry = &s->registry;
    for (perfmon_map_t::const_iterator iter = s_registry->begin(); iter != s_registry->end(); iter++)
    {
        perfmon_map_t::iterator var_iter = registry.find(iter->first);
        if(var_iter != registry.end())
            var_iter->second.combine(&iter->second);
        else
            registry[iter->first] = iter->second;
    }

}

void perfmon_t::copy_from(const perfmon_t &rhs)
{
    for (perfmon_map_t::const_iterator iter = rhs.registry.begin(); iter != rhs.registry.end(); iter++)
    {
        registry[iter->first] = iter->second;
        registry[iter->first].freeze_state();
    }
}

void perfmon_msg_t::send_back_to_free_perfmon() {
    state = sm_copy_cleanup;
    int cpu_owning_perfmon = return_cpu;
    return_cpu = get_cpu_context()->worker->workerid;
    request = NULL;
    get_cpu_context()->worker->event_queue->message_hub.store_message(cpu_owning_perfmon, this);
}
