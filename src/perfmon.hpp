#ifndef __PERFMON_HPP__
#define __PERFMON_HPP__

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

/* The actual perfmon Module */
struct perfmon_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, perfmon_t> {
public:
    typedef std::map<const char*, var_monitor_t, std::less<const char*>, gnew_alloc<var_monitor_t> > perfmon_map_t;
    static const char* name;    
public:
    void monitor(var_monitor_t monitor);
    void accumulate(perfmon_t *s);
    void copy_from(const perfmon_t &rhs);
    
    // TODO: Watch the allocation.
    perfmon_map_t registry;
};



/* This is what gets sent to a core when another core requests it's perfmon module. */
template <class config_t>
struct perfmon_msg : public cpu_message_t,
                     public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, perfmon_msg<config_t> >
{
public:
    typedef typename config_t::request_t request_t;
    static const char* name;
public:
    enum perfmon_msg_state {
        // Initial request for perfmon info
        sm_request,
        // Response with the copy of perfmon info
        sm_response,
        // Request to cleanup the copy
        sm_copy_cleanup,
        // Response suggesting copy has been cleaned up and it's time
        // to cleanup the msg structure itself
        sm_msg_cleanup
    };
        
    perfmon_msg(request_t *_request)
        : cpu_message_t(cpu_message_t::mt_perfmon), request(_request), perfmon(NULL), state(sm_request)
        {}
    
    request_t *request;
    perfmon_t *perfmon;
    perfmon_msg_state state;
};
template<class config_t> const char* perfmon_msg<config_t>::name = "perfmon_msg";
#endif // __PERFMON_HPP__

