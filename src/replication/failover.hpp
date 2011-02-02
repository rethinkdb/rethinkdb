#ifndef __FAILOVER_HPP__
#define __FAILOVER_HPP__

#include "containers/intrusive_list.hpp"
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
    /* on failure will be called when the master server fails, and on_resume
     * will be called when it comes back up. Generally these will be inverses
     * of one another in some sense of the word */
    virtual void on_failure() = 0;
    virtual void on_resume() = 0;
private:
    failover_t *parent;
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
    char *script_path;
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
};

#endif  // __FAILOVER_HPP__
