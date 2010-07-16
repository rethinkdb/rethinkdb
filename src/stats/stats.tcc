#ifndef __STATS_TCC__
#define __STATS_TCC__
#include "stats/stats.hpp"
#include "utils.hpp"
#include <sstream>
#include <string>

typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;

custom_string BaseType::getValue()
{
    return "you should never see this.";
}

custom_string IntType::getValue()
{
    custom_stringstream ss;
    ss << *value;
    return description +  " " + ss.str();
}

custom_string DoubleType::getValue()
{
    custom_stringstream ss;
    ss << *value;
    return description +  " " + ss.str();
}

custom_string FloatType::getValue()
{
    custom_stringstream ss;
    ss << *value;
    return description +  " " + ss.str();
}

/* Stats module */
custom_string Stats::get()
{
    custom_string ret = "";
    for (unsigned int i=0;i<registry.size();i++)
    {
        ret = "STAT " + registry[i]->getValue() + "\n";
    }
    return ret;
}

void Stats::add(int* val, const char* desc)
{    
    IntType* new_var = new IntType(val);
    custom_string a(desc);
    new_var->description = a;//custom_string(desc);
    registry.push_back(new_var);
}

void Stats::add(double* val, const char* desc)
{
    DoubleType* new_var = new DoubleType(val);
    new_var->description = desc;
    registry.push_back(new_var);
}

void Stats::add(float* val, const char* desc)
{
    FloatType* new_var = new FloatType(val);
    new_var->description = desc;
    registry.push_back(new_var);
}


Stats::~Stats()
{
    for (unsigned int i=0;i<registry.size();i++)
    {
        delete registry[i];
    }
}
#endif