#ifndef __FAILOVER_HPP__
#define __FAILOVER_HPP__

#include <string>

#include "containers/intrusive_list.hpp"
#include "gated_store.hpp"
#include "utils.hpp"

class failover_t;

/* base class for failover callbacks:
 * To make something react to failover derive from this class and then add it
 * to a failover_t with failover_t::add_callback.
 */
class failover_callback_t :
    public intrusive_list_node_t<failover_callback_t>
{
public:
    failover_callback_t();
    virtual ~failover_callback_t();
private:
    friend class failover_t;

    virtual void on_failure() { }
    virtual void on_resume() { }
    virtual void on_backfill_begin() { }
    virtual void on_backfill_end() { }
    virtual void on_reverse_backfill_begin() { }
    virtual void on_reverse_backfill_end() { }

    failover_t *parent;
};

/* Failover module:
 * The failover module allows a list of failover callbacks to be registered
 * with it, when failover_t::on_failure is called it will call all of its
 * registered callback's on_failure functions. Equivalent behaviour for
 * on_resume. */
class failover_t :
    public home_thread_mixin_t
{
public:
    failover_t();
    ~failover_t();

private:
    intrusive_list_t<failover_callback_t> callbacks;
public:
    void add_callback(failover_callback_t *cb);
    void remove_callback(failover_callback_t *cb);
    void on_failure(); /* push this button when things go wrong */
    void on_resume(); /* push this button when things go right */
    void on_backfill_begin();
    void on_backfill_end();
    void on_reverse_backfill_begin();
    void on_reverse_backfill_end();
};

/* failover callback to execute an external script */
class failover_script_callback_t :
    public failover_callback_t
{
public:
    failover_script_callback_t(const char *);
    ~failover_script_callback_t();
    void on_failure();
    void on_resume();
private:
    std::string script_path;
};

/* failover callback to enable/disable sets and gets at appropriate times */
class failover_query_enabler_disabler_t :
    public failover_callback_t
{
public:
    failover_query_enabler_disabler_t(gated_set_store_interface_t *, gated_get_store_t *);
    void on_failure();
    void on_resume();
    void on_backfill_begin();
    void on_backfill_end();
private:
    gated_set_store_interface_t *set_gate;
    boost::scoped_ptr<gated_set_store_interface_t::open_t> permit_sets;
    gated_get_store_t *get_gate;
    boost::scoped_ptr<gated_get_store_t::open_t> permit_gets;
};

#endif  // __FAILOVER_HPP__
