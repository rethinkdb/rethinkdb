#include "stats.hpp"
#include <sstream>
#include <string>

#include "config/code.hpp"
typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;

custom_string base_type::getValue()
{
    return "you should never see this.";
}

custom_string int_type::getValue()
{
    custom_stringstream ss;
    ss << *value;
    return description +  " " + ss.str();
}

custom_string double_type::getValue()
{
    custom_stringstream ss;
    ss << *value;
    return description +  " " + ss.str();
}

custom_string float_type::getValue()
{
    custom_stringstream ss;
    ss << *value;
    return description +  " " + ss.str();
}

/* stats module */
custom_string stats::get()
{
    custom_string ret = "";
    for (unsigned int i=0;i<registry.size();i++)
    {
        ret = "STAT " + registry[i]->getValue() + "\r\n";
    }
    return ret;
}

void stats::add(int* val, const char* desc)
{    
    int_type* new_var = new int_type(val);
    new_var->description = custom_string(desc);
    registry.push_back(new_var);
}

void stats::add(double* val, const char* desc)
{
    double_type* new_var = new double_type(val);
    new_var->description = custom_string(desc);
    registry.push_back(new_var);
}

void stats::add(float* val, const char* desc)
{
    float_type* new_var = new float_type(val);
    new_var->description = custom_string(desc);
    registry.push_back(new_var);
}


stats::~stats()
{
    for (unsigned int i=0;i<registry.size();i++)
    {
        delete registry[i];
    }
}