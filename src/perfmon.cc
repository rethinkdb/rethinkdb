#include "perfmon.hpp"
#include "config/alloc.hpp"
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
    default:
        fail("Trying to print var_monitor_T of bad type");
    }
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
    default:
        fail("Trying to freeze var_monitor_T of bad type");
    }
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
    request = NULL;
    if (continue_on_cpu(return_cpu, this)) on_cpu_switch();
}

void perfmon_msg_t::on_cpu_switch() {

    worker_t *worker = get_cpu_context()->worker;
    
    switch(state) {
    case sm_request:
        // Copy our statistics into the perfmon message and send a response
        perfmon = new perfmon_t();
        perfmon->copy_from(worker->perfmon);
        state = sm_response;
        break;
    case sm_response:
        request->on_request_part_completed();
        return;
    case sm_copy_cleanup:
        // Response has been sent to the client, time to delete the
        // copy
        delete perfmon;
        state = sm_msg_cleanup;
        break;
    case sm_msg_cleanup:
        // Copy has been deleted, delete the final message and return
        delete this;
        return;
    }
    
    // continue_on_cpu() returns true if we are trying to go to the core we are
    // already on. Because it doesn't call on_cpu_switch() in that case, we call it
    // ourselves.
    if (continue_on_cpu(return_cpu, this))
        on_cpu_switch();
}