#ifndef __STATS_HPP__
#define __STATS_HPP__

#include <map>
#include <string>
#include <functional>
#include "config/args.hpp"
#include "config/code.hpp"

#include "alloc/alloc_mixin.hpp"

/* Variable Types */
struct base_type {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;
    virtual void add(base_type* val) {}
    virtual custom_string get_value();
};

struct int_type : public base_type, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, int_type > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    int_type(int* a) : value(a) {}
    void add(base_type* val);
    custom_string get_value();
    int* value;
    int min;
    int max;
};

struct double_type : public base_type, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, double_type > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    double_type(double* a) : value(a) {}
    void add(base_type* val);
    custom_string get_value();
    double* value;
    double min;
    double max;
};

struct float_type : public base_type, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, float_type > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    float_type(float* a) : value(a) {}
    void add(base_type* val);
    custom_string get_value();
    float* value;
    float min;
    float max;
};

/* The actual stats Module */
struct stats {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;
    stats() {}
    ~stats();
    std::map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> >* get();
    void add(int *val, const char* desc);
    void add(double *val, const char* desc);
    void add(float *val, const char* desc);
    std::map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > registry;
};

/* Example:

        stats g;
        int a = 100;
        g.add(&a, "my variable");
        a = 200;
        cout << g.get() << endl;
*/

#endif