#ifndef __CONN_ACCEPTOR_HPP__
#define __CONN_ACCEPTOR_HPP__

#include "arch/arch.hpp"
#include "utils.hpp"
#include "conn_fsm.hpp"

class conn_fsm_handler_t;
class server_t;
class shutdown_conns_message_t;

class conn_acceptor_t :
    public net_listener_callback_t,
    public home_cpu_mixin_t
{
    friend class shutdown_conns_message_t;
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
    
    int n_active_conns;
    intrusive_list_t<conn_fsm_handler_t> conn_handlers[MAX_CPUS];
    
    shutdown_callback_t *shutdown_callback;
};

class conn_fsm_handler_t :
    public cpu_message_t,
    public conn_fsm_shutdown_callback_t,
    public intrusive_list_node_t<conn_fsm_handler_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, conn_fsm_handler_t>
{

public:
    conn_fsm_handler_t(conn_acceptor_t *parent, net_conn_t *conn);
    void on_cpu_switch();
    void on_conn_fsm_quit();
    void on_conn_fsm_shutdown();
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
