#ifndef __STATS_HPP__
#define __STATS_HPP__

#include <vector>
#include <string>
#include "config/args.hpp"
#include "config/code.hpp"

#include "alloc/alloc_mixin.hpp"

/* Variable Types */
struct base_type {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    custom_string description;    
    virtual custom_string getValue();

};

struct int_type : public base_type, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, int_type > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    int_type(int* a) : value(a) {}
    int* value;
    int min;
    int max;
    custom_string getValue();
};

struct double_type : public base_type, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, double_type > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    double_type(double* a) : value(a) {}
    double* value;
    double min;
    double max;
    custom_string getValue();
};

struct float_type : public base_type, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, float_type > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    float_type(float* a) : value(a) {}
    float* value;
    float min;
    float max;
    custom_string getValue();
};

/* The actual stats Module */
struct stats {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;
    stats() {}
    ~stats();
    custom_string get();
    void add(int *val, const char* desc);
    void add(double *val, const char* desc);
    void add(float *val, const char* desc);
    std::vector<base_type *, gnew_alloc<base_type*> > registry;
};

/* Example:

        stats g;
        int a = 100;
        g.add(&a, "my variable");
        a = 200;
        cout << g.get() << endl;
*/

#endif