#include "conn_acceptor.hpp"
#include "server.hpp"
#include "conn_fsm.hpp"
#include "request_handler/memcached_handler.hpp"

conn_acceptor_t::conn_acceptor_t(server_t *server)
    : state(state_off), server(server), listener(NULL), next_cpu(0), n_active_conns(0) { }

conn_acceptor_t::~conn_acceptor_t() {
    assert(state == state_off);
    assert(!listener);
    assert(n_active_conns == 0);
}

void conn_acceptor_t::start() {
    
    listener = new net_listener_t(server->cmd_config->port);
    listener->set_callback(this);
    
    state = state_ready;
}

void conn_acceptor_t::on_net_listener_accept(net_conn_t *conn) {
    
    assert(state == state_ready);
    
    conn_fsm_handler_t *c = new conn_fsm_handler_t(this, conn);
    n_active_conns++;
    
    int cpu = next_cpu++ % get_num_cpus();
    if (continue_on_cpu(cpu, c)) c->on_cpu_switch();
}

void conn_acceptor_t::on_conn_fsm_close() {
    
    assert(state == state_ready || state == state_shutting_down);
    
    n_active_conns--;
    assert(n_active_conns >= 0);
    
    if (state == state_shutting_down && n_active_conns == 0) {
        if (shutdown_callback) shutdown_callback->on_conn_acceptor_shutdown();
        state = state_off;
    }
}

struct shutdown_conns_message_t :
    public cpu_message_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, shutdown_conns_message_t>
{
    enum state_t {
        state_go_to_cpu,
        state_return_from_cpu
    } state;
    conn_acceptor_t *acceptor;
    
    void on_cpu_switch() {
        switch (state) {
    
            case state_go_to_cpu: {
                
                while (conn_fsm_handler_t *h = acceptor->conn_handlers[get_cpu_id()].head()) {
                    h->conn_fsm->start_quit();   // Removes it from the list
                }
                
                state = state_return_from_cpu;
                if (continue_on_cpu(acceptor->home_cpu, this)) on_cpu_switch();
                break;
            }
            
            case state_return_from_cpu:
                delete this;
                break;
            
            default: fail("Bad state");
        }
    }
};

bool conn_acceptor_t::shutdown(shutdown_callback_t *cb) {
    
    assert(state == state_ready);
    
    /* Stop accepting new network connections */
    
    delete listener;
    listener = NULL;
    
    /* Notify any existing network connections to shut down */
    
    for (int i = 0; i < get_num_cpus(); i++) {
        shutdown_conns_message_t *m = new shutdown_conns_message_t();
        m->state = shutdown_conns_message_t::state_go_to_cpu;
        m->acceptor = this;
        
        if (continue_on_cpu(i, m)) m->on_cpu_switch();
        
        /* It's not necessary to wait for the shutdown_conns_message_t objects to return
        before proceeding to the next state. Even if we return 'true' before the
        shutdown_conns_message_t gets back, it will still get freed because the event
        queue cannot shut down before the logger has been shut down, and that requires
        at least one more round trip. Because the shutdown_conns_message_t must be before
        the reply from the shutting down the logger in the message queue, it is
        guaranteed to be freed before the event queues shut down. */
    }
    
    /* Check if we can proceed immediately */
    
    if (n_active_conns > 0) {
        state = state_shutting_down;
        shutdown_callback = cb;
        return false;
    
    } else {
        state = state_off;
        return true;
    }
}

conn_fsm_handler_t::conn_fsm_handler_t(conn_acceptor_t *parent, net_conn_t *conn)
    : state(state_go_to_cpu), parent(parent), conn(conn), conn_fsm(NULL) {
}

void conn_fsm_handler_t::on_cpu_switch() {
    switch (state) {
        
        case state_go_to_cpu: {
            
            request_handler_t *rh = new memcached_handler_t(parent->server);
            conn_fsm = new conn_fsm_t(conn, this, rh);
            rh->conn_fsm = conn_fsm;
            
            parent->conn_handlers[get_cpu_id()].push_back(this);
            
            state = state_waiting_for_conn_fsm_quit;
            break;
        }
        
        case state_return_from_cpu:
            
            parent->on_conn_fsm_close();
            delete this;
            break;
        
        default: fail("Bad state.");
    }
}

void conn_fsm_handler_t::on_conn_fsm_quit() {
    
    assert(state == state_waiting_for_conn_fsm_quit);
    state = state_waiting_for_conn_fsm_shutdown;
    
    parent->conn_handlers[get_cpu_id()].remove(this);
}

void conn_fsm_handler_t::on_conn_fsm_shutdown() {
    
    conn_fsm = NULL;
    
    assert(state == state_waiting_for_conn_fsm_shutdown);
    state = state_return_from_cpu;
    if (continue_on_cpu(parent->home_cpu, this)) on_cpu_switch();
}

conn_fsm_handler_t::~conn_fsm_handler_t() {
    assert(!conn_fsm);
}
