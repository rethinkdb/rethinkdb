#include <iostream>
#include "stats.hpp"
#include <sstream>
#include <string>
#include <functional>
#include <cstdlib>

#include "config/code.hpp"
using namespace std;
typedef basic_string<char, char_traits<char>, gnew_alloc<char> > custom_string;

custom_string base_type::get_value()
{
    return "you should never see this.";
}

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
/* stats module */
map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> >* stats::get()
{
    return &registry;
}

void stats::add(int* val, const char* desc)
{    
    int_type* new_var = new int_type(val);
    registry[custom_string(desc)] = new_var;
}

void stats::add(double* val, const char* desc)
{
    double_type* new_var = new double_type(val);
    registry[custom_string(desc)] = new_var;
}

void stats::add(float* val, const char* desc)
{
    float_type* new_var = new float_type(val);
    registry[custom_string(desc)] = new_var;
}

void stats::add(stats& s)
{
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator iter;
    map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > *s_registry = s.get();
    for (iter=s_registry->begin();iter != s_registry->end();iter++)
    {
        if (registry.count(iter->first) == 0) continue;
        registry[iter->first]->add(iter->second);
    }

}

stats::~stats()
{
    uncreate();
}

void stats::uncreate()
{
    registry.clear();
}

void stats::copy(const stats &rhs)
{
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator iter;
    for (iter=rhs.registry.begin();iter!=rhs.registry.end();iter++)
    {
        /* figure out the type of object it was, then create a copy of that. */
        if(int_type *value = dynamic_cast<int_type*>(iter->second)) {
            int* val = gnew<int>(*(value->value));
            this->registry[iter->first] = new int_type(val);
        } else if(double_type *value = dynamic_cast<double_type*>(iter->second)) {
            double* val = gnew<double>(*(value->value));
            this->registry[iter->first] = new double_type(val);
        } else if(float_type *value = dynamic_cast<float_type*>(iter->second)) {
            float* val = gnew<float>(*(value->value));
            this->registry[iter->first] = new float_type(val);
        }
    }
    this->stats_added = rhs.stats_added;
    this->conn_fsm = rhs.conn_fsm;
}

stats::stats(const stats &rhs) : cpu_message_t(cpu_message_t::mt_stats_response)
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

int_type& int_type::operator=(const int_type &rhs)
{
    if (&rhs != this)
    {
        int* val = new int(*(rhs.value));
        this->value = val;
        this->min = rhs.min;
        this->max = rhs.max;
    }
    return *this;
}

double_type& double_type::operator=(const double_type &rhs)
{
    if (&rhs != this)
    {
        double* val = new double(*(rhs.value));
        this->value = val;
        this->min = rhs.min;
        this->max = rhs.max;
    }
    return *this;
}

float_type& float_type::operator=(const float_type &rhs)
{
    if (&rhs != this)
    {
        float* val = new float(*(rhs.value));
        this->value = val;
        this->min = rhs.min;
        this->max = rhs.max;
    }
    return *this;
}

int_type::int_type(const int_type &rhs)
{
    int* val = new int(*(rhs.value));
    this->value = val;
    this->min = rhs.min;
    this->max = rhs.max;
}

double_type::double_type(const double_type &rhs)
{
    double* val = new double(*(rhs.value));
    this->value = val;
    this->min = rhs.min;
    this->max = rhs.max;
}

float_type::float_type(const float_type &rhs)
{
    float* val = new float(*(rhs.value));
    this->value = val;
    this->min = rhs.min;
    this->max = rhs.max;
}

void stats::clear()
{
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::iterator iter;
    for (iter=registry.begin();iter!=registry.end();iter++)
    {
        iter->second->clear();
    }       
}

void int_type::clear()
{
    *value = 0;
}

void double_type::clear()
{
    *value = 0;
}

void float_type::clear()
{
    *value = 0;
}
