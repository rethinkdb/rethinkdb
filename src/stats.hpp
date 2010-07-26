#ifndef __STATS_HPP__
#define __STATS_HPP__

#include <map>
#include "config/args.hpp"
#include "config/code.hpp"
#include "message_hub.hpp"
#include "alloc/alloc_mixin.hpp"

/* Variable monitor types */
struct var_monitor_t {
public:
    enum var_type_t {
        vt_undefined,
        vt_int,
        vt_float
    };
        
public:
    var_monitor_t()
        : type(vt_undefined), name(NULL), value(NULL)
        {}
    var_monitor_t(var_type_t _type, const char *_name, void *_value)
        : type(_type), name(_name), value(_value)
        {}
    virtual ~var_monitor_t() {}

    void combine(const var_monitor_t *val);
    int print(char *buf, int max_size);
    void freeze_state();

public:
    var_type_t type;
    const char *name;
    void *value;
    char value_copy[8];
};

/* The actual stats Module */
struct stats : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, stats > {
public:
    typedef std::map<const char*, var_monitor_t, std::less<const char*>, gnew_alloc<var_monitor_t> > stats_map_t;
    
public:
    void monitor(var_monitor_t monitor);
    void accumulate(stats *s);
    void copy_from(const stats &rhs);
    
    // TODO: Watch the allocation.
    stats_map_t registry;
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

