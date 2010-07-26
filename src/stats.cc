#include "stats.hpp"
#include <sstream>
#include <string>
#include <functional>
#include <cstdlib>

#include "config/code.hpp"
using namespace std;
typedef basic_string<char, char_traits<char>, gnew_alloc<char> > custom_string;

/* type classes */
custom_string int_type::get_value()
{
    custom_stringstream ss;
    ss << *value;
    return ss.str();
}

custom_string double_type::get_value()
{
    custom_stringstream ss;
    ss << *value;
    return ss.str();
}

custom_string float_type::get_value()
{
    custom_stringstream ss;
    ss << *value;
    return ss.str();
}

void int_type::add(base_type* val)
{    
    custom_string stored_value = val->get_value();
    *value += atoi(stored_value.c_str());
}

void double_type::add(base_type* val)
{
    custom_string stored_value = val->get_value();
    *value += strtod(stored_value.c_str(), NULL);
}

void float_type::add(base_type* val)
{
    custom_string stored_value = val->get_value();
    *value += strtof(stored_value.c_str(), NULL);
}

void inline int_type::clear()    { *value = 0; }
void inline double_type::clear() { *value = 0; }
void inline float_type::clear()  { *value = 0; }

int_type::~int_type() {}
double_type::~double_type() {}
float_type::~float_type() {}

void int_type::copy(const int_type& rhs) {
    value = rhs.value;
    min = rhs.min;
    max = rhs.max;
}
int_type::int_type(const int_type& rhs) { copy(rhs); }
int_type& int_type::operator=(const int_type& rhs) {
    if (&rhs != this)
    {
        copy(rhs);
    }
    return *this;
}

void double_type::copy(const double_type& rhs) {
    value = rhs.value;
    min = rhs.min;
    max = rhs.max;
}
double_type::double_type(const double_type& rhs) { copy(rhs); }
double_type& double_type::operator=(const double_type& rhs) {
    if (&rhs != this)
    {
        copy(rhs);
    }
    return *this;
}

void float_type::copy(const float_type& rhs) {
    value = rhs.value;
    min = rhs.min;
    max = rhs.max;
}
float_type::float_type(const float_type& rhs) { copy(rhs); }
float_type& float_type::operator=(const float_type& rhs) {
    if (&rhs != this)
    {
        copy(rhs);
    }
    return *this;
}
/* stats module. */
map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> >* stats::get()
{
    return &registry;
}

void stats::add(int* val, custom_string desc)
{    
    assert(registry.count(desc)==0);
    registry[desc] = new int_type(val);
}

void stats::add(double* val, custom_string desc)
{
    assert(registry.count(desc)==0);
    registry[desc] = new double_type(val);
}

void stats::add(float* val, custom_string desc)
{
    assert(registry.count(desc)==0);
    registry[desc] = new float_type(val);
}

void stats::add(stats& s)
{
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator iter;
    map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > *s_registry = s.get();
    for (iter=s_registry->begin();iter != s_registry->end();iter++)
    {
        base_type *var = registry[iter->first];
        if(var)
            var->add(iter->second);
        else
            registry[iter->first] = iter->second;
    }

}

stats::~stats()
{
    uncreate();
}

void stats::uncreate()
{
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::iterator iter;
    for (iter=registry.begin();iter!=registry.end();iter++)
    {
        // XXX: what to do about this?
        //delete iter->second;
    }
    registry.clear();
}

void stats::copy(const stats &rhs)
{
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator iter;
    for (iter=rhs.registry.begin();iter!=rhs.registry.end();iter++)
    {
        /* figure out the type of object it was, then create a copy of that. */
        if(int_type *value = dynamic_cast<int_type*>(iter->second)) {
            this->registry[iter->first] = new int_type(gnew<int>(*(value->value)));
        } else if(double_type *value = dynamic_cast<double_type*>(iter->second)) {
            this->registry[iter->first] = new double_type(gnew<double>(*(value->value)));
        } else if(float_type *value = dynamic_cast<float_type*>(iter->second)) {
            this->registry[iter->first] = new float_type(gnew<float>(*(value->value)));
        }
    }
    this->stats_added = rhs.stats_added;
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
        uncreate();
        copy(rhs);
    }
    return *this;
}

void stats::clear()
{
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::iterator iter;
    for (iter=registry.begin();iter!=registry.end();iter++)
    {
        iter->second->clear();
    }       
}
