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


stats::~stats()
{
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator iter;
    for (iter=registry.begin();iter != registry.end();iter++)
    {
        delete &iter;
    }
}