#ifndef __CONN_ACCEPTOR_HPP__
#define __CONN_ACCEPTOR_HPP__

#include "arch/arch.hpp"
#include "utils.hpp"
#include "conn_fsm.hpp"
#include "perfmon.hpp"

class conn_fsm_handler_t;
class server_t;

/* The conn_acceptor_t is responsible for accepting incoming network connections, creating
conn_fsms to deal with them, and shutting down the network connections when the server
shuts down. It uses net_listener_t to actually accept the connections. The server_t
creates one conn_acceptor_t for the entire lifetime of the server. */

class conn_acceptor_t :
    public net_listener_callback_t,
    public home_cpu_mixin_t
{
    friend class conn_fsm_handler_t;

public:
    conn_acceptor_t(server_t *server);
    
    void start();
    
    struct shutdown_callback_t {
        virtual void on_conn_acceptor_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);
    
    ~conn_acceptor_t();

private:
    enum state_t {
        state_off,
        state_ready,
        state_shutting_down
    } state;
    
    server_t *server;
    
    net_listener_t *listener;
    int next_cpu;
    void on_net_listener_accept(net_conn_t *conn);
    void on_conn_fsm_close();
    
    perfmon_var_t<int> n_active_conns_perfmon;
    int n_active_conns;
    intrusive_list_t<conn_fsm_handler_t> conn_handlers[MAX_CPUS];
    
    bool shutdown_conns_on_this_core();   // Called on each thread
    shutdown_callback_t *shutdown_callback;
};

/* conn_fsm_handler_t takes care of a particular connection. There is a one-to-one
relationship between conn_fsm_handler_t and conn_fsm_t. The reason why
conn_fsm_handler_t is a separate type from conn_fsm_t is to make it so that
conn_fsm_t isn't too closely tied to conn_acceptor_t. */

class conn_fsm_handler_t :
    public conn_fsm_shutdown_callback_t,
    public intrusive_list_node_t<conn_fsm_handler_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, conn_fsm_handler_t>
{
    friend class conn_acceptor_t;

public:
    conn_fsm_handler_t(conn_acceptor_t *parent, net_conn_t *conn);
    bool create_conn_fsm();
    void on_conn_fsm_quit();
    void on_conn_fsm_shutdown();
    bool cleanup();
    ~conn_fsm_handler_t();
    
private:
    enum state_t {
        state_go_to_cpu,
        state_waiting_for_conn_fsm_quit,
        state_waiting_for_conn_fsm_shutdown,
        state_return_from_cpu
    } state;
    conn_acceptor_t *parent;
    net_conn_t *conn;

public:
    conn_fsm_t *conn_fsm;
};

#endif /* __CONN_ACCEPTOR_HPP__ */
