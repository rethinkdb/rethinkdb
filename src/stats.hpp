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
    virtual custom_string get_value() = 0;
    virtual void clear() {}
    virtual ~base_type() {}
};

struct int_type : public base_type, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, int_type > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    int_type(int* a) : value(a) {}    
    void add(base_type* val);
    custom_string get_value();
    void inline clear();
    ~int_type();
    int_type(const int_type& rhs);
    int_type& operator=(const int_type& rhs);
    void copy(const int_type& rhs);

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
    void inline clear();
    ~double_type();
    double_type(const double_type& rhs);
    double_type& operator=(const double_type& rhs);
    void copy(const double_type& rhs);

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
    void inline clear();
    ~float_type();
    float_type(const float_type& rhs);
    float_type& operator=(const float_type& rhs);
    void copy(const float_type& rhs);

    float* value;
    float min;
    float max;
};

/* The actual stats Module */
struct stats : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, stats > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;
    typedef standard_config_t::conn_fsm_t conn_fsm_t;
        
    stats() : stats_added(0) {}
    stats(const stats &rhs);
    ~stats();
    stats& operator=(const stats &rhs);

    std::map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> >* get();
    void add(int *val, custom_string desc);
    void add(double *val, custom_string desc);
    void add(float *val, custom_string desc);
    void add(stats& s);
    
    void uncreate();
    void clear();
    void copy(const stats &rhs);
    
    /* vars */
    // TODO: Watch the allocation. This is slow.
    std::map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > registry;
    conn_fsm_t *conn_fsm;
    int stats_added;
};

/* This is what gets sent to a core when another core requests it's stats module. */
struct stats_request : public cpu_message_t, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, stats_request > {
    typedef standard_config_t::conn_fsm_t conn_fsm_t;
    explicit stats_request(int id) : cpu_message_t(cpu_message_t::mt_stats_request), requester_id(id) {}
    
    /* vars */
    int requester_id;
    conn_fsm_t *conn_fsm;
};

struct stats_response : public cpu_message_t, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, stats_response > {
    typedef standard_config_t::conn_fsm_t conn_fsm_t;
    explicit stats_response(int id) : cpu_message_t(cpu_message_t::mt_stats_response), responsee_id(id), to_delete(false) {}
    
    /* vars */
    int responsee_id;
    conn_fsm_t *conn_fsm;
    stats_request *request;
    stats *stat;
    bool to_delete;
};

#endif