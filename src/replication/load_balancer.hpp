#ifndef __LOAD_BALANCER__
#define __LOAD_BALANCER__

#include "failover.hpp"
#include "utils.hpp"
#include "conn_acceptor.hpp"
#include <boost/scoped_ptr.hpp>

struct ignore_handler_t : public conn_handler_with_special_lifetime_t {
    virtual void talk_on_connection(UNUSED tcp_conn_t *conn) {
        /* This function gets called when the ELB port is connected to. It returns
           immediately, which means the connection is disconnected immediately. */
    }
};

struct ignore_acceptor_callback_t : public conn_acceptor_callback_t {
    virtual void make_handler_for_conn_thread(boost::scoped_ptr<conn_handler_with_special_lifetime_t>& output) {
        output.reset(new ignore_handler_t);
    }
};

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
    ignore_acceptor_callback_t ignore;
private:
    friend class failover_t;
    void on_failure();
    void on_resume();
};

#endif
