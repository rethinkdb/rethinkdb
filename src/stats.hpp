#ifndef __STATS_HPP__
#define __STATS_HPP__

#include <map>
#include <string>
#include <functional>
#include "config/args.hpp"
#include "config/code.hpp"
#include "message_hub.hpp"

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
    int_type& operator=(const int_type &rhs);
    
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
    double_type& operator=(const double_type &rhs);

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
    float_type& operator=(const float_type &rhs);

    void add(base_type* val);
    custom_string get_value();
    float* value;
    float min;
    float max;
};

/* The actual stats Module */
struct stats : public cpu_message_t, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, stats > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;
//    typedef standard_config_t::request_t request_t;
    typedef standard_config_t::conn_fsm_t conn_fsm_t;
        
    stats() : cpu_message_t(cpu_message_t::mt_stats_response) {}
    stats(const stats &rhs);
    ~stats();
    stats& operator=(const stats &rhs);

    std::map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> >* get();
    void add(int *val, const char* desc);
    void add(double *val, const char* desc);
    void add(float *val, const char* desc);

    void uncreate();
    
    /* vars */
    std::map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > registry;
    conn_fsm_t *conn_fsm;
};

/* This is what gets sent to a core when another core requests it's stats module. */
struct stats_request : public cpu_message_t, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, stats_request > {
//    typedef standard_config_t::request_t request_t;
    typedef standard_config_t::conn_fsm_t conn_fsm_t;
    explicit stats_request(int id) : cpu_message_t(cpu_message_t::mt_stats_request), requester_id(id) {}
    
    /* vars */
    int requester_id;
    conn_fsm_t *conn_fsm;
};

/* Example:

        stats g;
        int a = 100;
        g.add(&a, "my variable");
        a = 200;
        cout << g.get() << endl;
*/

#endif