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
    do_on_cpu(cpu, c, &conn_fsm_handler_t::create_conn_fsm);
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

bool conn_acceptor_t::shutdown(shutdown_callback_t *cb) {
    
    assert(state == state_ready);
    
    /* Stop accepting new network connections */
    
    delete listener;
    listener = NULL;
    
    /* Notify any existing network connections to shut down */
    
    for (int i = 0; i < get_num_cpus(); i++) {
        do_on_cpu(i, this, &conn_acceptor_t::shutdown_conns_on_this_core);
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

bool conn_acceptor_t::shutdown_conns_on_this_core() {
    
    while (conn_fsm_handler_t *h = conn_handlers[get_cpu_id()].head()) {
    
        assert(h->state == conn_fsm_handler_t::state_waiting_for_conn_fsm_quit);
        
        h->conn_fsm->start_quit();
        
        /* start_quit() should have caused the conn_fsm_handler_t to be removed
        from the list because it should have caused the conn_fsm_t to send an
        on_conn_fsm_quit() to the conn_fsm_handler_t. */
        assert(h != conn_handlers[get_cpu_id()].head());
    }
    
    return true;
}

conn_fsm_handler_t::conn_fsm_handler_t(conn_acceptor_t *parent, net_conn_t *conn)
    : state(state_go_to_cpu), parent(parent), conn(conn), conn_fsm(NULL) {
}

bool conn_fsm_handler_t::create_conn_fsm() {
            
    request_handler_t *rh = new memcached_handler_t(parent->server);
    conn_fsm = new conn_fsm_t(conn, this, rh);
    rh->conn_fsm = conn_fsm;
    
    parent->conn_handlers[get_cpu_id()].push_back(this);
    
    state = state_waiting_for_conn_fsm_quit;
    return true;
}
 
/* Shutting down a connection might not be an atomic process because there might be
outstanding btree_fsm_ts that the conn_fsm_t is still waiting for. We get an
on_conn_fsm_quit() when the conn_fsm begins the process of shutting down, and an
on_conn_fsm_shutdown() when it finishes shutting down. */

void conn_fsm_handler_t::on_conn_fsm_quit() {
    
    assert(state == state_waiting_for_conn_fsm_quit);
    state = state_waiting_for_conn_fsm_shutdown;
    
    /* Remove ourselves from the conn_handlers list immediately so that if the server
    gets a SIGINT before we get an on_conn_fsm_shutdown(), the conn_fsm_t won't be
    told to quit when it already is quitting. */
    parent->conn_handlers[get_cpu_id()].remove(this);
}

void conn_fsm_handler_t::on_conn_fsm_shutdown() {
    
    conn_fsm = NULL;
    
    assert(state == state_waiting_for_conn_fsm_shutdown);
    state = state_return_from_cpu;
    
    do_on_cpu(parent->home_cpu, this, &conn_fsm_handler_t::cleanup);
}
 
bool conn_fsm_handler_t::cleanup() {

    parent->on_conn_fsm_close();
    delete this;
    return true;
}

conn_fsm_handler_t::~conn_fsm_handler_t() {
    assert(!conn_fsm);
}
