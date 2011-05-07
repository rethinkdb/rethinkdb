#ifndef __LOAD_BALANCER__
#define __LOAD_BALANCER__

#include "failover.hpp"
#include "utils.hpp"
#include "conn_acceptor.hpp"
#include <boost/scoped_ptr.hpp>

/* elb_t will interact with Amazon ELB to make it work with our failover */
struct elb_t :
    public home_thread_mixin_t,
    public failover_callback_t
{
public:
    enum role_t {
        master = 0,
        slave,
    };
private:
    role_t role;
public:
    elb_t(role_t role, int port);
    ~elb_t();

private:
    int port;
    boost::scoped_ptr<conn_acceptor_t> conn_acceptor;

private:
    friend class failover_t;
    void on_failure();
    void on_resume();
};

#endif
