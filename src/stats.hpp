#ifndef __STATS_HPP__
#define __STATS_HPP__

#include <map>
#include "config/args.hpp"
#include "config/code.hpp"
#include "message_hub.hpp"
#include "alloc/alloc_mixin.hpp"

/* Variable monitor types */
struct var_monitor_t {
    virtual ~var_monitor_t() {}

    virtual void reset() = 0;
    virtual void combine(var_monitor_t *val) = 0;
    virtual int print(char *buf, int max_size) = 0;
};

struct int_monitor_t : public var_monitor_t
{
    int_monitor_t(int* a)
        : value(a)
        {}
    
    virtual void reset();
    virtual void combine(var_monitor_t* val);
    virtual int print(char *buf, int max_size);
    
    int *value;
};

struct float_monitor_t : public var_monitor_t
{
    float_monitor_t(float* a)
        : value(a)
        {}
    
    virtual void reset();
    virtual void combine(var_monitor_t* val);
    virtual int print(char *buf, int max_size);
    
    float *value;
};

/* The actual stats Module */
struct stats : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, stats > {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;
    typedef standard_config_t::conn_fsm_t conn_fsm_t;
        
    stats() {}
    stats(const stats &rhs);
    ~stats();
    stats& operator=(const stats &rhs);

    std::map<custom_string, var_monitor_t*, std::less<custom_string>, gnew_alloc<var_monitor_t*> >* get();
    void add(int *val, custom_string desc);
    void add(float *val, custom_string desc);
    
    void accumulate(stats *s);
    
    void clear();
    void copy(const stats &rhs);
    
    /* vars */
    // TODO: Watch the allocation. This is slow.
    std::map<custom_string, var_monitor_t *, std::less<custom_string>, gnew_alloc<var_monitor_t*> > registry;
    conn_fsm_t *conn_fsm;
};

/* This is what gets sent to a core when another core requests it's stats module. */
template <class config_t>
struct stats_msg : public cpu_message_t,
                   public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, stats_msg<config_t> >
{
public:
    typedef typename config_t::request_t request_t;
    
public:
    enum stats_msg_state {
        // Initial request for stats info
        sm_request,
        // Response with the copy of stats info
        sm_response,
        // Request to cleanup the copy
        sm_copy_cleanup,
        // Response suggesting copy has been cleaned up and it's time
        // to cleanup the msg structure itself
        sm_msg_cleanup
    };
        
    stats_msg(request_t *_request)
        : cpu_message_t(cpu_message_t::mt_stats), request(_request), stat(NULL), state(sm_request)
        {}
    
    request_t *request;
    stats *stat;
    stats_msg_state state;
};

#endif

