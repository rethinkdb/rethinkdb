#include "stats.hpp"
#include "config/code.hpp"

using namespace std;
typedef basic_string<char, char_traits<char>, gnew_alloc<char> > custom_string;

/* Variable monitors */
void int_monitor_t::reset() {
    *value = 0;
}

void int_monitor_t::combine(var_monitor_t* val) {
    *value += *(((int_monitor_t*)val)->value);
}

int int_monitor_t::print(char *buf, int max_size) {
    return snprintf(buf, max_size, "%d", *value);
}

void float_monitor_t::reset() {
    *value = 0;
}

void float_monitor_t::combine(var_monitor_t* val) {
    *value += *(((float_monitor_t*)val)->value);
}

int float_monitor_t::print(char *buf, int max_size) {
    return snprintf(buf, max_size, "%f", *value);
}

/* Stats module */
map<custom_string, var_monitor_t *, std::less<custom_string>, gnew_alloc<var_monitor_t*> >* stats::get()
{
    return &registry;
}

void stats::add(int* val, custom_string desc)
{    
    assert(registry.count(desc)==0);
    registry[desc] = gnew<int_monitor_t>(val);
}

void stats::add(float* val, custom_string desc)
{
    assert(registry.count(desc)==0);
    registry[desc] = gnew<float_monitor_t>(val);
}

void stats::accumulate(stats *s)
{
    map<custom_string, var_monitor_t*, less<custom_string>, gnew_alloc<var_monitor_t*> >::const_iterator iter;
    map<custom_string, var_monitor_t*, std::less<custom_string>, gnew_alloc<var_monitor_t*> > *s_registry = s->get();
    for (iter=s_registry->begin();iter != s_registry->end();iter++)
    {
        var_monitor_t *var = registry[iter->first];
        if(var)
            var->combine(iter->second);
        else
            registry[iter->first] = iter->second;
    }

}

stats::~stats()
{
    map<custom_string, var_monitor_t *, less<custom_string>, gnew_alloc<var_monitor_t*> >::iterator iter;
    for (iter=registry.begin();iter!=registry.end();iter++)
    {
        // XXX: what to do about this?
        //delete iter->second;
    }
    registry.clear();
}

void stats::copy(const stats &rhs)
{
    map<custom_string, var_monitor_t *, less<custom_string>, gnew_alloc<var_monitor_t*> >::const_iterator iter;
    for (iter=rhs.registry.begin();iter!=rhs.registry.end();iter++)
    {
        /* figure out the type of object it was, then create a copy of that. */
        if(int_monitor_t *value = dynamic_cast<int_monitor_t*>(iter->second)) {
            this->registry[iter->first] = gnew<int_monitor_t>(gnew<int>(*(value->value)));
        } else if(float_monitor_t *value = dynamic_cast<float_monitor_t*>(iter->second)) {
            this->registry[iter->first] = gnew<float_monitor_t>(gnew<float>(*(value->value)));
        }
    }
    this->conn_fsm = rhs.conn_fsm;
}

stats::stats(const stats &rhs)
{
    copy(rhs);
}

stats& stats::operator=(const stats &rhs)
{
    if (&rhs != this)
    {
        copy(rhs);
    }
    return *this;
}

void stats::clear()
{
    map<custom_string, var_monitor_t *, less<custom_string>, gnew_alloc<var_monitor_t*> >::iterator iter;
    for (iter=registry.begin();iter!=registry.end();iter++)
    {
        //iter->second->clear();
    }       
}
