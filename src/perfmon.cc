#include "perfmon.hpp"
#include "config/code.hpp"

/* Variable monitors */
void var_monitor_t::combine(const var_monitor_t* val) {
    switch(type) {
    case vt_int:
        *((int*)value) += *((int*)val->value);
        break;
    case vt_float:
        *((float*)value) += *((float*)val->value);
        break;
    case vt_undefined:
        assert(0);
        break;
    };
}

int var_monitor_t::print(char *buf, int max_size) {
    switch(type) {
    case vt_int:
        return snprintf(buf, max_size, "%d", *(int*)value);
        break;
    case vt_float:
        return snprintf(buf, max_size, "%f", *(float*)value);
        break;
    case vt_undefined:
        assert(0);
        break;
    };
    return 0;
}

void var_monitor_t::freeze_state() {
    int size = 0;
    switch(type) {
    case vt_int:
        size = sizeof(int);
        break;
    case vt_float:
        size = sizeof(float);
        break;
    case vt_undefined:
        assert(0);
        break;
    };
    memcpy(value_copy, value, size);
    value = value_copy;
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
const char* perfmon_t::name = "perfmon_t";
